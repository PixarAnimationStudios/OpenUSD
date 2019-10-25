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
#include "refiner.h"

#include "GT_OldPointInstancer.h"
#include "GT_PackedUSD.h"
#include "GT_PointInstancer.h"
#include "GU_USD.h"
#include "primWrapper.h"
#include "stageCache.h"

#include <GEO/GEO_Primitive.h>
#include <GT/GT_DAIndexedString.h>
#include <GT/GT_DANumeric.h>
#include <GT/GT_GEODetail.h>
#include <GT/GT_GEOPrimPacked.h>
#include <GT/GT_PrimInstance.h>
#include <SYS/SYS_Types.h>

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::map;
using std::pair;
using std::vector;

#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (PointInstancer)
    (PxPointInstancer)
);


namespace {

GT_DataArrayHandle newDataArray( GT_Storage storage, GT_Size size,
                                 int tupleSize, GT_Type typeInfo );
void copyDataArrayItem( GT_DataArrayHandle dstData, GT_DataArrayHandle srcData, 
                        GT_Offset dstOffset, GT_Offset srcOffset );
GT_AttributeListHandle findAndAddStringAttribute( GT_AttributeListHandle attrs,
                                            const std::string& attrName,
                                            const GT_PrimitiveHandle& gtPrim);
}  

GusdRefiner::GusdRefiner(
    GusdRefinerCollector&   collector,
    const SdfPath&          pathPrefix,
    const string&           pathAttrName,
    const UT_Matrix4D&      localToWorldXform )
    : m_collector( collector )
    , m_pathPrefix( pathPrefix )
    , m_pathAttrName( pathAttrName )
    , m_localToWorldXform( localToWorldXform )
    , m_refinePackedPrims( false )
    , m_useUSDIntrinsicNames( true )
    , m_forceGroupTopPackedPrim( false )
    , m_isTopLevel( true )
    , m_buildPointInstancer( false )
    , m_buildPrototypes( false )
{
}

void
GusdRefiner::refineDetail(
    const GU_ConstDetailHandle& detail,
    const GT_RefineParms&      refineParms )
{
    m_refineParms = refineParms;

    GU_DetailHandleAutoReadLock detailLock( detail );

    GA_ROHandleS partitionAttr;
    if( !m_pathAttrName.empty() ) {
        partitionAttr = 
            detailLock->findStringTuple( 
                            GA_ATTRIB_PRIMITIVE, 
                            m_pathAttrName.c_str() );
    }

    std::vector<GA_Range> partitions;
    GA_Range primRange = detailLock->getPrimitiveRange();
    if(!partitionAttr.isValid() || primRange.getEntries() == 0) {
        partitions.push_back(primRange);
    }
    else {

        typedef UT_Map<GA_StringIndexType, GA_OffsetList> PrimPartitionMap;

        PrimPartitionMap partitionMap;
        for(GA_Iterator offsetIt(primRange); !offsetIt.atEnd(); ++offsetIt) {
            GA_StringIndexType idx = partitionAttr.getIndex(offsetIt.getOffset());
            partitionMap[idx].append(offsetIt.getOffset());
        }

        partitions.reserve(partitionMap.size());
        for(PrimPartitionMap::const_iterator mapIt=partitionMap.begin();
                mapIt != partitionMap.end(); ++mapIt) {
            partitions.push_back(GA_Range(detailLock->getPrimitiveMap(),
                                          mapIt->second));
        }
    }
    
    // Refine each geometry partition to prims that can be written to USD. 
    // The results are accumulated in buffer in the refiner.
    for(vector<GA_Range>::const_iterator rangeIt=partitions.begin();
            rangeIt != partitions.end(); ++rangeIt) {

        const GA_Range& range = *rangeIt;

        // Before we refine we need to decide if we want to coalesce packed
        // fragments. We will coalesce unless we are writing transform
        // overlays and the fragment has a name.

        GU_DetailHandleAutoReadLock detailLock( detail );

        bool overlayTransforms = false;
        GA_AttributeOwner order[] = { GA_ATTRIB_PRIMITIVE, GA_ATTRIB_DETAIL };
        const GA_Attribute *overTransformsAttr = 
            detailLock->findAttribute( GUSD_OVERTRANSFORMS_ATTR, order, 2 );
        if( overTransformsAttr ) {
            GA_ROHandleI h( overTransformsAttr );
            if( overTransformsAttr->getOwner() == GA_ATTRIB_DETAIL ) {
                overlayTransforms = h.get( GA_Offset(0) );
            }
            else {
                // assume all prims in the range have the same usdovertransforms
                // attribute value
                overlayTransforms = h.get( range.begin().getOffset() );
            }
        }
        if( overlayTransforms ) {
            // prims must be named to overlay transforms
            const GA_Attribute *primPathAttr = 
                detailLock->findPrimitiveAttribute( GUSD_PRIMPATH_ATTR );
            if( !primPathAttr ) {
                overlayTransforms = false;
            }
        }
        
        GT_RefineParms newRefineParms( refineParms );
        newRefineParms.setCoalesceFragments( m_refinePackedPrims && !overlayTransforms );

        GT_PrimitiveHandle detailPrim
                = GT_GEODetail::makeDetail( detail, &range);
        if(detailPrim) {
            detailPrim->refine(*this, &newRefineParms );
        }
    }
}

