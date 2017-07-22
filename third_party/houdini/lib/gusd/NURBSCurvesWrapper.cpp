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
#include "NURBSCurvesWrapper.h"

#include "context.h"
#include "UT_Gf.h"
#include "USD_Proxy.h"
#include "GT_VtArray.h"
#include "USD_XformCache.h"

#include <GT/GT_PrimCurveMesh.h>
#include <GT/GT_PrimNURBSCurveMesh.h>
#include <GT/GT_RefineParms.h>
#include <GT/GT_Refine.h>
#include <GT/GT_TransformArray.h>
#include <GT/GT_PrimInstance.h>
#include <GT/GT_DAIndirect.h>
#include <GT/GT_GEOPrimPacked.h>

#include <numeric>

PXR_NAMESPACE_OPEN_SCOPE

using std::cerr;
using std::cout;
using std::endl;
using std::map;

namespace {

map<GT_Basis, TfToken> gtToUsdBasisTranslation = {
    { GT_BASIS_BEZIER,      UsdGeomTokens->bezier },
    { GT_BASIS_BSPLINE,     UsdGeomTokens->bspline },
    { GT_BASIS_CATMULLROM,  UsdGeomTokens->catmullRom },
    { GT_BASIS_CATMULL_ROM, UsdGeomTokens->catmullRom },
    { GT_BASIS_HERMITE,     UsdGeomTokens->hermite }};

map<TfToken,GT_Basis> usdToGtBasisTranslation = {
    { UsdGeomTokens->bezier,     GT_BASIS_BEZIER },
    { UsdGeomTokens->bspline,    GT_BASIS_BSPLINE },
    { UsdGeomTokens->catmullRom, GT_BASIS_CATMULLROM },
    { UsdGeomTokens->hermite,    GT_BASIS_HERMITE }};

} // end of namespace

GusdNURBSCurvesWrapper::
GusdNURBSCurvesWrapper(
        const GT_PrimitiveHandle& sourcePrim,
        const UsdStagePtr& stage,
        const SdfPath& path,
        bool isOverride )
{
    initUsdPrim( stage, path, isOverride );
}

GusdNURBSCurvesWrapper::
GusdNURBSCurvesWrapper(
        const GusdUSD_StageProxyHandle& stage,
        const UsdGeomNurbsCurves&       usdCurves, 
        const UsdTimeCode&              time,
        const GusdPurposeSet&           purposes )
    : GusdPrimWrapper( time, purposes )
    , m_usdCurvesForRead( usdCurves, stage->GetLock() )
{
}

GusdNURBSCurvesWrapper::
~GusdNURBSCurvesWrapper()
{}

bool GusdNURBSCurvesWrapper::
initUsdPrim(const UsdStagePtr& stage,
            const SdfPath& path,
            bool asOverride)
{
    bool newPrim = true;
    if( asOverride ) {
        UsdPrim existing = stage->GetPrimAtPath( path );
        if( existing ) {
            newPrim = false;
            m_usdCurvesForWrite = UsdGeomNurbsCurves(stage->OverridePrim( path ));
        }
        else {
            // When fracturing, we want to override the outside surfaces and create
            // new inside surfaces in one export. So if we don't find an existing prim
            // with the given path, create a new one.
            m_usdCurvesForWrite = UsdGeomNurbsCurves::Define( stage, path );
        }
    }
    else {
        m_usdCurvesForWrite = UsdGeomNurbsCurves::Define( stage, path );  
    }
    if( !m_usdCurvesForWrite || !m_usdCurvesForWrite.GetPrim().IsValid() ) {
        TF_WARN( "Unable to create %s NURBS curves '%s'.", newPrim ? "new" : "override", path.GetText() );
    }
    return bool( m_usdCurvesForWrite );
}

GT_PrimitiveHandle GusdNURBSCurvesWrapper::
defineForWrite(
        const GT_PrimitiveHandle& sourcePrim,
        const UsdStagePtr& stage,
        const SdfPath& path,
        const GusdContext& ctxt)
{
    return new GusdNURBSCurvesWrapper( 
                    sourcePrim,
                    stage, 
                    path, 
                    ctxt.getOverGeo( sourcePrim ));
}

