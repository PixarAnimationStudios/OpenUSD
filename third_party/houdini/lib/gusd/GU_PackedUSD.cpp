//
// Copyright 2017 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "GU_PackedUSD.h"

#include "GT_PackedUSD.h"
#include "xformWrapper.h"
#include "meshWrapper.h"
#include "pointsWrapper.h"

#include "UT_Gf.h"
#include "UT_Usd.h"
#include "GU_USD.h"

#include "USD_StdTraverse.h"
#include "GT_PrimCache.h"
#include "USD_StageCache.h"
#include "USD_XformCache.h"
#include "boundsCache.h"

#include "pxr/usd/usd/primRange.h"

#include "pxr/usd/usdGeom/pointBased.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/points.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/scope.h"
#include "pxr/usd/usdGeom/xformable.h"


#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include <GA/GA_SaveMap.h>
#include <GT/GT_PrimInstance.h>
#include <GT/GT_GEODetail.h>
#include <GT/GT_GEOPrimPacked.h>
#include <GT/GT_PrimPointMesh.h>
#include <GT/GT_PrimPolygonMesh.h>
#include <GT/GT_RefineCollect.h>
#include <GT/GT_RefineParms.h>
#include <GT/GT_TransformArray.h>
#include <GT/GT_Util.h>
#include <GU/GU_PackedFactory.h>
#include <GU/GU_PrimPacked.h>
#include <UT/UT_DMatrix4.h>
#include <UT/UT_Map.h>

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;

#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif

namespace {

class UsdPackedFactory : public GU_PackedFactory
{
public:
    UsdPackedFactory()
        : GU_PackedFactory("PackedUSD", "Packed USD")
    {
        registerIntrinsic("usdFileName",
            StringHolderGetterCast(&GusdGU_PackedUSD::intrinsicFileName),
            StringHolderSetterCast(&GusdGU_PackedUSD::setFileName));
        registerIntrinsic("usdAltFileName",
            StringHolderGetterCast(&GusdGU_PackedUSD::intrinsicAltFileName),
            StringHolderSetterCast(&GusdGU_PackedUSD::setAltFileName));
        registerIntrinsic("usdPrimPath",
            StringHolderGetterCast(&GusdGU_PackedUSD::intrinsicPrimPath),
            StringHolderSetterCast(&GusdGU_PackedUSD::setPrimPath));
        // The USD prim's localToWorldTransform is stored in this intrinsic.
        // This may differ from the packed prim's actual transform.
        registerTupleIntrinsic("usdLocalToWorldTransform",
            IntGetterCast(&GusdGU_PackedUSD::usdLocalToWorldTransformSize),
            F64VectorGetterCast(&GusdGU_PackedUSD::usdLocalToWorldTransform),
            NULL);
        registerIntrinsic("usdFrame",
            FloatGetterCast(&GusdGU_PackedUSD::intrinsicFrame),
            FloatSetterCast(&GusdGU_PackedUSD::setFrame));
        registerIntrinsic("usdSrcPrimPath",
            StringHolderGetterCast(&GusdGU_PackedUSD::intrinsicSrcPrimPath),
            StringHolderSetterCast(&GusdGU_PackedUSD::setSrcPrimPath));
        registerIntrinsic("usdIndex",
            IntGetterCast(&GusdGU_PackedUSD::index),
            IntSetterCast(&GusdGU_PackedUSD::setIndex));
        registerIntrinsic("usdType",
            StringHolderGetterCast(&GusdGU_PackedUSD::intrinsicType));
        registerTupleIntrinsic("usdViewportPurpose",
            IntGetterCast(&GusdGU_PackedUSD::getNumPurposes),
            StringArrayGetterCast(&GusdGU_PackedUSD::getIntrinsicPurposes),
            StringArraySetterCast(&GusdGU_PackedUSD::setIntrinsicPurposes));
    }
    virtual ~UsdPackedFactory() {}

    virtual GU_PackedImpl *create() const
    {
        return new GusdGU_PackedUSD();
    }
};

static UsdPackedFactory *theFactory = NULL;
const char* k_typeName = "PackedUSD";

} // close namespace 

