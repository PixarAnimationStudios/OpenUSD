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
#include "primWrapper.h"
#include "context.h"

#include "UT_Gf.h"
#include "GT_VtArray.h"
#include "USD_XformCache.h"
#include "GU_USD.h"
#include "tokens.h"

#include <GT/GT_PrimInstance.h>
#include <GT/GT_DAIndexedString.h>
#include <GT/GT_RefineParms.h>
#include <GT/GT_DAIndirect.h>

PXR_NAMESPACE_OPEN_SCOPE

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::map;
using std::set;
using std::vector;

#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif

namespace {

// XXX Temporary until UsdTimeCode::NextTime implemented
const double TIME_SAMPLE_DELTA = 0.001;

GT_PrimitiveHandle _nullPrimReadFunc(
        const UsdGeomImageable&, UsdTimeCode, GusdPurposeSet)
{
    return GT_PrimitiveHandle();
}
        
} // anon namespace

GusdPrimWrapper::GTTypeInfoMap GusdPrimWrapper::s_gtTypeInfoMap;

GusdPrimWrapper::
USDTypeToDefineFuncMap GusdPrimWrapper::s_usdTypeToFuncMap
    = USDTypeToDefineFuncMap();

GusdPrimWrapper::
GTTypeSet GusdPrimWrapper::s_supportedNativeGTTypes
    = GTTypeSet();

namespace {

int 
getPrimType( const GT_PrimitiveHandle &prim )
{
    int primType = prim->getPrimitiveType();
    if( primType == GT_PRIM_INSTANCE ) {
        const GT_PrimInstance *inst = 
            UTverify_cast<const GT_PrimInstance *>(prim.get());
        if( inst && inst->geometry() ) {
            primType = inst->geometry()->getPrimitiveType();
        }
    }   
    return primType;
}

} // close namespace

/// static
GT_PrimitiveHandle GusdPrimWrapper::
defineForWrite(const GT_PrimitiveHandle& sourcePrim,
               const UsdStagePtr& stage,
               const SdfPath& path,
               const GusdContext& ctxt)
{
    GT_PrimitiveHandle gtUsdPrimHandle;

    if( !sourcePrim || !stage )
        return gtUsdPrimHandle;

    int primType = getPrimType( sourcePrim );

    auto mapIt = s_gtTypeInfoMap.find( primType );
    if( mapIt != s_gtTypeInfoMap.end() ) {
        gtUsdPrimHandle = mapIt->second.writeFunc(sourcePrim,
                                                  stage,
                                                  path,
                                                  ctxt);
    }
    return gtUsdPrimHandle;
}

// static
bool GusdPrimWrapper::
getPrimName( const GT_PrimitiveHandle &sourcePrim,
             std::string &primName )
{
    int primType = getPrimType( sourcePrim );

    auto mapIt = s_gtTypeInfoMap.find( primType );
    if( mapIt != s_gtTypeInfoMap.end() &&
            mapIt->second.primNameFunc ) {
        return mapIt->second.primNameFunc(sourcePrim, primName);
    }
    return false;
}

/* static */
const char* GusdPrimWrapper::
getUsdName( int primType )
{
    auto mapIt = s_gtTypeInfoMap.find( primType );
    if( mapIt != s_gtTypeInfoMap.end() ) {
        return mapIt->second.templateName;
    }
    return NULL;
}

/* static */
bool GusdPrimWrapper::
isGroupType( int primType )
{
    auto mapIt = s_gtTypeInfoMap.find( primType );
    if( mapIt != s_gtTypeInfoMap.end() ) {
        return mapIt->second.isGroupType;
    }
    return false;
}