const GusdRefiner::GprimArray&
GusdRefiner::finish() {

    m_collector.finish( *this );
    return m_collector.m_gprims;
}

std::string 
GusdRefiner::createPrimPath( const std::string& primName) {
    std::string primPath;
    if( !primName.empty() && primName[0] == '/' ) {
        // Use an explicit absolute path
        primPath = primName;
    }
    else {
        // add prefix to relative path
        primPath = m_pathPrefix.GetString();
        if( !primName.empty() ) {
            if( primPath.empty() || primPath.back() != '/' ) {
                primPath += "/";
            }
            primPath += primName;
        }
        else if( !primPath.empty() && primPath.back() != '/' )
            primPath += '/';
    }

    // USD is persnikity about having a leading slash
    if( primPath[0] != '/' ) {
        primPath = "/" + primPath;
    }
    return primPath;
}

void 
GusdRefiner::addPrimitive( const GT_PrimitiveHandle& gtPrimIn )
{
    if(!gtPrimIn) {
        TF_WARN("Attempted to add invalid prim");
        return;
    }
    GT_PrimitiveHandle gtPrim = gtPrimIn;     // copy to a non-const handle
    int primType = gtPrim->getPrimitiveType();
    DBG( cerr << "GusdRefiner::addPrimitive, " << gtPrim->className() << endl );
    
    string primName;
    // Types can register a function to provide a prim name. 
    // Volumes do this to return a name stored in the f3d file. This is 
    // important for consistant cluster naming.
    string n;
    if( GusdPrimWrapper::getPrimName( gtPrim, n )) {
        primName = n;
    }

    bool refinePackedPrims = m_refinePackedPrims;
    bool primHasNameAttr = false;
    if( primName.empty() ) {

        GT_AttributeListHandle primAttrs;
        if( primType == GT_GEO_PACKED ) {
            primAttrs = UTverify_cast<const GT_GEOPrimPacked*>(gtPrim.get())->getInstanceAttributes();

        } 
        if( !primAttrs ) {
            primAttrs = gtPrim->getUniformAttributes();
        }
        if( !primAttrs ) {
            primAttrs = gtPrim->getDetailAttributes();
        }

        GT_DataArrayHandle dah;
        if( primAttrs ) {
            dah = primAttrs->get( m_pathAttrName.c_str() );
        }

        if( dah && dah->isValid() ) {
            const char *s = dah->getS(0);
            if( s != NULL ) {
                primName = s;
                primHasNameAttr = true;
            }
        }
        if( primAttrs ) {
            GT_DataArrayHandle overXformsAttr = primAttrs->get( GUSD_OVERTRANSFORMS_ATTR );
            if( overXformsAttr ) {
                if( overXformsAttr->getI32(0) != 0 ) {
                    refinePackedPrims = false;
                }
            }
        }
    }

    
    // The following is only necessary for point instancers. Prototypes 
    // can't be point instancers.
    if (!m_buildPrototypes) {

        // Check per prim if we are building a point instancer. This may cause
        // problems for point instancers with discontiguous packed prims.
        bool localBuildPointInstancer = false;
        // If we have imported USD geometry get the type to see if it is a
        // point instancer we need to overlay.
        if(auto packedUSD = dynamic_cast<const GusdGT_PackedUSD*>( gtPrim.get() )) {
            if(packedUSD->getFileName()) {

                // Get the usd src prim path used for point instancers
                const SdfPath& instancerPrimPath =
                    packedUSD->getSrcPrimPath();

                GusdStageCacheReader cache;
                if(UsdPrim prim = cache.GetPrimWithVariants(
                    packedUSD->getFileName(), instancerPrimPath).first) {
                    // Get the type name of the usd file to overlay
                    m_pointInstancerType = prim.GetTypeName();
            
                    // Make sure to set buildPointInstancer to true if we are overlaying a
                    // point instancer
                    if (m_pointInstancerType == _tokens->PointInstancer ||
                        m_pointInstancerType == _tokens->PxPointInstancer) {
                        localBuildPointInstancer = true;
                    }
                }
            }
        }
        // If we find either an instancepath or usdinstancepath attribute, build a
        // point instancer.
        GT_Owner owner;
        if(gtPrim->findAttribute("instancepath", owner, 0) ||
            gtPrim->findAttribute("usdinstancepath", owner, 0) ) {
            localBuildPointInstancer = true;
        }

        if (m_buildPointInstancer || localBuildPointInstancer) {
            // If we are building point instancer, stash prims that can be 
            // point instanced. Build the point instancer in the finish method.
            
            // If given a prim path, pass it to the collector for a custom
            // usd scope. Otherwise pass an empty SdfPath.
            SdfPath instancerPrimPath;
            if( !primName.empty() ) {
                instancerPrimPath = SdfPath(createPrimPath(primName));
            }

            if( auto packedUSD = dynamic_cast<const GusdGT_PackedUSD*>( gtPrim.get() )) {
                // Point instancer from packed usd
                instancerPrimPath = instancerPrimPath.IsEmpty() ? packedUSD->getSrcPrimPath() : instancerPrimPath;
                m_collector.addInstPrim( instancerPrimPath, gtPrim );
                return;
            }
            else if( gtPrim->getPrimitiveType() == GT_PRIM_INSTANCE ) {
                // Point instancer from packed primitives

                // A GT_PrimInstance can container more than one instance. Create 
                // an entry for each.
                auto instPrim = UTverify_cast<const GT_PrimInstance*>( gtPrim.get() );

                // TODO: If we put all geometry packed prims here, then we break
                // grouping prims for purpose
                for( size_t i = 0; i < instPrim->entries(); ++i ) {
                    m_collector.addInstPrim( instancerPrimPath, gtPrim, i );
                }
                return;
            }

            if( primType == GT_PRIM_PARTICLE || primType == GT_PRIM_POINT_MESH ) {
                // Point instancer from points with instancepath attribute

                // Check for the usdprototypespath attribute in case it is not
                // a point or primitivie attribute.
                GT_AttributeListHandle uniformAttrs = gtPrim->getUniformAttributes();
                uniformAttrs = findAndAddStringAttribute(uniformAttrs, "usdprototypespath", gtPrim);

                // Find and add a custom prototype scope attribute.
                uniformAttrs = findAndAddStringAttribute(uniformAttrs, "usdprototypesscope", gtPrim);

                gtPrim = new GusdGT_PointInstancer( 
                                    gtPrim->getPointAttributes(), 
                                    uniformAttrs );
                primType = gtPrim->getPrimitiveType();
            }
        }
    }
    // We must refine packed prims that don't have a name
    if( !primHasNameAttr && !refinePackedPrims ) {
        refinePackedPrims = true;
    }

    if( primName.empty() && 
        gtPrim->getPrimitiveType() == GusdGT_PackedUSD::getStaticPrimitiveType() ) {

        auto packedUsdPrim = UTverify_cast<const GusdGT_PackedUSD *>(gtPrim.get());
        SdfPath path = packedUsdPrim->getPrimPath().StripAllVariantSelections();
        if( m_useUSDIntrinsicNames ) {
            primName = path.GetString();
        }
        else {
            primName = path.GetName();
        }

        // We want prototypes to be children of the point instancer, so we make 
        // the usd path a relative scope of just the usd prim name
        if ( m_buildPrototypes && !primName.empty() && primName[0] == '/' ) {
            size_t idx = primName.find_last_of("/");
            primName = primName.substr(idx+1);
        } 
    }
    // If the prim path was not explicitly set, try to come up with a reasonable
    // default.
    bool addNumericSuffix = false;
    if( primName.empty() ) {

        int t = gtPrim->getPrimitiveType();
        if( t == GT_PRIM_POINT_MESH || t == GT_PRIM_PARTICLE )
            primName = "points";
        else if( t == GT_PRIM_POLYGON_MESH || t == GT_PRIM_SUBDIVISION_MESH )
            primName = "mesh";
        else if( t == GT_PRIM_CURVE_MESH )
            primName = "curve";
        else if( t == GusdGT_PointInstancer::getStaticPrimitiveType() )
            primName = "instances";
        else if(const char *n = GusdPrimWrapper::getUsdName( t ))
            primName = n;
        else
            primName = "obj";
        
        if( !primName.empty() ) {
            addNumericSuffix = true;
        }
    }

    string primPath = createPrimPath(primName);

    TfToken purpose = UsdGeomTokens->default_;
    {
        GT_Owner own = GT_OWNER_PRIMITIVE;
        GT_DataArrayHandle dah = gtPrim->findAttribute( GUSD_PURPOSE_ATTR, own, 0 );
        if( dah && dah->isValid() ) {
            purpose = TfToken(dah->getS(0));
        }
    }

    if( primType == GT_PRIM_INSTANCE ) {
 
        auto inst = UTverify_cast<const GT_PrimInstance*>(gtPrim.get());
        const GT_PrimitiveHandle geometry = inst->geometry();

        if ( geometry->getPrimitiveType() == GT_GEO_PACKED ) {

            // If we find a packed prim that has a name, this become a group (xform) in 
            // USD. If it doesn't have a name, we just accumulate the transform and recurse.

            auto packedGeo = UTverify_cast<const GT_GEOPrimPacked*>(geometry.get());
            for( GT_Size i = 0; i < inst->transforms()->entries(); ++i ) {

                UT_Matrix4D m;
                inst->transforms()->get(i)->getMatrix(m);

                UT_Matrix4D newCtm = m_localToWorldXform;
                newCtm = m* m_localToWorldXform;

                SdfPath newPath = m_pathPrefix;
                bool recurse = true;

                if( primHasNameAttr || 
                    ( m_forceGroupTopPackedPrim && m_isTopLevel )) {

                    // m_forceGroupTopPackedPrim is used when we are writing instance 
                    // prototypes. We need to add instance id attributes to the top 
                    // level group. Here we make sure that we create that group, even 
                    // if the user hasn't named it.

                    newPath = m_collector.add(  SdfPath(primPath), 
                                                addNumericSuffix,
                                                gtPrim,
                                                newCtm,
                                                purpose,
                                                m_writeCtrlFlags );
            
                    // If we are just writing transforms and encounter a packed prim, we 
                    // just want to write it's transform and not refine it further.
                    recurse = refinePackedPrims;
                }

                if( recurse ) {
                    GusdRefiner childRefiner(
                                    m_collector,
                                    newPath,
                                    m_pathAttrName,
                                    newCtm );
                    
                    childRefiner.m_refinePackedPrims = refinePackedPrims;
                    childRefiner.m_forceGroupTopPackedPrim = m_forceGroupTopPackedPrim;
                    childRefiner.m_isTopLevel = false;

                    childRefiner.m_writeCtrlFlags = m_writeCtrlFlags;
                    childRefiner.m_writeCtrlFlags.update( geometry );

#if UT_MAJOR_VERSION_INT >= 16
                    childRefiner.refineDetail( packedGeo->getPackedDetail(), m_refineParms );
#else
                    childRefiner.refineDetail( packedGeo->getPrim()->getPackedDetail(), m_refineParms );
#endif
                }
            }
            return;
        }
    }

    if( (primType != GT_GEO_PACKED || !refinePackedPrims) && 
                            GusdPrimWrapper::isGTPrimSupported(gtPrim) ) {

        UT_Matrix4D m;
        if( primType == GT_GEO_PACKED ) {
            // packed fragment
            UTverify_cast<const GT_GEOPrimPacked*>(gtPrim.get())->getFullTransform()->getMatrix(m);
        }
        else {
            gtPrim->getPrimitiveTransform()->getMatrix(m);
        }

        UT_Matrix4D newCtm = m_localToWorldXform;
        newCtm = m* m_localToWorldXform;

        m_collector.add( SdfPath(primPath),
                         addNumericSuffix,
                         gtPrim,
                         newCtm,
                         purpose,
                         m_writeCtrlFlags );
    }
    else {
        gtPrim->refine( *this, &m_refineParms );
    }
}