/* static */
GU_PrimPacked* 
GusdGU_PackedUSD::Build( 
    GU_Detail&              detail, 
    const UT_StringHolder&  fileName, 
    const SdfPath&          primPath, 
    const UsdTimeCode&      frame, 
    const char*             lod,
    GusdPurposeSet          purposes )
{   
    auto prim = GU_PrimPacked::build( detail, k_typeName );
    auto impl = UTverify_cast<GusdGU_PackedUSD *>(prim->implementation());
    impl->m_fileName = fileName;
    impl->m_primPath = primPath;
    impl->m_frame = frame;
    if( lod )
        impl->intrinsicSetViewportLOD( lod );
    impl->setPurposes( purposes );

    // It seems that Houdini may reuse memory for packed implementations with
    // out calling the constructor to initialize data. 
    impl->resetCaches();
    impl->updateTransform();
    return prim;
}

/* static */
GU_PrimPacked* 
GusdGU_PackedUSD::Build( 
    GU_Detail&              detail, 
    const UT_StringHolder&  fileName, 
    const SdfPath&          primPath, 
    const SdfPath&          srcPrimPath,
    int                     index,
    const UsdTimeCode&      frame, 
    const char*             lod,
    GusdPurposeSet          purposes )
{   
    auto prim = GU_PrimPacked::build( detail, k_typeName );
    auto impl = UTverify_cast<GusdGU_PackedUSD *>(prim->implementation());
    impl->m_fileName = fileName;
    impl->m_primPath = primPath;
    impl->m_srcPrimPath = srcPrimPath;
    impl->m_index = index;
    impl->m_frame = frame;
    if( lod )
        impl->intrinsicSetViewportLOD( lod );
    impl->setPurposes( purposes );

    // It seems that Houdini may reuse memory for packed implementations with
    // out calling the constructor to initialize data. 
    impl->resetCaches();
    impl->updateTransform();
    return prim;
}

GusdGU_PackedUSD::GusdGU_PackedUSD()
    : GU_PackedImpl()
    , m_transformCacheValid(false)
    , m_masterPathCacheValid(false)
    , m_index(-1)
    , m_frame(std::numeric_limits<float>::min())
    , m_purposes( GusdPurposeSet( GUSD_PURPOSE_DEFAULT | GUSD_PURPOSE_PROXY ))
{
}

GusdGU_PackedUSD::GusdGU_PackedUSD( const GusdGU_PackedUSD &src )
    : GU_PackedImpl( src )
    , m_fileName( src.m_fileName )
    , m_altFileName( src.m_altFileName )
    , m_primPath( src.m_primPath )
    , m_srcPrimPath( src.m_srcPrimPath )
    , m_index( src.m_index )
    , m_frame( src.m_frame )
    , m_purposes( src.m_purposes )
    , m_usdPrim( src.m_usdPrim )
    , m_stageProxy( src.m_stageProxy )
    , m_boundsCache( src.m_boundsCache )
    , m_transformCacheValid( src.m_transformCacheValid )
    , m_transformCache( src.m_transformCache )
    , m_masterPathCacheValid( src.m_masterPathCacheValid )
    , m_masterPathCache( src.m_masterPathCache )
    , m_gtPrimCache( NULL )
{
    topologyDirty();
}

GusdGU_PackedUSD::~GusdGU_PackedUSD()
{
}

void
GusdGU_PackedUSD::install( GA_PrimitiveFactory &gafactory )
{
    if (theFactory)
        return;

    theFactory = new UsdPackedFactory();
    GU_PrimPacked::registerPacked( &gafactory, theFactory );  

    const GA_PrimitiveDefinition* def = 
        GU_PrimPacked::lookupTypeDef( k_typeName );    

    // Bind GEOPrimCollect for collecting GT prims for display in the viewport
    static GusdGT_PrimCollect *collector = new GusdGT_PrimCollect();
    collector->bind(def->getId());  
}

GA_PrimitiveTypeId 
GusdGU_PackedUSD::typeId()
{
    return GU_PrimPacked::lookupTypeId( k_typeName );
}