GT_PrimitiveHandle GusdPrimWrapper::
defineForRead( const UsdGeomImageable&  sourcePrim, 
               UsdTimeCode              time,
               GusdPurposeSet           purposes )
{
    GT_PrimitiveHandle gtUsdPrimHandle;

    // Find the function registered for the source prim's type
    // to define the prim from read and call that function.
    if(sourcePrim) {
        const TfToken& typeName = sourcePrim.GetPrim().GetTypeName();
        USDTypeToDefineFuncMap::const_accessor caccessor;
        if(s_usdTypeToFuncMap.find(caccessor, typeName)) {
            gtUsdPrimHandle = caccessor->second(sourcePrim,time,purposes);
        }
        else {
            // If no function is registered for the prim's type, try to
            // find a supported base type.
            const TfType& baseType = TfType::Find<UsdSchemaBase>();
            const TfType& derivedType
                = baseType.FindDerivedByName(typeName.GetText());

            vector<TfType> ancestorTypes;
            derivedType.GetAllAncestorTypes(&ancestorTypes);

            for(size_t i=1; i<ancestorTypes.size(); ++i) {
                const TfType& ancestorType = ancestorTypes[i];
                vector<string> typeAliases = baseType.GetAliases(ancestorType);
                typeAliases.push_back(ancestorType.GetTypeName());

                for(auto const& typeAlias : typeAliases) {
                    if(s_usdTypeToFuncMap.find(caccessor, TfToken(typeAlias))) {
                        gtUsdPrimHandle = caccessor->second(sourcePrim,time,purposes);
                        USDTypeToDefineFuncMap::accessor accessor;
                        s_usdTypeToFuncMap.insert(accessor, typeName);
                        accessor->second = caccessor->second;
                        TF_WARN("Type \"%s\" not registered, using base type \"%s\".",
                                typeName.GetText(), typeAlias.c_str());
                        break;
                    }
                }
                if(gtUsdPrimHandle) break;
            }

            if(!gtUsdPrimHandle) {
                // If we couldn't find a function for the prim's type or any 
                // of it's base types, register a function which returns an
                // empty prim handle.
                registerPrimDefinitionFuncForRead(typeName, _nullPrimReadFunc);
                TF_WARN("Couldn't read unsupported USD prim type \"%s\".",
                        typeName.GetText());
            }
        }
    }
    return gtUsdPrimHandle;
}


bool GusdPrimWrapper::
registerPrimDefinitionFuncForWrite(int gtPrimId,
                                   DefinitionForWriteFunction writeFunc,
                                   GetPrimNameFunction primNameFunc,
                                   bool isGroupType,
                                   const char *typeTemplateName )
{
    if(s_gtTypeInfoMap.find(gtPrimId) != s_gtTypeInfoMap.end()) {
        return false;
    }

    s_gtTypeInfoMap[gtPrimId] = GTTypeInfo( writeFunc, primNameFunc, 
                                            isGroupType, typeTemplateName );
    s_supportedNativeGTTypes.insert(gtPrimId);

    return true;
}


bool GusdPrimWrapper::
registerPrimDefinitionFuncForRead(const TfToken& usdTypeName,
                                  DefinitionForReadFunction func)
{
    USDTypeToDefineFuncMap::accessor accessor;
    if(! s_usdTypeToFuncMap.insert(accessor, usdTypeName)) {
        return false;
    }

    accessor->second = func;

    return true;
}

bool GusdPrimWrapper::
isGTPrimSupported(const GT_PrimitiveHandle& prim)
{
    if (!prim) return false;
    
    const int primType = prim->getPrimitiveType();

    return s_supportedNativeGTTypes.find(primType)
        != s_supportedNativeGTTypes.end();
}

//-------------------------------------------------------------------------

map<GT_Owner, TfToken> GusdPrimWrapper::s_ownerToUsdInterp {
    {GT_OWNER_POINT,    UsdGeomTokens->vertex},
    {GT_OWNER_VERTEX,   UsdGeomTokens->faceVarying},
    {GT_OWNER_UNIFORM,  UsdGeomTokens->uniform},
    {GT_OWNER_CONSTANT, UsdGeomTokens->constant}};

map<GT_Owner, TfToken> GusdPrimWrapper::s_ownerToUsdInterpCurve {
    {GT_OWNER_VERTEX,   UsdGeomTokens->vertex},
    {GT_OWNER_UNIFORM,  UsdGeomTokens->uniform},
    {GT_OWNER_CONSTANT, UsdGeomTokens->constant}};

GusdPrimWrapper::GusdPrimWrapper()
    : m_time( UsdTimeCode::Default() )
    , m_visible( true )
    , m_lastXformSet( UsdTimeCode::Default() )
    , m_lastXformCompared( UsdTimeCode::Default() )
{
}

GusdPrimWrapper::GusdPrimWrapper( 
        const UsdTimeCode &time, 
        const GusdPurposeSet &purposes )
    : m_time( time )
    , m_purposes( purposes )
    , m_visible( true )
    , m_lastXformSet( UsdTimeCode::Default() )
    , m_lastXformCompared( UsdTimeCode::Default() )
{
}

GusdPrimWrapper::GusdPrimWrapper( const GusdPrimWrapper &in )
    : m_time( in.m_time )
    , m_purposes( in.m_purposes )
    , m_visible( in.m_visible )
    , m_lastXformSet( in.m_lastXformSet )
    , m_lastXformCompared( in.m_lastXformCompared )
{
}

GusdPrimWrapper::~GusdPrimWrapper()
{
}


bool 
GusdPrimWrapper::isValid() const
{ 
    return false;
}

bool
GusdPrimWrapper::unpack(
        GU_Detail&          gdr,
        const UT_StringRef& fileName,
        const SdfPath&      primPath,  
        const UT_Matrix4D&  xform,
        fpreal              frame,
        const char *        viewportLod,
        GusdPurposeSet      purposes )
{                        
    return false;
}

