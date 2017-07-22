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
#include "meshWrapper.h"

#include "context.h"
#include "UT_Gf.h"
#include "GU_USD.h"
#include "GT_VtArray.h"
#include "GT_VtStringArray.h"
#include "USD_XformCache.h"

#include <GT/GT_DAConstantValue.h>
#include <GT/GT_DANumeric.h>
#include <GT/GT_PrimPolygonMesh.h>
#include <GT/GT_PrimSubdivisionMesh.h>
#include <GT/GT_RefineParms.h>
#include <GT/GT_Refine.h>
#include <GT/GT_DAIndexedString.h>
#include <GT/GT_TransformArray.h>
#include <GT/GT_PrimInstance.h>
#include <GT/GT_DAIndirect.h>
#include <GT/GT_DASubArray.h>
#include <GT/GT_GEOPrimPacked.h>
#include <GT/GT_DAConstant.h>

PXR_NAMESPACE_OPEN_SCOPE

using std::cerr;
using std::endl;
using std::string;
using std::map;

#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif

namespace {

void _reverseWindingOrder(GT_Int32Array* indices,
                          GT_DataArrayHandle faceCounts)
{
    GT_DataArrayHandle buffer;
    int* indicesData = indices->data();
    const int32 *faceCountsData = faceCounts->getI32Array( buffer );
    size_t base = 0;
    for( size_t f = 0; f < faceCounts->entries(); ++f ) {
        int32 numVerts = faceCountsData[f];
        for( size_t p = 1, e = (numVerts + 1) / 2; p < e; ++p ) {
            std::swap( indicesData[base+p], indicesData[base+numVerts-p] );
        }
        base+= numVerts;
    }
}

void _validateAttrData(
        const char*             destName, // The Houdni name of the attribute
        const char*             srcName,  // The USD name of the attribute
        const char*             primName, 
        GT_DataArrayHandle      data, 
        const TfToken&          interpolation,
        int                     numFaces,
        int                     numPoints,
        int                     numVerticies,
        GT_AttributeListHandle* vertexAttrs,
        GT_AttributeListHandle* pointAttrs,
        GT_AttributeListHandle* uniformAttrs,
        GT_AttributeListHandle* detailAttrs );

} // anon namespace

GusdMeshWrapper::GusdMeshWrapper(
        const GT_PrimitiveHandle& sourcePrim,
        const UsdStagePtr& stage,
        const SdfPath& path,
        const GusdContext& ctxt,
        bool isOverride )
{
    initUsdPrim( stage, path, isOverride );
    initialize( ctxt, sourcePrim );
}

GusdMeshWrapper::GusdMeshWrapper( 
        const GusdUSD_StageProxyHandle& stage, 
        const UsdGeomMesh& mesh,
        const UsdTimeCode& time,
        const GusdPurposeSet& purposes )
    : GusdPrimWrapper( time, purposes )
    , m_usdMeshForRead( mesh, stage->GetLock() )
{
}    

GusdMeshWrapper::GusdMeshWrapper( const GusdMeshWrapper &in )
    : GusdPrimWrapper( in )
    , m_usdMeshForRead( in.m_usdMeshForRead )
    , m_usdMeshForWrite( in.m_usdMeshForWrite )
{
}

GusdMeshWrapper::
~GusdMeshWrapper()
{
}

bool GusdMeshWrapper::
initUsdPrim(const UsdStagePtr& stage,
            const SdfPath& path,
            bool asOverride)
{
    bool newPrim = true;
    if( asOverride ) {
        UsdPrim existing = stage->GetPrimAtPath( path );
        if( existing ) {
            newPrim = false;
            m_usdMeshForWrite = UsdGeomMesh(stage->OverridePrim( path ));
        }
        else {
            // When fracturing, we want to override the outside surfaces and create
            // new inside surfaces in one export. So if we don't find an existing prim
            // with the given path, create a new one.
            m_usdMeshForWrite = UsdGeomMesh::Define( stage, path );
        }
    }
    else {
        m_usdMeshForWrite = UsdGeomMesh::Define( stage, path );  
    }
    if( !m_usdMeshForWrite || !m_usdMeshForWrite.GetPrim().IsValid() ) {
        TF_WARN( "Unable to create %s mesh '%s'.", newPrim ? "new" : "override", path.GetText() );
    }
    return bool( m_usdMeshForWrite );
}

GT_PrimitiveHandle GusdMeshWrapper::
defineForWrite(
    const GT_PrimitiveHandle& sourcePrim,
    const UsdStagePtr& stage,
    const SdfPath& path,
    const GusdContext& ctxt )
{
    return new GusdMeshWrapper(
                            sourcePrim,
                            stage,
                            path,
                            ctxt,
                            ctxt.getOverGeo( sourcePrim ));
}

GT_PrimitiveHandle GusdMeshWrapper::
defineForRead( const GusdUSD_StageProxyHandle&  stage,
               const UsdGeomImageable&          sourcePrim, 
               const UsdTimeCode&               time,
               const GusdPurposeSet&            purposes )
{
    return new GusdMeshWrapper( 
                    stage, 
                    UsdGeomMesh( sourcePrim.GetPrim() ),
                    time,
                    purposes );
}