void
GusdGU_PackedUSD::resetCaches()
{
    m_boundsCache.makeInvalid();
    m_usdPrim = GusdUSD_PrimHolder();
    m_stageProxy = GusdUSD_StageProxyHandle();
    m_transformCacheValid = false;
    m_gtPrimCache = GT_PrimitiveHandle();
}

void
GusdGU_PackedUSD::updateTransform()
{
    const UT_Matrix4D& m = getUsdTransform();

    UT_Vector3D p;
    m.getTranslates(p);

    GEO_PrimPacked *prim = getPrim();
    prim->setLocalTransform(UT_Matrix3D(m));
    prim->setPos3(0, p );
}

void
GusdGU_PackedUSD::setFileName( const UT_StringHolder& fileName ) 
{
    if( fileName != m_fileName )
    {
        m_fileName = fileName;
        topologyDirty();    // Notify base primitive that topology has changed
        resetCaches();
        updateTransform();
    }
}

void
GusdGU_PackedUSD::setAltFileName( const UT_StringHolder& fileName ) 
{
    if( fileName != m_altFileName )
    {
        m_altFileName = fileName;
    }
}

void
GusdGU_PackedUSD::setPrimPath( const UT_StringHolder& p ) 
{
    if( p.isstring() )
    {
        setPrimPath(SdfPath(p.buffer()));
    }
    else
    {
        setPrimPath(SdfPath());
    }
}


void
GusdGU_PackedUSD::setPrimPath( const SdfPath &path ) 
{
    if( path != m_primPath )
    {
        m_primPath = path;
        topologyDirty();    // Notify base primitive that topology has changed
        resetCaches();
        updateTransform();
    }
}

void
GusdGU_PackedUSD::setSrcPrimPath( const UT_StringHolder& p )
{
    if( p.isstring() )
    {
        setSrcPrimPath(SdfPath(p.buffer()));
    }
    else
    {
        setSrcPrimPath( SdfPath() );
    }
}

void
GusdGU_PackedUSD::setSrcPrimPath( const SdfPath &path ) 
{
    if( path != m_srcPrimPath ) {
        m_srcPrimPath = path;
    }
}

void
GusdGU_PackedUSD::setIndex( exint index ) 
{
    if( index != m_index ) {
        m_index = index;
    }
}

void
GusdGU_PackedUSD::setFrame( const UsdTimeCode& frame ) 
{
    if( frame != m_frame )
    {
        m_frame = frame;
        topologyDirty();    // Notify base primitive that topology has changed
        resetCaches();
        updateTransform();
    }
}

void
GusdGU_PackedUSD::setFrame( fpreal frame )
{
    setFrame(UsdTimeCode(frame));
}

exint
GusdGU_PackedUSD::getNumPurposes() const
{
    exint rv = 0;
    if( m_purposes & GUSD_PURPOSE_PROXY )
        ++rv;
    if( m_purposes & GUSD_PURPOSE_RENDER )
        ++rv;
    if( m_purposes & GUSD_PURPOSE_GUIDE )
        ++rv;
    return rv;
}

void 
GusdGU_PackedUSD::setPurposes( GusdPurposeSet purposes )
{
    m_purposes = purposes;
    topologyDirty();
    resetCaches();
}

void 
GusdGU_PackedUSD::getIntrinsicPurposes( UT_StringArray& purposes ) const
{
    purposes.clear();
    if( m_purposes & GUSD_PURPOSE_PROXY )
        purposes.append( UT_StringHolder( UT_StringHolder::REFERENCE, "proxy" ));
    if( m_purposes & GUSD_PURPOSE_RENDER )
        purposes.append( UT_StringHolder( UT_StringHolder::REFERENCE, "render" ));
    if( m_purposes & GUSD_PURPOSE_GUIDE )
        purposes.append( UT_StringHolder( UT_StringHolder::REFERENCE, "guide" ));
}