bool
GusdPrimWrapper::redefine( 
   const UsdStagePtr& stage,
   const SdfPath& path,
   const GusdContext& ctxt,
   const GT_PrimitiveHandle& sourcePrim )
{
    return false;
}

bool 
GusdPrimWrapper::updateFromGTPrim(
    const GT_PrimitiveHandle&  sourcePrim,
    const UT_Matrix4D&         houXform,
    const GusdContext&         ctxt,
    GusdSimpleXformCache&      xformCache)
{
    // Set the active state of the UsdPrim if any "usdactive" attributes exist
    updateActiveFromGTPrim(sourcePrim, ctxt.time);

    return true;
}

void
GusdPrimWrapper::setVisibility(const TfToken& visibility, UsdTimeCode time)
{
    if( visibility == UsdGeomTokens->invisible ) {
        m_visible = false;
    } else {
        m_visible = true;
    }

    UsdAttribute visAttr = getUsdPrimForWrite().GetVisibilityAttr();
    if( visAttr.IsValid() ) {
        TfToken oldVal;
        if( !visAttr.Get( &oldVal, 
                          UsdTimeCode::Default() ) || oldVal != UsdGeomTokens->invisible ) {
            visAttr.Set(UsdGeomTokens->invisible, UsdTimeCode::Default()); 
        }
        visAttr.Set(visibility, time); 
    }
}

void
GusdPrimWrapper::updateVisibilityFromGTPrim(
        const GT_PrimitiveHandle& sourcePrim,
        UsdTimeCode time,
        bool forceWrite )
{
    // If we're tracking visibility, set this prim's default state to
    // invisible. File-per-frame exports rely on this if the prim isn't
    // persistent throughout the frame range.
    GT_Owner attrOwner;
    GT_DataArrayHandle houAttr
        = sourcePrim->findAttribute(GUSD_VISIBLE_ATTR, attrOwner, 0);
    if(houAttr) {
        GT_String visible = houAttr->getS(0);
        if (visible) {
            if (strcmp(visible, "inherited") == 0) {
                setVisibility(UsdGeomTokens->inherited, time);
            } else if (strcmp(visible, "invisible") == 0) {
                setVisibility(UsdGeomTokens->invisible, time);
            }
        }
    }
    else if ( forceWrite ) {
        if(isVisible()) {
            setVisibility(UsdGeomTokens->inherited, time);
        } else {
            setVisibility(UsdGeomTokens->invisible, time);
        }
    }
}

void
GusdPrimWrapper::updateActiveFromGTPrim(
        const GT_PrimitiveHandle& sourcePrim,
        UsdTimeCode time)
{
    UsdPrim prim = getUsdPrimForWrite().GetPrim();

    GT_Owner attrOwner;
    GT_DataArrayHandle houAttr
        = sourcePrim->findAttribute(GUSD_ACTIVE_ATTR, attrOwner, 0);
    if (houAttr) {
        GT_String state = houAttr->getS(0);
        if (state) {
            if (strcmp(state, "active") == 0) {
                prim.SetActive(true);
            } else if (strcmp(state, "inactive") == 0) {
                prim.SetActive(false);
            }
        }
    }
}

namespace {
bool
isClose( const GfMatrix4d &m1, const GfMatrix4d &m2, double tol = 1e-10 ) {

    for(int i = 0; i < 16; ++i) {
        if(not GfIsClose(m1.GetArray()[i], m2.GetArray()[i], tol))
            return false;
    }
    return true;
}
}