bool GusdMeshWrapper::
redefine( const UsdStagePtr& stage,
          const SdfPath& path,
          const GusdContext& ctxt,
          const GT_PrimitiveHandle& sourcePrim )
{
    initUsdPrim( stage, path, ctxt.getOverGeo( sourcePrim ));
    clearCaches();

    initialize( ctxt, sourcePrim );

    return true;
}

void GusdMeshWrapper::
initialize( const GusdContext& ctxt,
            const GT_PrimitiveHandle& sourcePrim )
{
    bool overlayTransforms = ctxt.getOverTransforms( sourcePrim );
    bool overlayPoints =     ctxt.getOverPoints( sourcePrim );
    bool overlayPrimvars =   ctxt.getOverPrimvars( sourcePrim );
    bool overlayAll =        ctxt.getOverAll( sourcePrim );
    bool writeNewGeo = !(overlayTransforms || overlayPoints || overlayPrimvars || overlayAll);

         // Set defaults from source prim if one was passed in
    if((writeNewGeo || overlayAll) && isValid() && sourcePrim) {
        
        UsdAttribute usdAttr;

        // Orientation
        usdAttr = m_usdMeshForWrite.GetOrientationAttr();
        if( usdAttr ) {

            // Houdini uses left handed winding order for mesh vertices.
            // USD can handle either right or left. If we are overlaying
            // geo, we have to reverse the vertex order to match the original, 
            // otherwise just write the left handled verts directly.
            TfToken orientation;
            usdAttr.Get(&orientation, UsdTimeCode::Default());
            if( writeNewGeo && orientation == UsdGeomTokens->rightHanded ) {
                usdAttr.Set(UsdGeomTokens->leftHanded, UsdTimeCode::Default());
            }
        }

        // SubdivisionScheme
        TfToken subdScheme = UsdGeomTokens->none;
        if(const GT_PrimSubdivisionMesh* mesh
            = dynamic_cast<const GT_PrimSubdivisionMesh*>(sourcePrim.get())) {
            if(GT_CATMULL_CLARK == mesh->scheme())
                        subdScheme = UsdGeomTokens->catmullClark;
            else if(GT_LOOP == mesh->scheme())
                subdScheme = UsdGeomTokens->loop;
        }
        setSubdivisionScheme(subdScheme);
    }
}

const UsdGeomImageable 
GusdMeshWrapper::getUsdPrimForRead(
    GusdUSD_ImageableHolder::ScopedLock &lock) const
{
    // obtain first lock to get geomtry as UsdGeomMesh.
    GusdUSD_MeshHolder::ScopedReadLock innerLock;
    innerLock.Acquire( m_usdMeshForRead );

    // Build new holder after casting to imageable
    GusdUSD_ImageableHolder tmp( UsdGeomImageable( (*innerLock).GetPrim() ),
                                 m_usdMeshForRead.GetLock() );
    lock.Acquire(tmp, /*write*/false);
    return *lock;
}