GT_PrimitiveHandle GusdNURBSCurvesWrapper::
defineForRead(
        const GusdUSD_StageProxyHandle& stage,
        const UsdGeomImageable&         sourcePrim, 
        const UsdTimeCode&              time,
        const GusdPurposeSet&           purposes )
{
    return new GusdNURBSCurvesWrapper( 
                    stage,
                    UsdGeomNurbsCurves( sourcePrim.GetPrim() ),
                    time, 
                    purposes );
}

bool GusdNURBSCurvesWrapper::
redefine( const UsdStagePtr& stage,
          const SdfPath& path,
          const GusdContext& ctxt,
          const GT_PrimitiveHandle& sourcePrim )
{
    initUsdPrim( stage, path, ctxt.getOverGeo( sourcePrim ));
    clearCaches();
    return true;
}

const UsdGeomImageable 
GusdNURBSCurvesWrapper::getUsdPrimForRead( GusdUSD_ImageableHolder::ScopedLock &lock) const
{
    // obtain first lock to get geomtry as UsdGeomCurves.
    GusdUSD_NURBSCurvesHolder::ScopedReadLock innerLock;
    innerLock.Acquire( m_usdCurvesForRead );

    // Build new holder after casting to imageable
    GusdUSD_ImageableHolder tmp( UsdGeomImageable( (*innerLock).GetPrim() ),
                                 m_usdCurvesForRead.GetLock() );
    lock.Acquire(tmp, /*write*/false);
    return *lock;
}