void 
GusdGU_PackedUSD::setIntrinsicPurposes( const UT_StringArray& purposes )
{
    int p = GUSD_PURPOSE_DEFAULT;
    for( auto& s : purposes ) {
        if( s == "proxy" )
            p |= GUSD_PURPOSE_PROXY;
        else if( s == "render" ) 
            p |= GUSD_PURPOSE_RENDER;
        else if( s == "guide" )
            p |= GUSD_PURPOSE_GUIDE;
    }
    setPurposes( GusdPurposeSet( p ));
}

UT_StringHolder
GusdGU_PackedUSD::intrinsicType() const
{
    // Return the USD prim type so it can be displayed in the spreadsheet.
    GusdUSD_PrimHolder::ScopedLock lock;
    UsdPrim prim = getUsdPrim(lock);    
    return UT_StringHolder( prim.GetTypeName().GetText() );
}

const UT_Matrix4D &
GusdGU_PackedUSD::getUsdTransform() const
{
    if( m_transformCacheValid )
        return m_transformCache;

    GusdUSD_PrimHolder::ScopedLock lock;
    UsdPrim prim = getUsdPrim(lock);

    if( !prim  ) {
        TF_WARN( "Invalid prim! %s", m_primPath.GetText() );
        m_transformCache = UT_Matrix4D(1);
        return m_transformCache;
    }

    if( prim.IsA<UsdGeomXformable>() )
    {
        GusdUSD_XformCache::GetInstance().GetLocalToWorldTransform( 
             prim, m_frame, m_transformCache );
        m_transformCacheValid = true;
    }        
    return m_transformCache;
}

void
GusdGU_PackedUSD::usdLocalToWorldTransform(fpreal64* val, exint size) const
{
    UT_ASSERT(size == 16);

    if( isPointInstance() )
    {
        UT_Matrix4D ident(1);
        std::copy( ident.data(), ident.data()+16, val );
    }
    else
    {
        const UT_Matrix4D &m = getUsdTransform();
        std::copy( m.data(), m.data()+16, val );
    }
}

GU_PackedFactory*
GusdGU_PackedUSD::getFactory() const
{
    return theFactory;
}

GU_PackedImpl*
GusdGU_PackedUSD::copy() const
{
    return new GusdGU_PackedUSD(*this);
}

void
GusdGU_PackedUSD::clearData()
{
}

bool
GusdGU_PackedUSD::isValid() const
{
    return bool(m_usdPrim);
}

bool
GusdGU_PackedUSD::load(const UT_Options &options, const GA_LoadMap &map)
{
    update( options );
    return true;
}

void    
GusdGU_PackedUSD::update(const UT_Options &options)
{
    string fileName, altFileName, primPath;
    if( options.importOption( "usdFileName", fileName ) || 
        options.importOption( "fileName", fileName ))
    {
        m_fileName = fileName;
    }

    if( options.importOption( "usdAltFileName", altFileName ) ||
        options.importOption( "altFileName", altFileName ))
    {
        setAltFileName( altFileName );
    }

    if( options.importOption( "usdPrimPath", primPath ) ||
        options.importOption( "nodePath", primPath ))
    {
        m_primPath = primPath.empty() ? SdfPath() : SdfPath(primPath);
    }

    if( options.importOption( "usdSrcPrimPath", primPath ))
    {
        m_srcPrimPath = primPath.empty() ? SdfPath() : SdfPath(primPath);
    }

    exint index;
    if( options.importOption( "usdIndex", index ))
    {
        m_index = index;
    }

    fpreal frame;
    if( options.importOption( "usdFrame", frame ) ||
        options.importOption( "frame", frame ))
    {
        m_frame = frame;
    }

    UT_StringArray purposes;
    if( options.importOption( "usdViewportPurpose", purposes ))
    {
        setIntrinsicPurposes( purposes );
    }
    resetCaches();
}
    