void
GusdPrimWrapper::updateTransformFromGTPrim( const GfMatrix4d &xform, 
                                            UsdTimeCode time, bool force )
{
    UsdGeomImageable usdGeom = getUsdPrimForWrite();
    UsdGeomXformable prim( usdGeom );

    // Determine if we need to clear previous transformations from a stronger
    // opinion on the stage before authoring ours.
    UsdStagePtr stage = usdGeom.GetPrim().GetStage();
    UsdEditTarget currEditTarget = stage->GetEditTarget();

    // If the edit target does no mapping, it is most likely the session
    // layer which means it is in the local layer stack and can overlay
    // any xformOps.
    if ( !currEditTarget.GetMapFunction().IsNull() && 
         !currEditTarget.GetMapFunction().IsIdentity() ) {
        bool reset;
        std::vector<UsdGeomXformOp> xformVec = prim.GetOrderedXformOps(&reset);

        // The xformOps attribute is static so we only check if we haven't
        // changed anything yet. In addition nothing needs to be cleared if it
        // was previously empty.
        if (m_lastXformSet.IsDefault() && (int)xformVec.size() > 0) {
            // Load the root layer for temp, stronger opinion changes.
            stage->GetRootLayer()->SetPermissionToSave(false);
            stage->SetEditTarget(stage->GetRootLayer());
            UsdGeomXformable stagePrim( getUsdPrimForWrite() );

            // Clear the xformOps on the stronger layer, so our weaker edit
            // target (with mapping across a reference) can write out clean,
            // new transforms.
            stagePrim.ClearXformOpOrder();
            stage->SetEditTarget(currEditTarget);
        }
    }

    if( !prim )
        return;

    // Try to avoid setting the transform when we can.
    // If force it true, always write the transform (used when writting per frame)
    bool setKnot = true;
    if( !force ) {
        
        // Has the transform has been set at least once
        if( !m_lastXformSet.IsDefault() ) {

            // Is the transform at this frame the same as the last frame
            if( isClose(xform,m_xformCache) ) {
                setKnot = false;
                m_lastXformCompared = time;
            }
            else {
                // If the transform has been held for more than one frame, 
                // set a knot on the last frame
                if( m_lastXformCompared != m_lastXformSet ) {
                    prim.MakeMatrixXform().Set( m_xformCache, m_lastXformCompared );
                }
            }
        }
        else {
            // If the transform is an identity, don't set it
            if( isClose(xform,GfMatrix4d( 1.0 ))) {

                setKnot = false;
                m_lastXformCompared = time;
            }
            else {

                // If the transform was identity and now isn't, set a knot on the last frame
                if( !m_lastXformCompared.IsDefault() ) {
                    prim.MakeMatrixXform().Set( GfMatrix4d(1.0), m_lastXformCompared );
                }
            }
        }
    }

    if( setKnot ) {
        prim.MakeMatrixXform().Set( xform, time );
        m_xformCache = xform;
        m_lastXformSet = time;
        m_lastXformCompared = time;
    }
}

bool
GusdPrimWrapper::updateAttributeFromGTPrim( 
    GT_Owner owner, 
    const std::string& name,
    const GT_DataArrayHandle& houAttr, 
    UsdAttribute& usdAttr, 
    UsdTimeCode time )
{
    // return true if we need to set the value
    if( !houAttr || !usdAttr )
       return false;

    // Check to see if the current value of this attribute has changed 
    // from the last time we set the value.

    AttrLastValueKeyType key(owner, name);
    auto it = m_lastAttrValueDict.find( key );
    if( it == m_lastAttrValueDict.end()) { 

        // Set the value for the first time
        m_lastAttrValueDict.insert(
                std::make_pair(key, 
                               AttrLastValueEntry( time, houAttr->harden())));

        GusdGT_Utils::setUsdAttribute(usdAttr, houAttr, time);
        return true;
    } 
    else {
        AttrLastValueEntry& entry = it->second;
        if( houAttr->isEqual( *entry.data )) {

            // The value are the as before. Don't set.
            entry.lastCompared = time;
            return false;
        }
        else {
            if( entry.lastCompared != entry.lastSet ) {
                // Set a value on the last frame the previous value was valid.
                GusdGT_Utils::setUsdAttribute(usdAttr, entry.data, entry.lastCompared);
            }
            
            // set the new value
            GusdGT_Utils::setUsdAttribute(usdAttr, houAttr, time);

            // save this value to compare on later frames
            entry.data = houAttr->harden();
            entry.lastSet = entry.lastCompared = time;
            return true;
        }
    }
    return false;
}

bool
GusdPrimWrapper::updatePrimvarFromGTPrim( 
    const TfToken&            name,
    const GT_Owner&           owner,
    const TfToken&            interpolation,
    UsdTimeCode               time,
    const GT_DataArrayHandle& dataIn )
{
    GT_DataArrayHandle data = dataIn;
    UsdGeomImageable prim( getUsdPrimForWrite() );

    // cerr << "updatePrimvarFromGTPrim: " 
    //         << prim.GetPrim().GetPath() << ":" << name << ", " << interpolation 
    //         << ", entries = " << dataIn->entries() << endl;

    AttrLastValueKeyType key(owner, name);
    auto it = m_lastAttrValueDict.find( key );
    if( it == m_lastAttrValueDict.end() ) {

        // If we're creating an overlay this primvar might already be
        // authored on the prim. If the primvar is indexed we need to 
        // block the indices attribute, because we flatten indexed
        // primvars.
        if( UsdGeomPrimvar primvar = prim.GetPrimvar(name) ) {
            if( primvar.IsIndexed() ) {
                primvar.BlockIndices();
            }
        }

        m_lastAttrValueDict.insert(
            std::make_pair(key, 
                       AttrLastValueEntry( time, data->harden())));

        GusdGT_Utils::setPrimvarSample( prim, name, data, interpolation, time );
    }
    else {
        AttrLastValueEntry& entry = it->second;
        if( data->isEqual( *entry.data )) {
            entry.lastCompared = time;
            return false;
        }
        else {
            if( entry.lastCompared != entry.lastSet ) {
                GusdGT_Utils::setPrimvarSample( prim, name, entry.data, interpolation, entry.lastCompared );
            }
            
             if( UsdGeomPrimvar primvar = prim.GetPrimvar(name) ) {
                if( primvar.IsIndexed() ) {
                    primvar.BlockIndices();
                }
            }

            GusdGT_Utils::setPrimvarSample( prim, name, data, interpolation, time );
            entry.data = data->harden();
            entry.lastSet = entry.lastCompared = time;
            return true;
        }
    }
    return true;
}