bool 
GusdNURBSCurvesWrapper::refine( 
    GT_Refine& refiner, 
    const GT_RefineParms* parms) const
{
    if(!isValid()) 
        return false;

    bool refineForViewport = GT_GEOPrimPacked::useViewportLOD(parms);

    GusdUSD_NURBSCurvesHolder::ScopedReadLock lock;
    lock.Acquire(m_usdCurvesForRead);
    UsdGeomNurbsCurves usdCurves = *lock;

    GT_AttributeListHandle gtVertexAttrs = new GT_AttributeList( new GT_AttributeMap() );
    GT_AttributeListHandle gtUniformAttrs = new GT_AttributeList( new GT_AttributeMap() );
    GT_AttributeListHandle gtDetailAttrs = new GT_AttributeList( new GT_AttributeMap() );

   // vertex counts
    UsdAttribute countsAttr = usdCurves.GetCurveVertexCountsAttr();
    if(!countsAttr) {
        TF_WARN( "Invalid USD vertext count attribute for NURB Curve. %s",
                 usdCurves.GetPrim().GetPath().GetText() );
        return false;
    }

    VtIntArray usdCounts;
    countsAttr.Get(&usdCounts, m_time);
    auto gtVertexCounts = new GusdGT_VtArray<int32>( usdCounts );

    UsdAttribute orderAttr = usdCurves.GetOrderAttr();
    if( !orderAttr ) {
        TF_WARN( "Invalid USD order attribute for NURB Curve. %s",
                 usdCurves.GetPrim().GetPath().GetText() );
        return false;
    }

    VtIntArray usdOrder;
    orderAttr.Get( &usdOrder, m_time );

    if( usdOrder.size() < usdCounts.size() ) {
        TF_WARN( "Not enough values given for USD order attribute for NURB Curve. %s",
                 usdCurves.GetPrim().GetPath().GetText() );
        return false;
    }
    GT_DataArrayHandle gtOrder = new GusdGT_VtArray<int32>( usdOrder );

    int numPoints = std::accumulate( usdCounts.begin(), usdCounts.end(), 0 );
    int numSegs = numPoints - 3 * usdCounts.size();
    int numSegEndPoints = numSegs + usdCounts.size();
    int numKnots = numPoints + std::accumulate( usdOrder.begin(), usdOrder.end(), 0 );

    // point positions
    UsdAttribute pointsAttr = usdCurves.GetPointsAttr();
    if(!pointsAttr) {
        TF_WARN( "Invalid USD points attribute for NURB Curve. %s",
                 usdCurves.GetPrim().GetPath().GetText() );
        return false;
    }

    VtVec3fArray usdPoints;
    pointsAttr.Get(&usdPoints, m_time);

    if( usdPoints.size() < numPoints ) {
        TF_WARN( "Not enough points specified for NURBS Curve. %s. Expected %d, got %zd",
                 usdCurves.GetPrim().GetPath().GetText(),
                 numPoints, usdPoints.size() );
        return false;
    }

    GT_DataArrayHandle gtPoints = new GusdGT_VtArray<GfVec3f>(usdPoints,GT_TYPE_POINT);
    gtVertexAttrs = gtVertexAttrs->addAttribute( "P", gtPoints, true );

    GT_Basis basis = GT_BASIS_LINEAR;
    GT_DataArrayHandle gtKnots;

    if( !refineForViewport ) {

        basis = GT_BASIS_BSPLINE;

        UsdAttribute knotsAttr = usdCurves.GetKnotsAttr();
        if( !knotsAttr ) {
            TF_WARN( "Invalid USD order attribute for NURB Curve. %s",
                     usdCurves.GetPrim().GetPath().GetText() );
        } 
        else {        
            VtDoubleArray usdKnots;
            knotsAttr.Get( &usdKnots, m_time );

            if( usdKnots.size() >= numKnots ) {
                gtKnots = new GusdGT_VtArray<fpreal64>( usdKnots );
            }
            else if ( usdKnots.size() == numKnots - 2 ) {

                // There was a time when the maya exported did not duplicate
                // the end points when it should. 
                auto knotArray = new GT_Real64Array( numKnots, 1 );
                knotArray->set( usdKnots[0], 0 );
                for( size_t i = 0; i < usdKnots.size(); ++i ) {
                    knotArray->set( usdKnots[i], i+1 );
                }
                knotArray->set( usdKnots[usdKnots.size()-1], numKnots-1 );
                gtKnots = knotArray;
            }
            else {
                TF_WARN( "Not enough NURBS curve knot values specified. %s. Expected %d, got %zd",
                     usdCurves.GetPrim().GetPath().GetText(),
                     numKnots, usdKnots.size() );
            }
        }

        // Build a array that maps values defined on segment end points to 
        // verticies. The number of segment end points is 2 less than the 
        // number of control point so just duplicate the first and last values.
        // - 3. 
        auto segEndPointIndicies = new GT_Int32Array( usdPoints.size(), 1 );  

        GT_Offset srcIdx = 0;
        GT_Offset dstIdx = 0;
        for( const auto& c : usdCounts ) {
            segEndPointIndicies->set( srcIdx, dstIdx++ );
            for( int i = 0; i < c - 2; ++i ) {
                segEndPointIndicies->set( srcIdx++, dstIdx++ );
            }
            segEndPointIndicies->set( srcIdx, dstIdx++ );
        }

        UsdAttribute widthsAttr = usdCurves.GetWidthsAttr();
        if(widthsAttr && widthsAttr.HasAuthoredValueOpinion() ) {

            VtFloatArray usdWidths;
            widthsAttr.Get(&usdWidths, m_time);
            GT_DataArrayHandle gtWidths = new GusdGT_VtArray<fpreal32>(usdWidths); 

            TfToken widthsInterpolation = usdCurves.GetWidthsInterpolation();
            if( widthsInterpolation == UsdGeomTokens->varying ) {

                if( usdWidths.size() < numSegEndPoints ) {
                    TF_WARN( "Not enough values provided for NURB curve varying widths for %s. Expected %d got %zd.",
                             usdCurves.GetPrim().GetPath().GetText(),
                             numSegEndPoints, usdWidths.size() );
                }
                else {

                    gtWidths = new GT_DAIndirect( segEndPointIndicies, gtWidths );  
                    gtVertexAttrs = gtVertexAttrs->addAttribute( "pscale", gtWidths, true );
                }
            }
            if( widthsInterpolation == UsdGeomTokens->vertex ) {

                if( usdWidths.size() < numPoints ) {
                    TF_WARN( "Not enough values provided for NURB curve vertex widths for %s. Expected %d got %zd.",
                             usdCurves.GetPrim().GetPath().GetText(),
                             numPoints, usdWidths.size() );
                }
                else {
                    gtVertexAttrs = gtVertexAttrs->addAttribute( "pscale", gtWidths, true );
                }
            }
            else if( widthsInterpolation == UsdGeomTokens->uniform ) {
                if( usdWidths.size() < usdCounts.size() ) { 
                    TF_WARN( "Not enough values provided for NURB curve uniform widths for %s. Expected %zd got %zd.",
                             usdCurves.GetPrim().GetPath().GetText(),
                             usdCounts.size(), usdWidths.size() );
                } 
                else {
                    gtUniformAttrs = gtUniformAttrs->addAttribute( "pscale", gtWidths, true );
                }
            }
            else if( widthsInterpolation == UsdGeomTokens->constant ) {
                if( usdWidths.size() < 1 ) { 
                    TF_WARN( "Not enough values provided for NURB curve constant widths for %s. Expected 1 got %zd.",
                             usdCurves.GetPrim().GetPath().GetText(),
                             usdWidths.size() );
                } 
                else {
                    GT_DataArrayHandle gtWidths = new GusdGT_VtArray<fpreal32>(usdWidths); 
                    gtDetailAttrs = gtDetailAttrs->addAttribute( "pscale", gtWidths, true );
                }
            }
        }
        // velocities
        UsdAttribute velAttr = usdCurves.GetVelocitiesAttr();
        if( velAttr && velAttr.HasAuthoredValueOpinion() ) {

            VtVec3fArray usdVelocities;
            velAttr.Get(&usdVelocities, m_time);

            GT_DataArrayHandle gtVelocities = 
                new GusdGT_VtArray<GfVec3f>(usdVelocities,GT_TYPE_VECTOR);

            // velocities are always vertex attributes
            gtVertexAttrs = gtVertexAttrs->addAttribute( "v", gtVelocities, true );
        }
        // normals
        UsdAttribute normAttr = usdCurves.GetNormalsAttr();
        if(normAttr && normAttr.HasAuthoredValueOpinion()) {
            VtVec3fArray usdNormals;
            normAttr.Get(&usdNormals, m_time);
            GT_DataArrayHandle gtNormals = 
                new GusdGT_VtArray<GfVec3f>(usdNormals,GT_TYPE_NORMAL);

            TfToken normalsInterpolation = usdCurves.GetNormalsInterpolation();
            if( normalsInterpolation == UsdGeomTokens->varying ) {

                if( usdNormals.size() < numSegEndPoints ) {
                    TF_WARN( "Not enough values provided for NURB curve varying normals for %s. Expected %d got %zd.",
                             usdCurves.GetPrim().GetPath().GetText(),
                             numSegEndPoints, usdNormals.size() );
                }
                else {

                    gtNormals = new GT_DAIndirect( segEndPointIndicies, gtNormals );  
                    gtVertexAttrs = gtVertexAttrs->addAttribute( "N", gtNormals, true );
                }
            }
            if( normalsInterpolation == UsdGeomTokens->vertex ) {

                if( usdNormals.size() < numPoints ) {
                    TF_WARN( "Not enough values provided for NURB curve vertex normals for %s. Expected %d got %zd.",
                             usdCurves.GetPrim().GetPath().GetText(),
                             numPoints, usdNormals.size() );
                }
                else {
                    gtVertexAttrs = gtVertexAttrs->addAttribute( "N", gtNormals, true );
                }
            }
            else if( normalsInterpolation == UsdGeomTokens->uniform ) {
                if( usdNormals.size() < usdCounts.size() ) { 
                    TF_WARN( "Not enough values provided for NURB curve uniform normals for %s. Expected %zd got %zd.",
                             usdCurves.GetPrim().GetPath().GetText(),
                             usdCounts.size(), usdNormals.size() );
                } 
                else {
                    gtUniformAttrs = gtUniformAttrs->addAttribute( "N", gtNormals, true );
                }
            }
            else if( normalsInterpolation == UsdGeomTokens->constant ) {
                if( usdNormals.size() < 1 ) { 
                    TF_WARN( "Not enough values provided for NURB curve constant widths for %s. Expected 1 got %zd.",
                             usdCurves.GetPrim().GetPath().GetText(),
                             usdNormals.size() );
                } 
                else {
                    gtDetailAttrs = gtDetailAttrs->addAttribute( "N", gtNormals, true );
                }
            }
        }
        // Load primvars. segEndPointIndicies are used if we need to expand primvar arrays
        // from a value at segment end points to values in point attributes.
        loadPrimvars( m_time, parms, 
                      usdCounts.size(),
                      usdPoints.size(),
                      numSegEndPoints,
                      usdCurves.GetPath().GetString(),
                      NULL,
                      &gtVertexAttrs,
                      &gtUniformAttrs,
                      &gtDetailAttrs,
                      segEndPointIndicies );

    } else {

        UsdGeomPrimvar colorPrimvar = usdCurves.GetPrimvar(TfToken("Cd"));
        if( !colorPrimvar || !colorPrimvar.GetAttr().HasAuthoredValueOpinion() ) {
            colorPrimvar = usdCurves.GetPrimvar(TfToken("displayColor"));
        }

        if( colorPrimvar && colorPrimvar.GetAttr().HasAuthoredValueOpinion()) {

            GT_DataArrayHandle gtData = convertPrimvarData( colorPrimvar, m_time );
            if( gtData ) {
                if( colorPrimvar.GetInterpolation() == UsdGeomTokens->constant ) {

                    gtDetailAttrs = gtDetailAttrs->addAttribute( "Cd", gtData, true );
                }
                else if( colorPrimvar.GetInterpolation() == UsdGeomTokens->uniform ) {

                    gtUniformAttrs = gtUniformAttrs->addAttribute( "Cd", gtData, true );
                }
                else if( colorPrimvar.GetInterpolation() == UsdGeomTokens->vertex ) {

                    gtVertexAttrs = gtVertexAttrs->addAttribute( "Cd", gtData, true );
                }
                else {

                    auto segEndPointIndicies = new GT_Int32Array( usdPoints.size(), 1 );  

                    GT_Offset srcIdx = 0;
                    GT_Offset dstIdx = 0;
                    for( const auto& c : usdCounts ) {
                        segEndPointIndicies->set( srcIdx, dstIdx++ );
                        for( int i = 0; i < c; ++i ) {
                            segEndPointIndicies->set( srcIdx++, dstIdx++ );
                        }
                        segEndPointIndicies->set( srcIdx, dstIdx++ );
                    }
                    gtData = new GT_DAIndirect( segEndPointIndicies, gtData );
                    gtVertexAttrs = gtVertexAttrs->addAttribute( "Cd", gtData, true );
                }
            }
        }
    }

    auto prim = new GT_PrimCurveMesh( 
        basis, 
        gtVertexCounts,
        gtVertexAttrs,
        gtUniformAttrs,
        gtDetailAttrs,
        false );

    if( !refineForViewport ) {
        if( gtOrder ) {
            prim->setOrder( gtOrder );
        }
        if( gtKnots ) {
            prim->setKnots( gtKnots );
        }
    }

    // set local transform
    UT_Matrix4D mat;

    if( !GusdUSD_XformCache::GetInstance().GetLocalToWorldTransform( 
            usdCurves.GetPrim(),
            m_time, 
            mat ) ) {
        TF_WARN( "Failed to compute transform" );
        return false;
    }

    prim->setPrimitiveTransform( getPrimitiveTransform() );
    refiner.addPrimitive( prim );
    return true;
}