bool
GusdGU_PackedUSD::save(UT_Options &options, const GA_SaveMap &map) const
{
    options.setOptionS( "usdFileName", m_fileName );
    options.setOptionS( "usdAltFileName", m_altFileName );
    options.setOptionS( "usdPrimPath", m_primPath.GetText() );
    options.setOptionS( "usdSrcPrimPath", m_srcPrimPath.GetText() );
    options.setOptionI( "usdIndex", m_index );
    options.setOptionF( "usdFrame", m_frame.GetValue() );

    UT_StringArray purposes;
    getIntrinsicPurposes( purposes );
    options.setOptionSArray( "usdViewportPurpose", purposes );
    return true;
}

bool
GusdGU_PackedUSD::getBounds(UT_BoundingBox &box) const
{
    if( m_boundsCache.isValid() )
    {
        box = m_boundsCache;
        return true;
    }

    GusdUSD_PrimHolder::ScopedLock lock;
    UsdPrim prim = getUsdPrim(lock);

    if( !prim || !m_stageProxy ) {
        cerr << "Invalid prim " << m_primPath << endl;
    }

    if(UsdGeomImageable visPrim = UsdGeomImageable(prim))
    {
        TfTokenVector purposes;
        if( m_purposes & GUSD_PURPOSE_DEFAULT )
            purposes.push_back( UsdGeomTokens->default_ );
        if( m_purposes & GUSD_PURPOSE_PROXY )
            purposes.push_back( UsdGeomTokens->proxy );
        if( m_purposes & GUSD_PURPOSE_RENDER )
            purposes.push_back( UsdGeomTokens->render );
        if( m_purposes & GUSD_PURPOSE_GUIDE )
            purposes.push_back( UsdGeomTokens->guide );

        if( GusdBoundsCache::GetInstance().ComputeUntransformedBound( 
                        prim, 
                        UsdTimeCode( m_frame ), 
                    purposes,
                        m_boundsCache )) {
                box = m_boundsCache;
                return true;          
        }
    }
    return false;
}

bool
GusdGU_PackedUSD::getRenderingBounds(UT_BoundingBox &box) const
{
    return getBounds(box);
}

void
GusdGU_PackedUSD::getVelocityRange(UT_Vector3 &min, UT_Vector3 &max) const
{

}

void
GusdGU_PackedUSD::getWidthRange(fpreal &min, fpreal &max) const
{

}

bool
GusdGU_PackedUSD::getLocalTransform(UT_Matrix4D &m) const
{
    return false;
}

bool
containsBoundable( UsdPrim p )
{
    // Return true if this prim has a boundable geom descendant.
    // Boundables are gprims and the point instancers.
    // Used when unpacking so we don't create empty GU prims.

    if( p.IsInstance() )
        return containsBoundable( p.GetMaster() );

    UsdGeomImageable ip( p );
    if( !GusdUSD_Utils::ImageablePrimHasDefaultPurpose(ip) && !p.IsMaster() )
        return false;
    
    if( p.IsA<UsdGeomBoundable>() )
        return true;

    for( auto child : p.GetChildren() )
    {
        if( containsBoundable( child ))
            return true;
    }
    return false;
}