bool
GusdPrimWrapper::updatePrimvarFromGTPrim( 
    const GT_AttributeListHandle& gtAttrs,
    const GusdGT_AttrFilter&      primvarFilter,
    const TfToken&                interpolation,
    UsdTimeCode                   time )
{
    UsdGeomImageable prim( getUsdPrimForWrite() );
    const GT_AttributeMapHandle attrMapHandle = gtAttrs->getMap();

    for(GT_AttributeMap::const_names_iterator mapIt=attrMapHandle->begin();
            !mapIt.atEnd(); ++mapIt) {

        if(!primvarFilter.matches(mapIt.name())) 
            continue;

        const int attrIndex = attrMapHandle->get(mapIt.name());
        const GT_Owner owner = attrMapHandle->getOriginalOwner(attrIndex);
        GT_DataArrayHandle attrData = gtAttrs->get(attrIndex);

        TfToken name( mapIt.name() );

        updatePrimvarFromGTPrim( 
                    TfToken( name ),
                    owner, 
                    interpolation, 
                    time, 
                    attrData );
    }
    return true;
}

void
GusdPrimWrapper::clearCaches()
{
    m_lastAttrValueDict.clear();
}

void
GusdPrimWrapper::addLeadingBookend( double curFrame, double startFrame )
{
    if( curFrame != startFrame ) {
        double bookendFrame = curFrame - TIME_SAMPLE_DELTA;

        // Ensure the stage start frame <= bookendFrame
        UsdStagePtr stage = getUsdPrimForWrite().GetPrim().GetStage();
        if(stage) {
            double startFrame = stage->GetStartTimeCode();
            if( startFrame > bookendFrame) {
                stage->SetStartTimeCode(bookendFrame);
            }
        }

        getUsdPrimForWrite().GetVisibilityAttr().Set(UsdGeomTokens->invisible,
                                       UsdTimeCode(bookendFrame));
        getUsdPrimForWrite().GetVisibilityAttr().Set(UsdGeomTokens->inherited,
                                       UsdTimeCode(curFrame));   
    }
}

void
GusdPrimWrapper::addTrailingBookend( double curFrame )
{
    double bookendFrame = curFrame - TIME_SAMPLE_DELTA;

    getUsdPrimForWrite().GetVisibilityAttr().Set(UsdGeomTokens->inherited,
                                   UsdTimeCode(bookendFrame));
    getUsdPrimForWrite().GetVisibilityAttr().Set(UsdGeomTokens->invisible,
                                   UsdTimeCode(curFrame));     
}