bool 
GusdMeshWrapper::refine(
    GT_Refine& refiner, 
    const GT_RefineParms* parms) const
{
    if(!isValid()) {
        TF_WARN( "Invalid prim" );
        return false;
    }

    bool refineForViewport = GT_GEOPrimPacked::useViewportLOD(parms);

    GusdUSD_MeshHolder::ScopedReadLock lock;
    lock.Acquire(m_usdMeshForRead);
    UsdGeomMesh usdMesh = *lock;

    DBG(cerr << "GusdMeshWrapper::refine, " << usdMesh.GetPrim().GetPath() << endl);
    VtFloatArray vtFloatArray;
    VtIntArray   vtIntArray;
    VtVec3fArray vtVec3Array;

    // Houdini only supports left handed geometry. Right handed polys need to 
    // be reversed on import.
    TfToken orientation;
    bool reverseWindingOrder =
           usdMesh.GetOrientationAttr().Get(&orientation, m_time)
        && orientation == UsdGeomTokens->rightHanded;

    // vertex counts
    UsdAttribute countsAttr = usdMesh.GetFaceVertexCountsAttr();
    if(!countsAttr) {
        TF_WARN( "Invalid vertex count attribute" );
        return false;
    }

    VtIntArray usdCounts;
    countsAttr.Get(&usdCounts, m_time);
    if( usdCounts.size() < 1 ) {
        return false;
    }
    GT_DataArrayHandle gtVertexCounts = new GusdGT_VtArray<int32>( usdCounts );
    int numVerticiesExpected = std::accumulate( usdCounts.begin(), usdCounts.end(), 0 );

    // vertex indices
    UsdAttribute faceIndexAttr = usdMesh.GetFaceVertexIndicesAttr();
    if(!faceIndexAttr) {
        TF_WARN( "Invalid face vertex indicies attribute for %s.",
                 usdMesh.GetPrim().GetPath().GetText());
        return false;
    }
    VtIntArray usdFaceIndex;
    faceIndexAttr.Get(&usdFaceIndex, m_time);
    if( usdFaceIndex.size() < numVerticiesExpected ) {
        TF_WARN( "Invalid topology found for %s. "
                 "Expected at least %d verticies and only got %zd.",
                 usdMesh.GetPrim().GetPath().GetText(), numVerticiesExpected, usdFaceIndex.size() );
        return false;
    }

    GT_DataArrayHandle gtIndicesHandle;
    if( reverseWindingOrder ) {
        // Make a copy and reorder
        gtIndicesHandle = new GT_Int32Array(usdFaceIndex.cdata(),
                            usdFaceIndex.size(), 1);
        _reverseWindingOrder(UTverify_cast<GT_Int32Array*>(gtIndicesHandle.get()),
                             gtVertexCounts);
    }
    else {
        gtIndicesHandle = new GusdGT_VtArray<int32>( usdFaceIndex );
    }

    // point positions
    UsdAttribute pointsAttr = usdMesh.GetPointsAttr();
    if(!pointsAttr) {
        TF_WARN( "Invalid point attribute" );
        return false;
    }
    VtVec3fArray usdPoints;
    pointsAttr.Get(&usdPoints, m_time);
    int maxPointIndex = *std::max_element( usdFaceIndex.begin(), usdFaceIndex.end() ) + 1;
    if( usdPoints.size() < maxPointIndex ) {
        TF_WARN( "Invalid topology found for %s. "
                 "Expected at least %d points and only got %zd.",
                 usdMesh.GetPrim().GetPath().GetText(), maxPointIndex, usdPoints.size() ); 
        return false;
    }

    auto gtPoints = new GusdGT_VtArray<GfVec3f>(usdPoints,GT_TYPE_POINT);

    GT_AttributeListHandle gtPointAttrs = new GT_AttributeList( new GT_AttributeMap() );
    GT_AttributeListHandle gtVertexAttrs = new GT_AttributeList( new GT_AttributeMap() );
    GT_AttributeListHandle gtUniformAttrs = new GT_AttributeList( new GT_AttributeMap() );
    GT_AttributeListHandle gtDetailAttrs = new GT_AttributeList( new GT_AttributeMap() );

    gtPointAttrs = gtPointAttrs->addAttribute("P", gtPoints, true);

    UsdAttribute normalsAttr = usdMesh.GetNormalsAttr();
    if( normalsAttr && normalsAttr.HasAuthoredValueOpinion()) {
        normalsAttr.Get(&vtVec3Array, m_time);
        GT_DataArrayHandle gtNormals = 
                new GusdGT_VtArray<GfVec3f>(vtVec3Array, GT_TYPE_NORMAL);
        gtPointAttrs = gtPointAttrs->addAttribute("N", gtNormals, true);
    }

    if( !refineForViewport ) {

        // point velocities
        UsdAttribute velAttr = usdMesh.GetVelocitiesAttr();
        if (velAttr && velAttr.HasAuthoredValueOpinion()) {
            velAttr.Get(&vtVec3Array, m_time);
            GT_DataArrayHandle gtVel = 
                    new GusdGT_VtArray<GfVec3f>(vtVec3Array, GT_TYPE_VECTOR);
            gtPointAttrs = gtPointAttrs->addAttribute("v", gtVel, true);
        }
            
        loadPrimvars( m_time, parms, 
                      usdCounts.size(), 
                      usdPoints.size(),
                      usdFaceIndex.size(),
                      usdMesh.GetPath().GetString(),
                      &gtVertexAttrs,
                      &gtPointAttrs,
                      &gtUniformAttrs,
                      &gtDetailAttrs );

        if( gtVertexAttrs->entries() > 0 ) {
            if( reverseWindingOrder ) {
                // Construct an index array which will be used to lookup vertex 
                // attributes in the correct order.
                GT_Int32Array* vertexIndirect
                    = new GT_Int32Array(gtIndicesHandle->entries(), 1);
                GT_DataArrayHandle vertexIndirectHandle(vertexIndirect);
                for(int i=0; i<gtIndicesHandle->entries(); ++i) {
                    vertexIndirect->set(i, i);     
                }
                _reverseWindingOrder(vertexIndirect, gtVertexCounts );

                gtVertexAttrs = gtVertexAttrs->createIndirect(vertexIndirect);
            }
        }
    }

    else {

        // When refining for the viewport, the only attribute we care about is color.
        // Prefer Cd, but fallback to display_color.
        // In order to be able to coalesce meshes in the GT_PrimCache, we need to use
        // the same attribute owner for the attribute in all meshes. So promote 
        // to vertex.

        UsdGeomPrimvar colorPrimvar = usdMesh.GetPrimvar(TfToken("Cd"));
        if( !colorPrimvar || !colorPrimvar.GetAttr().HasAuthoredValueOpinion() ) {
            colorPrimvar = usdMesh.GetPrimvar(TfToken("displayColor"));
        }

        if( colorPrimvar && colorPrimvar.GetAttr().HasAuthoredValueOpinion()) {

            GT_DataArrayHandle gtData = convertPrimvarData( colorPrimvar, m_time );
            if( gtData ) {

                _validateAttrData(
                    "Cd",
                    colorPrimvar.GetBaseName().GetText(),
                    usdMesh.GetPrim().GetPath().GetText(),
                    gtData,
                    colorPrimvar.GetInterpolation(),
                    usdCounts.size(),
                    usdPoints.size(),
                    usdFaceIndex.size(),
                    &gtVertexAttrs,
                    &gtPointAttrs,
                    &gtUniformAttrs,
                    &gtDetailAttrs );
            }
        }
    }

    // build GT_Primitive
    TfToken subdScheme;
    usdMesh.GetSubdivisionSchemeAttr().Get(&subdScheme, m_time);
    bool isSubdMesh = (UsdGeomTokens->none != subdScheme);

    GT_PrimitiveHandle meshPrim;
    if(isSubdMesh) {
        GT_PrimSubdivisionMesh *subdPrim =
            new GT_PrimSubdivisionMesh(gtVertexCounts,
                                       gtIndicesHandle,
                                       gtPointAttrs,
                                       gtVertexAttrs,
                                       gtUniformAttrs,
                                       gtDetailAttrs);
        meshPrim = subdPrim;

        // See the houdini distribution's alembic importer for examples
        // of how to use these tags:
        //     ./HoudiniAlembic/GABC/GABC_IObject.C
        // inside HFS/houdini/public/Alembic/HoudiniAlembic.tgz

        // Scheme
        if (subdScheme == UsdGeomTokens->catmullClark) {
            subdPrim->setScheme(GT_CATMULL_CLARK);
        } else if (subdScheme == UsdGeomTokens->loop) {
            subdPrim->setScheme(GT_LOOP);
        } else {
            // Other values, like bilinear, have no equivalent in houdini.
        }

        // Corners
        UsdAttribute cornerIndicesAttr   = usdMesh.GetCornerIndicesAttr();
        UsdAttribute cornerSharpnessAttr = usdMesh.GetCornerSharpnessesAttr();
        if (cornerIndicesAttr.IsValid() && cornerSharpnessAttr.IsValid()) {
            cornerIndicesAttr.Get(&vtIntArray, m_time);
            cornerSharpnessAttr.Get(&vtFloatArray, m_time);
            if(!vtIntArray.empty() && !vtFloatArray.empty()) {
                GT_DataArrayHandle cornerArrayHandle
                    = new GT_Int32Array(vtIntArray.data(), vtIntArray.size(), 1);
                subdPrim->appendIntTag("corner", cornerArrayHandle);

                GT_DataArrayHandle corenerWeightArrayHandle
                    = new GT_Real32Array(vtFloatArray.data(), vtFloatArray.size(), 1);
                subdPrim->appendRealTag("corner", corenerWeightArrayHandle);
            }
        }

        // Creases
        UsdAttribute creaseIndicesAttr = usdMesh.GetCreaseIndicesAttr();
        UsdAttribute creaseLengthsAttr = usdMesh.GetCreaseLengthsAttr();
        UsdAttribute creaseSharpnessesAttr = usdMesh.GetCreaseSharpnessesAttr();
        if (creaseIndicesAttr.IsValid() &&
            creaseLengthsAttr.IsValid() &&
            creaseSharpnessesAttr.IsValid() &&
            creaseIndicesAttr.HasAuthoredValueOpinion()) {
            // Extract vt arrays
            VtIntArray vtCreaseIndices;
            VtIntArray vtCreaseLengths;
            VtFloatArray vtCreaseSharpnesses;
            creaseIndicesAttr.Get(&vtCreaseIndices, m_time);
            creaseLengthsAttr.Get(&vtCreaseLengths, m_time);
            creaseSharpnessesAttr.Get(&vtCreaseSharpnesses, m_time);

            // Unpack creases to vertex-pairs.
            // Usd stores creases as N-length chains of vertices;
            // Houdini expects separate creases per vertex pair.
            std::vector<int> creaseIndices;
            std::vector<float> creaseSharpness;
            if (vtCreaseLengths.size() == vtCreaseSharpnesses.size()) {
                // We have exactly 1 sharpness per crease.
                size_t i=0;
                for (size_t creaseNum=0; creaseNum < vtCreaseLengths.size();
                     ++creaseNum) {
                    const float length = vtCreaseLengths[creaseNum];
                    const float sharp = vtCreaseSharpnesses[creaseNum];
                    for (size_t indexInCrease=0; indexInCrease < length-1;
                         ++indexInCrease) {
                        creaseIndices.push_back( vtCreaseIndices[i] );
                        creaseIndices.push_back( vtCreaseIndices[i+1] );
                        creaseSharpness.push_back( sharp );
                        ++i;
                    }
                    // Last index is only used once.
                    ++i;
                }
                UT_ASSERT(i == vtCreaseIndices.size());
            } else {
                // We have N-1 sharpnesses for each crease that has N edges,
                // i.e. the sharpness varies along each crease.
                size_t i=0;
                size_t sharpIndex=0;
                for (size_t creaseNum=0; creaseNum < vtCreaseLengths.size();
                     ++creaseNum) {
                    const float length = vtCreaseLengths[creaseNum];
                    for (size_t indexInCrease=0; indexInCrease < length-1;
                         ++indexInCrease) {
                        creaseIndices.push_back( vtCreaseIndices[i] );
                        creaseIndices.push_back( vtCreaseIndices[i+1] );
                        const float sharp = vtCreaseSharpnesses[sharpIndex];
                        creaseSharpness.push_back(sharp);
                        ++i;
                        ++sharpIndex;
                    }
                    // Last index is only used once.
                    ++i;
                }
                UT_ASSERT(i == vtCreaseIndices.size());
                UT_ASSERT(sharpIndex == vtCreaseSharpnesses.size());
            }

            // Store tag.
            GT_Int32Array *index =
                new GT_Int32Array( &creaseIndices[0],
                                   creaseIndices.size(), 1);
            GT_Real32Array *weight =
                new GT_Real32Array( &creaseSharpness[0],
                                    creaseSharpness.size(), 1);
            UT_ASSERT(index->entries() == weight->entries()*2);
            subdPrim->appendIntTag("crease", GT_DataArrayHandle(index));
            subdPrim->appendRealTag("crease", GT_DataArrayHandle(weight));
        }

        // Interpolation boundaries
        UsdAttribute interpBoundaryAttr = usdMesh.GetInterpolateBoundaryAttr();
        if(interpBoundaryAttr.IsValid()) {
            TfToken val;
            interpBoundaryAttr.Get(&val, m_time);
            GT_DataArrayHandle interpBoundaryHandle = new GT_IntConstant(1, 1);
            subdPrim->appendIntTag("interpolateboundary", interpBoundaryHandle);
        }
        //if (ival = sample.getFaceVaryingInterpolateBoundary())
        //{
            //GT_IntConstant    *val = new GT_IntConstant(1, ival);
            //gt->appendIntTag("facevaryinginterpolateboundary",
                //GT_DataArrayHandle(val));
        //}
        //if (ival = sample.getFaceVaryingPropagateCorners())
        //{
            //GT_IntConstant    *val = new GT_IntConstant(1, ival);
            //gt->appendIntTag("facevaryingpropagatecorners",
                //GT_DataArrayHandle(val));
        //}
    }
    else {
        meshPrim
            = new GT_PrimPolygonMesh(gtVertexCounts,
                                     gtIndicesHandle,
                                     gtPointAttrs,
                                     gtVertexAttrs,
                                     gtUniformAttrs,
                                     gtDetailAttrs);
    }
    meshPrim->setPrimitiveTransform( getPrimitiveTransform() );
    refiner.addPrimitive( meshPrim );
    return true;
}