bool
GusdGU_PackedUSD::unpackPrim( 
    GU_Detail&              destgdp,
    UsdGeomImageable        prim, 
    const SdfPath&          primPath,
    const UT_Matrix4D&      xform,
    const GT_RefineParms&   rparms,
    bool                    addPathAttributes ) const
{
    GT_PrimitiveHandle gtPrim = 
        GusdPrimWrapper::defineForRead( 
                    m_stageProxy,
                    prim,
                    m_frame,
                    m_purposes );

    if( !gtPrim ) {
        const TfToken &type = prim.GetPrim().GetTypeName();
        if( type != "PxHairman" && type != "PxProcArgs" )
            TF_WARN( "Can't convert prim for unpack. %s. Type = %s.", 
                      prim.GetPrim().GetPath().GetText(),
                      type.GetText() );
        return false;
    }
    GusdPrimWrapper* wrapper = UTverify_cast<GusdPrimWrapper*>(gtPrim.get());

    if( !wrapper->unpack( 
            destgdp,
            TfToken(fileName().buffer()),
            primPath,
            xform,
            intrinsicFrame(),
            intrinsicViewportLOD(),
            m_purposes )) {

        // If the wrapper prim does not do the unpack, do it here.
        UT_Array<GU_Detail *>   details;

        if( prim.GetPrim().IsInMaster() ) {

            gtPrim->setPrimitiveTransform( new GT_Transform( &xform, 1 ) );
        }    


        GA_Size startIndex = destgdp.getNumPrimitives();

        GT_Util::makeGEO(details, gtPrim, &rparms);

        for (exint i = 0; i < details.entries(); ++i)
        {
            copyPrimitiveGroups(*details(i), false);
            unpackToDetail(destgdp, details(i), true);
            delete details(i);
        }

        if( addPathAttributes ) { 
            // Add usdpath and usdprimpath attributes to unpacked geometry.
            GA_Size endIndex = destgdp.getNumPrimitives();

            const char *path = prim.GetPrim().GetPath().GetString().c_str();

            if( endIndex > startIndex )
            {
                GA_RWHandleS primPathAttr( 
                    destgdp.addStringTuple( GA_ATTRIB_PRIMITIVE, GUSD_PRIMPATH_ATTR, 1 ));
                GA_RWHandleS pathAttr( 
                    destgdp.addStringTuple( GA_ATTRIB_PRIMITIVE, GUSD_PATH_ATTR, 1 ));

                for( GA_Size i = startIndex; i < endIndex; ++i )
                {
                    primPathAttr.set( destgdp.primitiveOffset( i ), 0, path );
                    pathAttr.set( destgdp.primitiveOffset( i ), 0, fileName().c_str() );
                }
            }
        }
    }
    return true;
}

bool
GusdGU_PackedUSD::unpackGeometry(GU_Detail &destgdp,
                                 const char* primvarPattern) const
{
    GusdUSD_PrimHolder::ScopedLock lock;
    UsdPrim usdPrim = getUsdPrim(lock);

    if( !usdPrim )
    {
        TF_WARN( "Invalid prim found" );
        return false;
    }

    UT_Matrix4D xform(1);
    const GU_PrimPacked *prim = getPrim();
    if( prim ) {
        prim->getFullTransform4(xform);
    }

    GT_RefineParms      rparms;
    // Need to manually force polysoup to be turned off.
    rparms.setAllowPolySoup( false );

    if (primvarPattern) {
        rparms.set("usd:primvarPattern", primvarPattern);
    }

    GT_PrimitiveHandle gtPrim;

    DBG( cerr << "GusdGU_PackedUSD::unpackGeometry: " << usdPrim.GetTypeName() << ", " << usdPrim.GetPath() << endl; )

    if( usdPrim.IsInstance() )
    {
        // We can't refine instances into other usd packed prims. This may be fixed
        // soon but right now you can't have a prim path that refers to the 
        // contents of an instance. 
        // So unpack instances all the way to houdini geometry.
        // Also, note that we don't create primpath attributes for the houdini
        // geometry. There is no valid primpath for instanced geometry.

        UT_Array<UsdPrim> gprims;
        GusdUSD_StdTraverse::GetBoundableAndInstanceTraversal().FindPrims( 
                usdPrim.GetMaster(),
                m_frame,
                m_purposes,
                gprims,
                true );

        for( const auto &prim : gprims ) {

            UT_Matrix4D m;
        GusdUSD_XformCache::GetInstance().GetLocalToWorldTransform( 
                    prim, m_frame, m );            

            unpackPrim( destgdp, UsdGeomImageable(prim), SdfPath(), m, rparms, false );
        }
    }
    else {
        unpackPrim( destgdp, UsdGeomImageable( usdPrim ), m_primPath, xform, rparms, true );
    }
    return true;
}

bool
GusdGU_PackedUSD::unpack(GU_Detail &destgdp) const
{
    // Unpack with "*" as the primvar pattern, meaning unpack all primvars.
    return unpackGeometry( destgdp, "*" );
}

bool
GusdGU_PackedUSD::unpackUsingPolygons(GU_Detail &destgdp) const
{
    // Unpack with "*" as the primvar pattern, meaning unpack all primvars.
    return unpackGeometry( destgdp, "*" );
}