SdfPath
GusdRefinerCollector::add( 
    const SdfPath&              path,
    bool                        addNumericSuffix,
    GT_PrimitiveHandle          prim,
    const UT_Matrix4D&          xform,
    const TfToken &             purpose,
    const GusdWriteCtrlFlags&   writeCtrlFlagsIn )
{
    // Update the write control flags from the attributes on the prim
    GusdWriteCtrlFlags writeCtrlFlags = writeCtrlFlagsIn;

    writeCtrlFlags.update( prim );

    // If addNumericSuffix is true, use the name directly unless there
    // is a conflict. Otherwise add a numeric suffix to keep names unique.
    int64 count = 0;
    auto it = m_names.find( path );
    if( it == m_names.end() ) {
        // Name has not been used before
        m_names[path] = NameInfo(m_gprims.size() - 1);
        if( !addNumericSuffix ) {
            m_gprims.push_back(GprimArrayEntry(path,prim,xform,purpose,writeCtrlFlags));
            return path;
        }
    }
    else {
        if( !addNumericSuffix && it->second.count == 0 ) {

            for( GprimArray::iterator pit = m_gprims.begin(); pit != m_gprims.end(); ++pit ) {
                if( pit->path == path ) {
                    // We have a name conflict. Go back and change the 
                    // name of the first prim to use this name.
                    pit->path = SdfPath( pit->path.GetString() + "_0" );
                }
                else if( TfStringStartsWith( pit->path.GetString(), path.GetString() ) ) {
                    pit->path = SdfPath( path.GetString() + "_0" + pit->path.GetString().substr( path.GetString().length() ));
                }
            }
        }
        ++it->second.count;
        count = it->second.count;
    }

    // Add a numeric suffix to get a unique name
    SdfPath newPath( TfStringPrintf("%s_%" SYS_PRId64, path.GetText(), count));

    m_gprims.push_back(GprimArrayEntry(newPath,prim,xform,purpose,writeCtrlFlags));
    return newPath;
}