namespace {

void
_validateAttrData( 
        const char*             destName, // The Houdni name of the attribute
        const char*             srcName,  // The USD name of the attribute
        const char*             primName, 
        GT_DataArrayHandle      data, 
        const TfToken&          interpolation,
        int                     numFaces,
        int                     numPoints,
        int                     numVerticies,
        GT_AttributeListHandle* vertexAttrs,
        GT_AttributeListHandle* pointAttrs,
        GT_AttributeListHandle* uniformAttrs,
        GT_AttributeListHandle* detailAttrs )
{
    if( interpolation == UsdGeomTokens->vertex || 
        interpolation == UsdGeomTokens->varying ) {

        if( data->entries() < numPoints ) {
            TF_WARN( "Not enough values found for attribute: %s:%s",
                     primName, srcName );
        }
        else {
            *pointAttrs = (*pointAttrs)->addAttribute( destName, data, true );
        }
    }
    else if( interpolation == UsdGeomTokens->faceVarying ) {

        if( data->entries() < numVerticies ) {
            TF_WARN( "Not enough values found for attribute: %s:%s",
                    primName, srcName );  
        }
        else {
            *vertexAttrs =  (*vertexAttrs)->addAttribute( destName, data, true );
        }
    }                 
    else if( interpolation == UsdGeomTokens->uniform ) {
        if( data->entries() < numFaces ) {
            TF_WARN( "Not enough values found for attribute: %s:%s",
                    primName, srcName );  
        }                    
        *uniformAttrs = (*uniformAttrs)->addAttribute( destName, data, true );
    }
    else if( interpolation == UsdGeomTokens->constant ) {
       if( data->entries() < 1 ) {
            TF_WARN( "Not enough values found for attribute: %s:%s",
                    primName, srcName );  
        } 
        *detailAttrs = (*detailAttrs)->addAttribute( destName, data, true );
    }
}
}