bool
GusdGU_PackedUSD::getInstanceKey(UT_Options& key) const
{
    key.setOptionS("f", m_fileName);
    key.setOptionS("n", m_primPath.GetString());
    key.setOptionF("t", m_frame.GetValue());
    key.setOptionI("p", m_purposes );

    if( !m_masterPathCacheValid ) {
        GusdUSD_PrimHolder::ScopedLock lock;
        UsdPrim usdPrim = getUsdPrim(lock);
        if( usdPrim.IsValid() && usdPrim.IsInstance() ) {
            m_masterPathCache = usdPrim.GetMaster().GetPrimPath().GetString();
        } 
        else {
            m_masterPathCache = "";
        }
        m_masterPathCacheValid = true;
    }

    if( !m_masterPathCache.empty() ) {
        // If this prim is an instance, replace the prim path with the 
        // master's path so that instances can share GT prims.
        key.setOptionS("n", m_masterPathCache );
    }

    return true;
}

int64 
GusdGU_PackedUSD::getMemoryUsage(bool inclusive) const
{
    int64 mem = inclusive ? sizeof(*this) : 0;

    // Don't count the (shared) GU_Detail, since that will greatly
    // over-estimate the overall memory usage.
    // mem += _detail.getMemoryUsage(false);

    return mem;
}

void 
GusdGU_PackedUSD::countMemory(UT_MemoryCounter &counter, bool inclusive) const
{
    // TODO
}

bool
GusdGU_PackedUSD::visibleGT() const
{
    return true;
}

UsdPrim 
GusdGU_PackedUSD::getUsdPrim(GusdUSD_PrimHolder::ScopedLock &lock,
                             GusdUT_ErrorContext* err) const
{
    if (BOOST_UNLIKELY(!m_usdPrim)) {

        GusdUSD_StageCacheContext cache;

        // bind accessor using the cache
        GusdUSD_StageProxy::Accessor accessor;

        const TfToken filePath(m_fileName.toStdString());

        // The m_primPath member stores a path using variant notation (meaning
        // it may contain variant selections inside {} curly braces). So set up
        // the PrimIdentifier by giving it m_primPath as a variant path.
        GusdUSD_Utils::PrimIdentifier identifier;
        identifier.SetFromVariantPath(m_primPath);
        if (cache.Bind(accessor, filePath, identifier, err)) {
            m_usdPrim = accessor.GetPrimHolderAtPath(identifier.GetPrimPath(), err);
            m_stageProxy = accessor.GetProxy();
        }
    }

    if( !m_usdPrim ) {
        return UsdPrim();
    }

    lock.Acquire(m_usdPrim, /*write*/false);
    return *lock;
}

GusdUSD_StageProxyHandle
GusdGU_PackedUSD::getProxy() const
{
    if (BOOST_UNLIKELY(!m_stageProxy)) {

        GusdUSD_StageCacheContext cache;

        const TfToken filePath(m_fileName.toStdString());

        // The m_primPath member stores a path using variant notation (meaning
        // it may contain variant selections inside {} curly braces). So set up
        // the PrimIdentifier by giving it m_primPath as a variant path.
        GusdUSD_Utils::PrimIdentifier identifier;
        identifier.SetFromVariantPath(m_primPath);

        m_stageProxy =
            cache.FindOrCreateProxy(filePath, identifier.GetVariants());
    }

    return m_stageProxy;
}

GT_PrimitiveHandle
GusdGU_PackedUSD::fullGT() const
{
    if( m_gtPrimCache )
        return m_gtPrimCache;

    GusdUSD_PrimHolder::ScopedLock lock;
    UsdPrim usdPrim = getUsdPrim(lock);
    if( !usdPrim.IsValid() ) {
        return GT_PrimitiveHandle();
    }

    m_gtPrimCache = GusdGT_PrimCache::GetInstance().GetPrim( 
                        m_stageProxy,
                        m_usdPrim, 
                        m_frame,
                        m_purposes );

    return m_gtPrimCache;
}

PXR_NAMESPACE_CLOSE_SCOPE