bool GusdNURBSCurvesWrapper::
getUniqueID(int64& id) const
{
    static const int s_id = GT_Primitive::createPrimitiveTypeId();
    id = s_id;
    return true;
}


const char* GusdNURBSCurvesWrapper::
className() const
{
    return "GusdNURBSCurvesWrapper";
}


void GusdNURBSCurvesWrapper::
enlargeBounds(UT_BoundingBox boxes[], int nsegments) const
{
    // TODO
}


int GusdNURBSCurvesWrapper::
getMotionSegments() const
{
    // TODO
    return 1;
}


int64 GusdNURBSCurvesWrapper::
getMemoryUsage() const
{
    // TODO
    return 0;
}


GT_PrimitiveHandle GusdNURBSCurvesWrapper::
doSoftCopy() const
{
    // TODO
    return GT_PrimitiveHandle(new GusdNURBSCurvesWrapper(*this));
}


bool 
GusdNURBSCurvesWrapper::isValid() const
{
    return bool( m_usdCurvesForWrite || m_usdCurvesForRead );
}

bool 
GusdNURBSCurvesWrapper::updateFromGTPrim(
     const GT_PrimitiveHandle&  sourcePrim,
     const UT_Matrix4D&         houXform,
     const GusdContext&         ctxt,
     GusdSimpleXformCache&      xformCache
      )
{
    if( !m_usdCurvesForWrite ) {
        TF_WARN( "Attempting to update invalid curve prim" );
        return false;
    }

    const GT_PrimCurveMesh* gtCurves
        = dynamic_cast<const GT_PrimCurveMesh*>(sourcePrim.get());
    if(!gtCurves) {
        TF_WARN( "Attempting to update curve of wrong type %s", sourcePrim->className() );
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

    GfMatrix4d xform = computeTransform( 
                            m_usdCurvesForWrite.GetPrim().GetParent(),
                            ctxt.time,
                            houXform,
                            xformCache );

    GfMatrix4d loc_xform = computeTransform( 
                            m_usdCurvesForWrite.GetPrim(),
                            ctxt.time,
                            houXform,
                            xformCache );

    // If we are writing points for an overlay but not writing transforms, 
    // then we have to transform the points into the proper space.
    bool transformPoints = 
        (overlayPoints || overlayAll) && 
        !GusdUT_Gf::Cast(loc_xform).isIdentity();

    GT_Owner attrOwner = GT_OWNER_INVALID;
    GT_DataArrayHandle houAttr;
    UsdAttribute usdAttr;
    
    if( writeNewGeo && ctxt.purpose != UsdGeomTokens->default_ ) {
        m_usdCurvesForWrite.GetPurposeAttr().Set( ctxt.purpose );
    }

    // intrinsic attributes ----------------------------------------------------

    if( writeNewGeo || overlayAll || overlayTransforms || overlayPoints) {

        // extent ------------------------------------------------------------------
        houAttr = GusdGT_Utils::getExtentsArray(sourcePrim);
        usdAttr = m_usdCurvesForWrite.GetExtentAttr();
        if(houAttr && usdAttr && transformPoints ) {
             houAttr = GusdGT_Utils::transformPoints( houAttr, loc_xform );
        }
        updateAttributeFromGTPrim( GT_OWNER_INVALID, "extents", houAttr, usdAttr, ctxt.time );
    }

    // transform ---------------------------------------------------------------
    if( writeNewGeo || overlayAll || overlayTransforms) {
        updateTransformFromGTPrim( xform, ctxt.time, 
                                   ctxt.granularity == GusdContext::PER_FRAME );
    }

    // visibility ---------------------------------------------------------------

    updateVisibilityFromGTPrim(sourcePrim, ctxt.time, 
                               (writeNewGeo || overlayAll) && 
                                ctxt.granularity == GusdContext::PER_FRAME );

    if( writeNewGeo || overlayAll || overlayPoints ) {
        
        // P
        houAttr = sourcePrim->findAttribute("P", attrOwner, 0);
        usdAttr = m_usdCurvesForWrite.GetPointsAttr();
        if(houAttr && usdAttr && transformPoints ) {
            houAttr = GusdGT_Utils::transformPoints( houAttr, loc_xform );
        }
        updateAttributeFromGTPrim( attrOwner, "P", houAttr, usdAttr, ctxt.time );
    }

    if( writeNewGeo || overlayAll ) {
        // Vertex counts

        usdAttr = m_usdCurvesForWrite.GetCurveVertexCountsAttr();
        auto gtCurveCounts = gtCurves->getCurveCounts();

        updateAttributeFromGTPrim( GT_OWNER_INVALID, "vertexcounts",
                                   gtCurveCounts, usdAttr, ctxt.time );

        // Order
        usdAttr = m_usdCurvesForWrite.GetOrderAttr();
        if( gtCurves->isUniformOrder() ) {
            VtIntArray val( gtCurveCounts->entries() );
            for( size_t i = 0; i < val.size(); ++i ) {
                val[i] = gtCurves->uniformOrder();
            }
            usdAttr.Set( val );
        }
        else {
            GT_DataArrayHandle buffer;
            const int64* gtOrders = gtCurves->varyingOrders()->getI64Array( buffer );
            VtIntArray val( gtCurves->varyingOrders()->entries() );
             for( size_t i = 0; i < val.size(); ++i ) {
                val[i] = gtOrders[i];
            }
            usdAttr.Set( val );           
        }

        // Knots 
        GT_DataArrayHandle knotTmpBuffer;
        usdAttr = m_usdCurvesForWrite.GetKnotsAttr();
        GT_DataArrayHandle gtKnots = gtCurves->knots();
        if( gtKnots->getStorage() != GT_STORE_REAL64 ) {
            gtKnots = new GT_Real64Array( gtKnots->getF64Array( knotTmpBuffer ),
                                          gtKnots->entries(), 1 );
        }

        updateAttributeFromGTPrim( GT_OWNER_INVALID, "knots",
                                   gtKnots, usdAttr, ctxt.time );      
    }


    if( writeNewGeo || overlayAll || overlayPoints ) {
        // N
        houAttr = sourcePrim->findAttribute("N", attrOwner, 0);
        usdAttr = m_usdCurvesForWrite.GetNormalsAttr();
        updateAttributeFromGTPrim( attrOwner, "N", houAttr, usdAttr, ctxt.time );

        // v
        houAttr = sourcePrim->findAttribute("v", attrOwner, 0);
        usdAttr = m_usdCurvesForWrite.GetVelocitiesAttr();
        updateAttributeFromGTPrim( attrOwner, "v", houAttr, usdAttr, ctxt.time );
        
        // pscale & width
        houAttr = sourcePrim->findAttribute("width", attrOwner, 0);
        if(!houAttr) {
            houAttr = sourcePrim->findAttribute("pscale", attrOwner, 0);
        }

        usdAttr = m_usdCurvesForWrite.GetWidthsAttr();

        updateAttributeFromGTPrim( attrOwner, "width", houAttr, usdAttr, ctxt.time );
        m_usdCurvesForWrite.SetWidthsInterpolation( UsdGeomTokens->vertex );
    }

    // -------------------------------------------------------------------------
    
    // primvars ----------------------------------------------------------------
    
    if( writeNewGeo || overlayAll || overlayPrimvars ) {

        GusdGT_AttrFilter filter = ctxt.attributeFilter;

        filter.appendPattern(GT_OWNER_VERTEX, "^P ^N ^v ^width ^pscale ^visible");
        if(const GT_AttributeListHandle vtxAttrs = sourcePrim->getVertexAttributes()) {
            GusdGT_AttrFilter::OwnerArgs owners;
            owners << GT_OWNER_VERTEX;
            filter.setActiveOwners(owners);
            updatePrimvarFromGTPrim( 
                vtxAttrs, filter, UsdGeomTokens->vertex, ctxt.time );
        }
        filter.appendPattern(GT_OWNER_CONSTANT, "^visible");
        if(const GT_AttributeListHandle constAttrs = sourcePrim->getDetailAttributes()) {
            GusdGT_AttrFilter::OwnerArgs owners;
            owners << GT_OWNER_CONSTANT;
            filter.setActiveOwners(owners);
            updatePrimvarFromGTPrim( constAttrs, filter, UsdGeomTokens->constant, ctxt.time );
        }
        filter.appendPattern(GT_OWNER_UNIFORM, "^visible");
        if(const GT_AttributeListHandle uniformAttrs = sourcePrim->getUniformAttributes()) {
            GusdGT_AttrFilter::OwnerArgs owners;
            owners << GT_OWNER_UNIFORM;
            filter.setActiveOwners(owners);
            updatePrimvarFromGTPrim( uniformAttrs, filter, UsdGeomTokens->uniform, ctxt.time );
        }

        // If we have a "Cd" attribute, write it as both "Cd" and "displayColor".
        // The USD guys promise me that this data will get "deduplicated" so there
        // is not cost for doing this.
        GT_Owner own;
        if(GT_DataArrayHandle Cd = sourcePrim->findAttribute( "Cd", own, 0 )) {

            GT_AttributeMapHandle attrMap = new GT_AttributeMap();
            GT_AttributeListHandle attrList = new GT_AttributeList( attrMap );
            attrList = attrList->addAttribute( "displayColor", Cd, true );
            GusdGT_AttrFilter filter( "*" );
            GusdGT_AttrFilter::OwnerArgs owners;
            owners << own;
            filter.setActiveOwners(owners);
            updatePrimvarFromGTPrim( attrList, filter, s_ownerToUsdInterpCurve[own], ctxt.time );
        }
    }

    // -------------------------------------------------------------------------
    return GusdPrimWrapper::updateFromGTPrim(sourcePrim, houXform, ctxt, xformCache);
}

PXR_NAMESPACE_CLOSE_SCOPE