void
GusdRefinerCollector::finish( GusdRefiner& refiner )
{
    // If we are building a point instancer, as packed prims are added they 
    // have been collected into m_instancePrims sorted by "srcPrimPath". 
    // Build a GT_PointPrimMesh for each entry in this map. 
 
    for( auto const & instancerMapIt : m_instancePrims ) {

        const SdfPath &instancerPrimPath = instancerMapIt.first;
        const vector<InstPrimEntry>& primArray = instancerMapIt.second;

        size_t nprims = primArray.size();
        DBG( cerr << "Create point instancers for \"" << instancerPrimPath 
                              << "\" with " << nprims << " entries" << endl);

        GT_AttributeListHandle pAttrs = new GT_AttributeList( new GT_AttributeMap() );

        // Allocate storage for all the attributes we want to copy.
        
        // Assume all entries in the primArray have the same set of attributes.
        // (They all came from the same detail). 
        const GT_PrimitiveHandle& prim = primArray[0].prim;

        auto instPtAttrs = prim->getPointAttributes();
        GT_Real32Array*     pivotArray = nullptr;

        if( instPtAttrs ) {

            for( size_t j = 0; j < instPtAttrs->entries(); ++j ) {

                // Filter attributes that begin with an underscore
                const char *n = instPtAttrs->getName(j);
                if( !n || strlen( n ) < 1 || n[0] == '_' ) {
                    continue;
                }
                GT_Storage storage = instPtAttrs->get( j )->getStorage();
                GT_Size tupleSize  = instPtAttrs->get( j )->getTupleSize();
                GT_Type typeInfo   = instPtAttrs->get( j )->getTypeInfo();
                pAttrs = pAttrs->addAttribute( 
                            n, 
                            newDataArray( storage, nprims, tupleSize, typeInfo ), 
                            true );
            }
        }
        bool hasInstanceIndices = false;
        if(auto packedUSD = dynamic_cast<const GusdGT_PackedUSD*>( prim.get() )) {
            if (packedUSD->getInstanceIndex() >= 0) {
                hasInstanceIndices = true;
            }
        }

        if( auto instUniAttrs = prim->getUniformAttributes() ) {

            for( size_t j = 0; j < instUniAttrs->entries(); ++j ) {

                // Filter out attributes that begin with an underscore and
                // usdprimpath (usdprimpath on instances will confuse the 
                // instancerWrapper).

                const char *n = instUniAttrs->getName(j);
                if( !n || strlen( n ) < 1 || n[0] == '_' || 
                    string( n ) == GUSD_PRIMPATH_ATTR ) {
                    continue;
                }
                if( !pAttrs->hasName( n ) ) {

                    GT_Storage storage = instUniAttrs->get( j )->getStorage();
                    GT_Size tupleSize  = instUniAttrs->get( j )->getTupleSize();
                    GT_Type typeInfo   = instUniAttrs->get( j )->getTypeInfo();
                    pAttrs = pAttrs->addAttribute( 
                                n, 
                                newDataArray( storage, nprims, tupleSize, typeInfo ),
                                true );
                }
            }
        }
       

        // Allocate xform attribute used to communicate about the instances
        // with the instancerWrapper.
        GT_Real64Array*     xformArray = new GT_Real64Array(nprims, 16);
        bool foundValidTransform = false;
        GT_Int64Array* instanceIndices = hasInstanceIndices ? new GT_Int64Array(nprims, 1) : NULL;

        for( size_t primIndex = 0; primIndex < nprims; ++primIndex ) {

            const GT_PrimitiveHandle& prim = primArray[primIndex].prim;

            // copy point attribute data from the src prims into prims for 
            // the point instancer.
            auto instPtAttrs = prim->getPointAttributes();
            if( instPtAttrs ) {

                for( size_t attrIndex = 0; attrIndex < instPtAttrs->entries(); ++attrIndex ) {

                    const char *n = instPtAttrs->getName(attrIndex);
                    if( !n || strlen( n ) < 1 || n[0] == '_' ) {
                        continue;
                    }
                    
                    auto srcData = instPtAttrs->get( attrIndex );
                    if( auto dstData = pAttrs->get( n ) ) {
                        copyDataArrayItem( dstData, srcData, primIndex, primArray[primIndex].index );
                    }
                }
                if( pivotArray ) {
                    if( auto pos = instPtAttrs->get( "P" ) ) {
                        pivotArray->set( pos->getF32( 0, 0 ), primIndex, 0 );
                        pivotArray->set( pos->getF32( 0, 1 ), primIndex, 1 );
                        pivotArray->set( pos->getF32( 0, 2 ), primIndex, 2 );
                    }
                }
                if (hasInstanceIndices) {
                    if(auto packedUSD = dynamic_cast<const GusdGT_PackedUSD*>( prim.get() )) {
                        exint index = packedUSD->getInstanceIndex();
                        if (index >= 0) {
                            instanceIndices->setTuple(&index, primIndex);
                        }
                    }
                }
            }

            // copy uniform attribute data from the src prims into prims for 
            // the point instancer.
            auto instUniAttrs = prim->getUniformAttributes();
            if( instUniAttrs ) {

                for( size_t attrIndex = 0; attrIndex < instUniAttrs->entries(); ++attrIndex ) {

                    const char *n = instUniAttrs->getName(attrIndex);
                    if( !n || strlen( n ) < 1 || n[0] == '_' || string(n) == GUSD_PRIMPATH_ATTR ) {
                        continue;
                    }
                    
                    auto srcData = instUniAttrs->get( attrIndex );
                    if( auto dstData = pAttrs->get( n ) ) {
                        copyDataArrayItem( dstData, srcData, primIndex, primArray[primIndex].index );
                    }
                }
            }

            // For USD packed prims or geometry packed prims with a usdprimpath 
            // attributes, get the transforms and stuff them into arrays that
            // can be passed as attributes to the instancerWrapper.
            if( auto packedUSD = dynamic_cast<const GusdGT_PackedUSD *>( prim.get() )) {

                const SdfPath primpath(packedUSD->getPrimPath());

                UT_Matrix4D xform;
                packedUSD->getPrimitiveTransform()->getMatrix(xform);

                xformArray->setTuple(xform.data(), primIndex );

                foundValidTransform = true;
            }
            else if( auto instance = dynamic_cast<const GT_PrimInstance *>( prim.get() )) {

                UT_Matrix4D xform;
                instance->transforms()->get(primArray[primIndex].index)->getMatrix( xform );
                xformArray->setTuple(xform.data(), primIndex );
                foundValidTransform = true;
            }
        }

        if( foundValidTransform ) {
            pAttrs = pAttrs->addAttribute( "__instancetransform", xformArray, true );
        }

        if ( hasInstanceIndices ) {
            pAttrs = pAttrs->addAttribute( "__instanceindex", instanceIndices, true );
        }

        // If the instance prims have a "srcPrimPath" intrinsic (typically 
        // because we are doing an overlay), set the "usdprimpath" attribute on 
        // the point mesh prim so that the point instancer prim gets named properly.
        GT_AttributeListHandle uniformAttrs;

        if( !instancerPrimPath.IsEmpty() ) {
            uniformAttrs = new GT_AttributeList( new GT_AttributeMap() );
            auto primPathArray = new GT_DAIndexedString(1);
            primPathArray->setString( 0, 0, instancerPrimPath.GetText() );
            uniformAttrs = uniformAttrs->addAttribute( GUSD_PRIMPATH_ATTR, primPathArray, true );
        }

        // Check for the usdprototypespath attribute in case it is not
        // a point or primitivie attribute.
        uniformAttrs = findAndAddStringAttribute(uniformAttrs, "usdprototypespath", prim);
        
        // Find and add a custom prototype scope attribute.
        uniformAttrs = findAndAddStringAttribute(uniformAttrs, "usdprototypesscope", prim);

        // Add the refined point instancer. If we are overlaying an old point
        // instancer make sure to use the old type (temporary).
        if (refiner.m_pointInstancerType == _tokens->PxPointInstancer) {
            refiner.addPrimitive( new GusdGT_OldPointInstancer( pAttrs, uniformAttrs ) );
        } else {
            refiner.addPrimitive( new GusdGT_PointInstancer( pAttrs, uniformAttrs ) );
        }
    }
}