/* static */
GT_DataArrayHandle
GusdPrimWrapper::convertPrimvarData( const UsdGeomPrimvar& primvar, UsdTimeCode time ) {

    SdfValueTypeName typeName = primvar.GetTypeName();
    if( typeName == SdfValueTypeNames->Int )
    {
        int usdVal;
        primvar.Get( &usdVal, time );

        return new GT_Int32Array( &usdVal, 1, 1 );
    }
    else if( typeName == SdfValueTypeNames->Int64 )
    {
        int64_t usdVal;
        primvar.Get( &usdVal, time );

        return new GT_Int64Array( &usdVal, 1, 1 );
    }
    else if( typeName == SdfValueTypeNames->Float )
    {
        float usdVal;
        primvar.Get( &usdVal, time );

        return new GT_Real32Array( &usdVal, 1, 1 );
    }
    else if( typeName == SdfValueTypeNames->Double )
    {
        double usdVal;
        primvar.Get( &usdVal, time );

        return new GT_Real64Array( &usdVal, 1, 1 );
    }
    else if( typeName == SdfValueTypeNames->Float3 )
    {
        GfVec3f usdVal;
        primvar.Get( &usdVal, time );

        return new GT_Real32Array( usdVal.data(), 1, 3 );
    }
    else if( typeName == SdfValueTypeNames->Double3 )
    {
        GfVec3d usdVal;
        primvar.Get( &usdVal, time );

        return new GT_Real64Array( usdVal.data(), 1, 3 );
    }
    else if( typeName == SdfValueTypeNames->Color3f )
    {
        GfVec3f usdVal;
        primvar.Get( &usdVal, time );

        return new GT_Real32Array( usdVal.data(), 1, 3, GT_TYPE_COLOR );
    }
    else if( typeName == SdfValueTypeNames->Color3d )
    {
        GfVec3d usdVal;
        primvar.Get( &usdVal, time );

        return new GT_Real64Array( usdVal.data(), 1, 3, GT_TYPE_COLOR );
    }
    else if( typeName == SdfValueTypeNames->Normal3f )
    {
        GfVec3f usdVal;
        primvar.Get( &usdVal, time );

        return new GT_Real32Array( usdVal.data(), 1, 3, GT_TYPE_NORMAL );
    }
    else if( typeName == SdfValueTypeNames->Normal3d )
    {
        GfVec3d usdVal;
        primvar.Get( &usdVal, time );

        return new GT_Real64Array( usdVal.data(), 1, 3, GT_TYPE_NORMAL );
    }
    else if( typeName == SdfValueTypeNames->Point3f )
    {
        GfVec3f usdVal;
        primvar.Get( &usdVal, time );

        return new GT_Real32Array( usdVal.data(), 1, 3, GT_TYPE_POINT );
    }
    else if( typeName == SdfValueTypeNames->Point3d )
    {
        GfVec3d usdVal;
        primvar.Get( &usdVal, time );

        return new GT_Real64Array( usdVal.data(), 1, 3, GT_TYPE_POINT );
    }
    else if( typeName == SdfValueTypeNames->Float4 )
    {
        GfVec4f usdVal;
        primvar.Get( &usdVal, time );

        return new GT_Real32Array( usdVal.data(), 1, 4 );
    }
    else if( typeName == SdfValueTypeNames->Double4 )
    {
        GfVec4d usdVal;
        primvar.Get( &usdVal, time );

        return new GT_Real64Array( usdVal.data(), 1, 4 );
    }
    else if( typeName == SdfValueTypeNames->Quatf )
    {
        GfVec4f usdVal;
        primvar.Get( &usdVal, time );

        return new GT_Real32Array( usdVal.data(), 1, 4, GT_TYPE_QUATERNION );
    }
    else if( typeName == SdfValueTypeNames->Quatd )
    {
        GfVec4d usdVal;
        primvar.Get( &usdVal, time );

        return new GT_Real64Array( usdVal.data(), 1, 4, GT_TYPE_QUATERNION );
    }
    else if( typeName == SdfValueTypeNames->Matrix3d )
    {
        GfMatrix3d usdVal;
        primvar.Get( &usdVal, time );

        return new GT_Real64Array( usdVal.GetArray(), 1, 9, GT_TYPE_MATRIX3 );
    }
    else if( typeName == SdfValueTypeNames->Matrix4d ||
             typeName == SdfValueTypeNames->Frame4d )
    {
        GfMatrix4d usdVal;
        primvar.Get( &usdVal, time );

        return new GT_Real64Array( usdVal.GetArray(), 1, 16, GT_TYPE_MATRIX );
    }
    else if( typeName == SdfValueTypeNames->String )
    {
        string usdVal;
        primvar.Get( &usdVal, time );

        auto     gtString = new GT_DAIndexedString( 1 );
        gtString->setString( 0, 0, usdVal.c_str() );
        return gtString;
    }
    else if( typeName == SdfValueTypeNames->StringArray )
    {
        VtArray<string> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        auto gtString = new GT_DAIndexedString( usdVal.size() );
        for( size_t i = 0; i < usdVal.size(); ++i )
            gtString->setString( i, 0, usdVal[i].c_str() );
        return gtString;
    }
    else if( typeName == SdfValueTypeNames->IntArray )
    {
        VtArray<int> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        return new GusdGT_VtArray<int>(usdVal);
    }
    else if( typeName == SdfValueTypeNames->Int64Array )
    {
        VtArray<int64_t> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        return new GusdGT_VtArray<int64_t>(usdVal);
    }
    else if( typeName == SdfValueTypeNames->FloatArray )
    {
        VtArray<float> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        return new GusdGT_VtArray<float>(usdVal);
    }
    else if( typeName == SdfValueTypeNames->DoubleArray )
    {
        VtArray<double> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        return new GusdGT_VtArray<double>(usdVal);
    }
    else if( typeName == SdfValueTypeNames->Float2Array )
    {
        VtArray<GfVec2f> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        return new GusdGT_VtArray<GfVec2f>(usdVal);
    }
    else if( typeName == SdfValueTypeNames->Double2Array )
    {
        VtArray<GfVec2d> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        return new GusdGT_VtArray<GfVec2d>(usdVal);
    }
    else if( typeName == SdfValueTypeNames->Float3Array )
    {
        VtArray<GfVec3f> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        return new GusdGT_VtArray<GfVec3f>(usdVal);
    }
    else if( typeName == SdfValueTypeNames->Double3Array )
    {
        VtArray<GfVec3d> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        return new GusdGT_VtArray<GfVec3d>(usdVal);
    }
    else if( typeName == SdfValueTypeNames->Color3fArray )
    {
        VtArray<GfVec3f> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        return new GusdGT_VtArray<GfVec3f>(usdVal,GT_TYPE_COLOR);
    }
    else if( typeName == SdfValueTypeNames->Color3dArray )
    {
        VtArray<GfVec3d> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        return new GusdGT_VtArray<GfVec3d>(usdVal,GT_TYPE_COLOR);
    }
    else if( typeName == SdfValueTypeNames->Vector3fArray )
    {
        VtArray<GfVec3f> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        return new GusdGT_VtArray<GfVec3f>(usdVal, GT_TYPE_VECTOR);
    }
    else if( typeName == SdfValueTypeNames->Vector3dArray )
    {
        VtArray<GfVec3d> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        return new GusdGT_VtArray<GfVec3d>(usdVal, GT_TYPE_VECTOR);
    }
    else if( typeName == SdfValueTypeNames->Normal3fArray )
    {
        VtArray<GfVec3f> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        return new GusdGT_VtArray<GfVec3f>(usdVal, GT_TYPE_NORMAL);
    }
    else if( typeName == SdfValueTypeNames->Normal3dArray )
    {
        VtArray<GfVec3d> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        return new GusdGT_VtArray<GfVec3d>(usdVal, GT_TYPE_NORMAL);
    }
    else if( typeName == SdfValueTypeNames->Point3fArray )
    {
        VtArray<GfVec3f> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        return new GusdGT_VtArray<GfVec3f>(usdVal, GT_TYPE_POINT);
    }
    else if( typeName == SdfValueTypeNames->Point3dArray )
    {
        VtArray<GfVec3d> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        return new GusdGT_VtArray<GfVec3d>(usdVal, GT_TYPE_POINT);
    }
    else if( typeName == SdfValueTypeNames->Float4Array )
    {
        VtArray<GfVec4f> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        return new GusdGT_VtArray<GfVec4f>(usdVal);
    }
    else if( typeName == SdfValueTypeNames->Double4Array )
    {
        VtArray<GfVec4d> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        return new GusdGT_VtArray<GfVec4d>(usdVal);
    }
    else if( typeName == SdfValueTypeNames->QuatfArray )
    {
        VtArray<GfVec4f> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        return new GusdGT_VtArray<GfVec4f>(usdVal, GT_TYPE_QUATERNION);
    }
    else if( typeName == SdfValueTypeNames->QuatdArray )
    {
        VtArray<GfVec4d> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        return new GusdGT_VtArray<GfVec4d>(usdVal, GT_TYPE_QUATERNION);
    }
    else if( typeName == SdfValueTypeNames->Matrix3dArray )
    {
        VtArray<GfMatrix3d> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        return new GusdGT_VtArray<GfMatrix3d>(usdVal, GT_TYPE_MATRIX3);
    }
    else if( typeName == SdfValueTypeNames->Matrix4dArray ||
             typeName == SdfValueTypeNames->Frame4dArray )
    {
        VtArray<GfMatrix4d> usdVal;
        primvar.ComputeFlattened( &usdVal, time );
        return new GusdGT_VtArray<GfMatrix4d>(usdVal, GT_TYPE_MATRIX);
    }
    return NULL;
}