//------------------------------------------------------------------------------


bool GusdMeshWrapper::
setSubdivisionScheme(const TfToken& scheme)
{
    if(!m_usdMeshForWrite) {
        return false;
    }

    UsdAttribute usdAttr = m_usdMeshForWrite.GetSubdivisionSchemeAttr();
    if(!usdAttr)   return false;

    return usdAttr.Set(scheme, UsdTimeCode::Default());
}


TfToken GusdMeshWrapper::
getSubdivisionScheme() const
{
    TfToken scheme;
    if(m_usdMeshForWrite) {

        m_usdMeshForWrite.GetSubdivisionSchemeAttr().Get(&scheme, UsdTimeCode::Default());
    }
    return scheme;
}


bool GusdMeshWrapper::
getUniqueID(int64& id) const
{
    static const int s_id = GT_Primitive::createPrimitiveTypeId();
    id = s_id;
    return true;
}


const char* GusdMeshWrapper::
className() const
{
    return "GusdMeshWrapper";
}


void GusdMeshWrapper::
enlargeBounds(UT_BoundingBox boxes[], int nsegments) const
{
    // TODO
}


int GusdMeshWrapper::
getMotionSegments() const
{
    // TODO
    return 1;
}


int64 GusdMeshWrapper::
getMemoryUsage() const
{
    cerr << "mesh wrapper, get memory usage" << endl;
    return 0;
}