void 
GusdRefinerCollector::addInstPrim( const SdfPath &path, GT_PrimitiveHandle p, int index )
{
    // When we are building point instancers, the refiner collects prims 
    // that can be instances until finish is called.
    //
    // GT_PrimInstance prims contain more than one instance of a prototype.
    // Each instance in a entry in the m_instancePrims array and has an 
    // index into the GT_PrimInstance.

    DBG(cerr << "addInstPrim " << path << endl);

    auto instPrimsIt = m_instancePrims.find( path );
    if( instPrimsIt == m_instancePrims.end() ) {
        m_instancePrims[ path ] = 
            vector<InstPrimEntry>( 1, InstPrimEntry( p, index ));
    }
    else {
        instPrimsIt->second.push_back( InstPrimEntry( p, index ));
    }
}

namespace {

GT_DataArrayHandle
newDataArray( GT_Storage storage, GT_Size size, int tupleSize,
              GT_Type typeInfo=GT_TYPE_NONE )
{
    if( storage == GT_STORE_REAL32 ) {
        return new GT_Real32Array( size, tupleSize, typeInfo );
    }
    else if( storage == GT_STORE_REAL16 ) {
        return new GT_Real16Array( size, tupleSize, typeInfo );
    }
    else if( storage == GT_STORE_REAL64 ) {
        return new GT_Real64Array( size, tupleSize, typeInfo );
    }
    else if( storage == GT_STORE_UINT8 ) {
        return new GT_UInt8Array( size, tupleSize, typeInfo );
    }
#if SYS_VERSION_FULL_INT >= 0x11000000
    else if( storage == GT_STORE_INT8) {
        return new GT_Int8Array( size, tupleSize, typeInfo );
    }
    else if( storage == GT_STORE_INT16) {
        return new GT_Int16Array( size, tupleSize, typeInfo );
    }
#endif
    else if( storage == GT_STORE_INT32 ) {
        return new GT_Int32Array( size, tupleSize, typeInfo );
    }
    else if( storage == GT_STORE_INT64 ) {
        return new GT_Int64Array( size, tupleSize, typeInfo );
    }
    else if( storage == GT_STORE_STRING ) {
        return new GT_DAIndexedString( size, tupleSize );
    }
    return NULL;
}

void
copyDataArrayItem( GT_DataArrayHandle dstData, GT_DataArrayHandle srcData, 
                   GT_Offset dstOffset, GT_Offset srcOffset )
{
    // copy a scalar data item into the destination array at the given offset.
    GT_Storage storage = dstData->getStorage();
    if( storage == GT_STORE_REAL32 ) {
        for( int i = 0; i < dstData->getTupleSize(); ++i ) {
            auto dst = UTverify_cast<GT_Real32Array*>(dstData.get());
            dst->set( srcData->getF32( srcOffset, i ), dstOffset, i );
        }
    }
    else if( storage == GT_STORE_REAL64 ) {
        for( int i = 0; i < dstData->getTupleSize(); ++i ) {
            auto dst = UTverify_cast<GT_Real64Array*>(dstData.get());
            dst->set( srcData->getF64( srcOffset, i ), dstOffset, i );
        }
    }
    else if( storage == GT_STORE_INT32 ) {
        for( int i = 0; i < dstData->getTupleSize(); ++i ) {
            auto dst = UTverify_cast<GT_Int32Array*>(dstData.get());
            dst->set( srcData->getI32( srcOffset, i ), dstOffset, i );
        }
    }
    else if( storage == GT_STORE_INT64 ) {
        for( int i = 0; i < dstData->getTupleSize(); ++i ) {
            auto dst = UTverify_cast<GT_Int64Array*>(dstData.get());
            dst->set( srcData->getI64( srcOffset, i ), dstOffset, i );
        }
    }
    else if( storage == GT_STORE_STRING ) {
        for( int i = 0; i < dstData->getTupleSize(); ++i ) {
            auto dst = UTverify_cast<GT_DAIndexedString*>(dstData.get());
            dst->setString( dstOffset, i, srcData->getS( srcOffset, i ) );
        }
    }
}

GT_AttributeListHandle
findAndAddStringAttribute( GT_AttributeListHandle attrs,
                   const std::string& attrName,
                   const GT_PrimitiveHandle& gtPrim) {
    // find a string attribute on the prim and add it to the attribute list.
    GT_Owner owner;
    if(auto attrib = gtPrim->findAttribute(attrName.c_str(), owner, 0)){
        if (attrib->isValid()){
            if (!attrs) {
                attrs = new GT_AttributeList( new GT_AttributeMap() );
            }
            auto array = new GT_DAIndexedString(1);
            array->setString( 0, 0, attrib->getS(0) );
            attrs = attrs->addAttribute( attrName.c_str(), array, true );
        }
    }
    return attrs;
}

} /* close namespace */

PXR_NAMESPACE_CLOSE_SCOPE