void
GusdPrimWrapper::loadPrimvars( 
    UsdTimeCode               time,
    const GT_RefineParms*     rparms,
    int                       minUniform,
    int                       minPoint,
    int                       minVertex,
    const string&             primPath,
    GT_AttributeListHandle*   vertex,
    GT_AttributeListHandle*   point,
    GT_AttributeListHandle*   primitive,
    GT_AttributeListHandle*   constant,
    const GT_DataArrayHandle& remapIndicies ) const
{
    // Primvars will be loaded if they match a provided pattern.
    // By default, set the pattern to match only "Cd". Then write
    // over this pattern if there is one provided in rparms.
    const char* Cd = "Cd";
    UT_String primvarPattern(Cd);

    if (rparms) {
        rparms->import("usd:primvarPattern", primvarPattern);
    }

    std::vector<UsdGeomPrimvar> authoredPrimvars;
    bool hasCdPrimvar = false;

    {
        UsdGeomImageable prim = getUsdPrimForRead();

        UsdGeomPrimvar colorPrimvar = prim.GetPrimvar(GusdTokens->Cd);
        if (colorPrimvar && colorPrimvar.GetAttr().HasAuthoredValueOpinion()) {
            hasCdPrimvar = true;
        }

        // It's common for "Cd" to be the only primvar to load.
        // In this case, avoid getting all other authored primvars.
        if (primvarPattern == Cd) {
            if (hasCdPrimvar) {
                authoredPrimvars.push_back(colorPrimvar);
            } else {
                // There is no authored "Cd" primvar.
                // Try to find "displayColor" instead.
                colorPrimvar = prim.GetPrimvar(UsdGeomTokens->primvarsDisplayColor);
                if (colorPrimvar &&
                    colorPrimvar.GetAttr().HasAuthoredValueOpinion()) {
                    authoredPrimvars.push_back(colorPrimvar);
                }
            }
        } else if (primvarPattern != "") {
            authoredPrimvars = prim.GetAuthoredPrimvars();
        }
    }    

    // Is it better to sort the attributes and build the attributes all at once.

    for( const UsdGeomPrimvar &primvar : authoredPrimvars )
    {
        DBG(cerr << "loadPrimvar " << primvar.GetPrimvarName() << "\t" << primvar.GetTypeName() << "\t" << primvar.GetInterpolation() << endl);

        UT_String name(primvar.GetPrimvarName());

        // One special case we always handle here is to change
        // the name of the USD "displayColor" primvar to "Cd",
        // as long as there is not already a "Cd" primvar.
        if (!hasCdPrimvar && 
            primvar.GetName() == UsdGeomTokens->primvarsDisplayColor) {
            name = Cd;
        }

        // If the name of this primvar doesn't
        // match the primvarPattern, skip it.
        if (!name.multiMatch(primvarPattern, 1, " ")) {
            continue;
        }

        GT_DataArrayHandle gtData = convertPrimvarData( primvar, time );

        if( !gtData )
        {
            TF_WARN( "Failed to convert primvar %s:%s %s.", 
                        primPath.c_str(),
                        primvar.GetPrimvarName().GetText(),
                        primvar.GetTypeName().GetAsToken().GetText() );
            continue;
        }

        // usd vertex primvars are assigned to points
        if( primvar.GetInterpolation() == UsdGeomTokens->vertex )
        {
            if( gtData->entries() < minPoint ) {
                TF_WARN( "Not enough values found for primvar: %s:%s. "
                         "%zd values given for %d points.",
                         primPath.c_str(),
                         primvar.GetPrimvarName().GetText(),
                         gtData->entries(), minPoint );
            }
            else {
                if (remapIndicies) {
                    gtData = new GT_DAIndirect( remapIndicies, gtData );
                }
                if( point ) {
                    *point = (*point)->addAttribute( name.c_str(), gtData, true );
                }
            }
        }
        else if( primvar.GetInterpolation() == UsdGeomTokens->faceVarying )
        {
            if( gtData->entries() < minVertex ) {
                TF_WARN( "Not enough values found for primvar: %s:%s. "
                         "%zd values given for %d verticies.", 
                         primPath.c_str(),
                         primvar.GetPrimvarName().GetText(), 
                         gtData->entries(), minVertex );
            }
            else if( vertex ) {           
                *vertex = (*vertex)->addAttribute( name.c_str(), gtData, true );
            }
        }
        else if( primvar.GetInterpolation() == UsdGeomTokens->uniform )
        {
            if( gtData->entries() < minUniform ) {
                TF_WARN( "Not enough values found for primvar: %s:%s. "
                         "%zd values given for %d faces.", 
                         primPath.c_str(),
                         primvar.GetPrimvarName().GetText(),
                         gtData->entries(), minUniform );
            }
            else if( primitive ) {
                *primitive = (*primitive)->addAttribute( name.c_str(), gtData, true );
            }
        }
        else if( primvar.GetInterpolation() == UsdGeomTokens->constant )
        {
            if( constant ) {
                *constant = (*constant)->addAttribute( name.c_str(), gtData, true );
            }
        }
    }
}

/* static */
GfMatrix4d
GusdPrimWrapper::computeTransform( 
        const UsdPrim&              prim,
        UsdTimeCode                 time,
        const UT_Matrix4D&          houXform,
        const GusdSimpleXformCache& xformCache ) {

    // We need the transform into the prims space.
    // If the prim is in a hierarchy that we have written on this frame, 
    // its transform will be in the xformCache. Otherwise, we can read it 
    // from the global cache. 
    //
    // The transform cache is necessary because the gobal cache 
    // will only contain transform that we read from the stage and 
    // not anything that we have modified.

    UT_Matrix4D primXform;
    auto it = xformCache.find( prim.GetPath() );
    if( it != xformCache.end() ) {
        primXform = it->second;
    }
    else if( !GusdUSD_XformCache::GetInstance().GetLocalToWorldTransform( 
                        prim,
                        time,
                        primXform )) {
        TF_WARN( "Failed to get transform for %s.", prim.GetPath().GetText() );
        primXform.identity();
    }
    return GusdUT_Gf::Cast( houXform ) / GusdUT_Gf::Cast( primXform );
}

PXR_NAMESPACE_CLOSE_SCOPE