GT_PrimitiveHandle GusdMeshWrapper::
doSoftCopy() const
{
    // TODO
    return GT_PrimitiveHandle(new GusdMeshWrapper( *this ));
}


bool GusdMeshWrapper::isValid() const
{
    return m_usdMeshForRead || m_usdMeshForWrite;
}

bool GusdMeshWrapper::
updateFromGTPrim(const GT_PrimitiveHandle& sourcePrim,
                 const UT_Matrix4D&        houXform,
                 const GusdContext&        ctxt,
                 GusdSimpleXformCache&     xformCache )
{
    if(!isValid()) {
        TF_WARN( "Can't update USD mesh from GT prim '%s'", m_usdMeshForWrite.GetPrim().GetPath().GetText() );
        return false;
    }

    const GT_PrimPolygonMesh* gtMesh
        = dynamic_cast<const GT_PrimPolygonMesh*>(sourcePrim.get());
    if(!gtMesh) {
        TF_WARN( "source prim is not a mesh. '%s'", m_usdMeshForWrite.GetPrim().GetPath().GetText() );
        return false;
    }

    bool overlayTransforms = ctxt.getOverTransforms( sourcePrim );
    bool overlayPoints =     ctxt.getOverPoints( sourcePrim );
    bool overlayPrimvars =   ctxt.getOverPrimvars( sourcePrim );
    bool overlayAll =        ctxt.getOverAll( sourcePrim );

    // While I suppose we could write both points and transforms, it gets confusing,
    // and I don't this its necessary so lets not.
    if( overlayPoints || overlayAll ) {
        overlayTransforms = false;
    }
    bool writeNewGeo = !(overlayTransforms || overlayPoints || overlayPrimvars || overlayAll);

    bool reverseWindingOrder = false;
    GT_DataArrayHandle vertexIndirect;
    if( overlayPrimvars || overlayAll ) {

        // If we are writing an overlay, we need to write geometry that matches
        // the orientation of the underlying prim. All geometry in Houdini is left
        // handed.
        TfToken orientation;
        m_usdMeshForWrite.GetOrientationAttr().Get(&orientation, ctxt.time);
        if( orientation == UsdGeomTokens->rightHanded ) {
            reverseWindingOrder = true;

            // Build a LUT that will allow us to map the vertex list or
            // primvars using a "indirect" data array.
            GT_Size entries = gtMesh->getVertexList()->entries();
            GT_Int32Array* indirect = new GT_Int32Array( entries, 1 );
            for(int i=0; i<entries; ++i) {
                indirect->set(i, i);     
            }
            _reverseWindingOrder(indirect, gtMesh->getFaceCounts() );
            vertexIndirect = indirect;
        }
    }

    // houXform is a transform from world space to the space this prim's 
    // points are defined in. Compute this space relative to this prims's 
    // USD parent.

    // Compute transform not including this prims transform.
    GfMatrix4d xform = computeTransform( 
                            m_usdMeshForWrite.GetPrim().GetParent(),
                            ctxt.time,
                            houXform,
                            xformCache );

    // Compute transform including this prims transform.
    GfMatrix4d loc_xform = computeTransform( 
                            m_usdMeshForWrite.GetPrim(),
                            ctxt.time,
                            houXform,
                            xformCache );

    // If we are writing points for an overlay but not writing transforms, 
    // then we have to transform the points into the proper space.
    bool transformPoints = 
        (overlayPoints || overlayAll) && 
        !GusdUT_Gf::Cast(loc_xform).isIdentity();

    if( writeNewGeo && ctxt.purpose != UsdGeomTokens->default_ ) {
        m_usdMeshForWrite.GetPurposeAttr().Set( ctxt.purpose );
    }

    // intrinsic attributes ----------------------------------------------------

    GT_Owner attrOwner = GT_OWNER_INVALID;
    GT_DataArrayHandle houAttr;
    UsdAttribute usdAttr;
    
    if( writeNewGeo || overlayAll || overlayTransforms || overlayPoints ) {
        // extent
        houAttr = GusdGT_Utils::getExtentsArray(sourcePrim);

        usdAttr = m_usdMeshForWrite.GetExtentAttr();
        if(houAttr && usdAttr && transformPoints ) {
            houAttr = GusdGT_Utils::transformPoints( houAttr, loc_xform );
        }       
        updateAttributeFromGTPrim( GT_OWNER_INVALID, "extents", houAttr, usdAttr, ctxt.time );
    }

    // transform ---------------------------------------------------------------
    if( writeNewGeo || overlayTransforms ) {

        updateTransformFromGTPrim( xform, ctxt.time, 
                                   ctxt.granularity == GusdContext::PER_FRAME );
    }

    updateVisibilityFromGTPrim(sourcePrim, ctxt.time, 
                               (writeNewGeo || overlayAll) && 
                                ctxt.granularity == GusdContext::PER_FRAME );

    //  Points
    if( writeNewGeo || overlayAll || overlayPoints ) {
        
        // P
        houAttr = sourcePrim->findAttribute("P", attrOwner, 0);
        usdAttr = m_usdMeshForWrite.GetPointsAttr();
        if( houAttr && usdAttr && transformPoints ) {

            houAttr = GusdGT_Utils::transformPoints( houAttr, loc_xform );
        }
        updateAttributeFromGTPrim( attrOwner, "P",
                                   houAttr, usdAttr, ctxt.time );


        // N
        houAttr = sourcePrim->findAttribute("N", attrOwner, 0);
        if( houAttr && houAttr->getTupleSize() != 3 ) {
            TF_WARN( "normals (N) attribute is not a 3 vector. Tuple size = %zd.", 
                     houAttr->getTupleSize() );
        }
        usdAttr = m_usdMeshForWrite.GetNormalsAttr();
        if( updateAttributeFromGTPrim( attrOwner, "N",
                                       houAttr, usdAttr, ctxt.time ) ) {
            if(GT_OWNER_VERTEX == attrOwner) {
                m_usdMeshForWrite.SetNormalsInterpolation(UsdGeomTokens->faceVarying);
            } else {
                 m_usdMeshForWrite.SetNormalsInterpolation(UsdGeomTokens->varying);
            }
        }

        // v
        houAttr = sourcePrim->findAttribute("v", attrOwner, 0);
        if( houAttr && houAttr->getTupleSize() != 3 ) {
            TF_WARN( "velocity (v) attribute is not a 3 vector. Tuple size = %zd.", 
                     houAttr->getTupleSize() );
        }
        usdAttr = m_usdMeshForWrite.GetVelocitiesAttr();

        updateAttributeFromGTPrim( attrOwner, "v",
                                   houAttr, usdAttr, ctxt.time );
    }

    // Topology
    if( writeNewGeo || overlayAll ) {
        // FaceVertexCounts
        GT_DataArrayHandle houVertexCounts = gtMesh->getFaceCounts();
        usdAttr = m_usdMeshForWrite.GetFaceVertexCountsAttr();
        updateAttributeFromGTPrim( GT_OWNER_INVALID, "facevertexcounts",
                                   houVertexCounts, usdAttr, ctxt.time );

        // FaceVertexIndices
        GT_DataArrayHandle houVertexList = gtMesh->getVertexList();
        if( reverseWindingOrder ) {
            houVertexList = new GT_DAIndirect( vertexIndirect, houVertexList );
        }
        usdAttr = m_usdMeshForWrite.GetFaceVertexIndicesAttr();
        updateAttributeFromGTPrim( GT_OWNER_INVALID, "facevertexindices",
                                   houVertexList, usdAttr, ctxt.time );

        // Creases
        if(const GT_PrimSubdivisionMesh* subdMesh
           = dynamic_cast<const GT_PrimSubdivisionMesh*>(gtMesh)) {
            if (const GT_PrimSubdivisionMesh::Tag *tag =
                subdMesh->findTag("crease")) {
                const GT_Int32Array *index =
                    dynamic_cast<GT_Int32Array*>(tag->intArray().get());
                const GT_Real32Array *weight =
                    dynamic_cast<GT_Real32Array*>(tag->realArray().get());

                if (index && weight) {
                    // We expect two index entries per weight
                    UT_ASSERT(index->entries() == weight->entries()*2);

                    // Convert to vt arrays
                    const int numCreases = weight->entries();
                    VtIntArray vtCreaseIndices;
                    VtIntArray vtCreaseLengths;
                    VtFloatArray vtCreaseSharpnesses;
                    vtCreaseIndices.resize(numCreases*2);
                    vtCreaseLengths.resize(numCreases);
                    vtCreaseSharpnesses.resize(numCreases);
                    for (size_t i=0; i < numCreases; ++i) {
                        vtCreaseIndices[i*2+0] = index->getValue<int>(i*2+0);
                        vtCreaseIndices[i*2+1] = index->getValue<int>(i*2+1);
                        vtCreaseSharpnesses[i] = weight->getValue<float>(i);
                        // We leave creases as vertex-pairs; we do
                        // not attempt to stitch them into longer runs.
                        vtCreaseLengths[i] = 2;
                    }

                    // Set usd attributes
                    UsdAttribute creaseIndicesAttr =
                        m_usdMeshForWrite.GetCreaseIndicesAttr();
                    UsdAttribute creaseLengthsAttr =
                        m_usdMeshForWrite.GetCreaseLengthsAttr();
                    UsdAttribute creaseSharpnessesAttr =
                        m_usdMeshForWrite.GetCreaseSharpnessesAttr();
                    creaseIndicesAttr.Set(vtCreaseIndices, m_time);
                    creaseLengthsAttr.Set(vtCreaseLengths, m_time);
                    creaseSharpnessesAttr.Set(vtCreaseSharpnesses, m_time);
                }
            }
        }
    }
        
        // -------------------------------------------------------------------------

        // primvars ----------------------------------------------------------------

    if( writeNewGeo || overlayAll || overlayPrimvars ) {

        GusdGT_AttrFilter filter = ctxt.attributeFilter;
        filter.appendPattern(GT_OWNER_POINT, "^P ^N ^v ^visible ^usdactive");
        filter.appendPattern(GT_OWNER_VERTEX, "^N ^visible ^creaseweight ^usdactive");
        filter.appendPattern(GT_OWNER_UNIFORM, "^visible ^usdactive");
        filter.appendPattern(GT_OWNER_CONSTANT, "^visible ^usdactive");
        if( !ctxt.primPathAttribute.empty() ) {
            filter.appendPattern(GT_OWNER_UNIFORM, "^" + ctxt.primPathAttribute );
        }
        if(const GT_AttributeListHandle pointAttrs = sourcePrim->getPointAttributes()) {
            GusdGT_AttrFilter::OwnerArgs owners;
            owners << GT_OWNER_POINT;
            filter.setActiveOwners(owners);
            updatePrimvarFromGTPrim( pointAttrs, filter, UsdGeomTokens->vertex, ctxt.time );
        }
        if(GT_AttributeListHandle vertexAttrs = sourcePrim->getVertexAttributes()) {
            GusdGT_AttrFilter::OwnerArgs owners;
            owners << GT_OWNER_VERTEX;
            filter.setActiveOwners(owners);

            if( reverseWindingOrder ) {
                // Use an index array (LUT) which will be used to lookup vertex 
                // attributes in the correct order.
                vertexAttrs = vertexAttrs->createIndirect(vertexIndirect); 
            }           

            updatePrimvarFromGTPrim( vertexAttrs, filter, UsdGeomTokens->faceVarying, ctxt.time );
        }

        if(const GT_AttributeListHandle primAttrs = sourcePrim->getUniformAttributes()) {
            GusdGT_AttrFilter::OwnerArgs owners;
            owners << GT_OWNER_UNIFORM;
            filter.setActiveOwners(owners);

            // When we import primvars from USD, both constant and uniform values get
            // put into primitive attributes. At this point we don't really know 
            // which to write so we use the hueristic, if all the values in the array are
            // identical, write the value as constant.

            for( auto it = primAttrs->begin(); !it.atEnd(); ++it ) {

                if(!filter.matches( it.getName() )) 
                    continue;

                GT_DataArrayHandle data = it.getData();
                TfToken interpolation = UsdGeomTokens->uniform;
                
                // cerr << "primitive primvar = " << it.getName() << ", " 
                //         << data->className() << ", " 
                //         << GTstorage( data->getStorage() ) << ", "
                //         << data->entries() << endl;
                // cerr << "face count = " << gtMesh->getFaceCount() << endl;

                if( GusdGT_Utils::isDataConstant( data ) ) {
                    interpolation = UsdGeomTokens->constant;
                    data = new GT_DASubArray( data, 0, 1 );
                }

                updatePrimvarFromGTPrim( 
                    TfToken( it.getName() ),
                    GT_OWNER_UNIFORM, 
                    interpolation,
                    ctxt.time, 
                    data );
            }
        }

        if(const GT_AttributeListHandle constAttrs = sourcePrim->getDetailAttributes()) {
            GusdGT_AttrFilter::OwnerArgs owners;
            owners << GT_OWNER_CONSTANT;
            filter.setActiveOwners(owners);
            updatePrimvarFromGTPrim( constAttrs, filter, UsdGeomTokens->constant, ctxt.time );
        }

        // If we have a "Cd" attribute, write it as both "Cd" and "displayColor".
        // The USD guys promise me that this data will get "deduplicated" so there
        // is not cost for doing this.
        GT_Owner own;
        if(GT_DataArrayHandle Cd = sourcePrim->findAttribute( "Cd", own, 0 )) {

            TfToken interpolation = s_ownerToUsdInterp[own];
            if ( own == GT_OWNER_PRIMITIVE ) {

                if( GusdGT_Utils::isDataConstant( Cd ) ) {
                    interpolation = UsdGeomTokens->constant;
                    Cd = new GT_DASubArray( Cd, 0, 1 );
                }
            }

            updatePrimvarFromGTPrim( TfToken("displayColor"), GT_OWNER_UNIFORM, 
                                     interpolation, ctxt.time, Cd );
        }
    }

    // ------------------------------------------------------------------------

    return GusdPrimWrapper::updateFromGTPrim(sourcePrim, houXform, ctxt, xformCache);
}

PXR_NAMESPACE_CLOSE_SCOPE
