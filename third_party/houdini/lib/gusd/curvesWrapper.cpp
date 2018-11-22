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
#include "curvesWrapper.h"
#include "NURBSCurvesWrapper.h"

#include "context.h"
#include "GT_VtArray.h"
#include "tokens.h"
#include "UT_Gf.h"
#include "USD_XformCache.h"

#include <GT/GT_PrimCurveMesh.h>
#include <GT/GT_RefineParms.h>
#include <GT/GT_Refine.h>
#include <GT/GT_TransformArray.h>
#include <GT/GT_PrimInstance.h>
#include <GT/GT_DAIndirect.h>
#include <GT/GT_GEOPrimPacked.h>
#include <GT/GT_DAConstant.h>
#include <GT/GT_DAConstantValue.h>

#include <iostream>

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

void _validateData( 
        const char*             destName, 
        const char*             srcName,
        const char*             primName,
        GT_DataArrayHandle      data, 
        const TfToken&          interpolation,
        GT_DataArrayHandle      segEndPointIndicies,
        int                     numCurves,
        int                     numPoints,
        int                     numSegmentEndPoints,
        GT_AttributeListHandle* vertexAttrs,
        GT_AttributeListHandle* uniformAttrs,
        GT_AttributeListHandle* detailAttrs );

} // end of namespace

GusdCurvesWrapper::
GusdCurvesWrapper(
        const GT_PrimitiveHandle& sourcePrim,
        const UsdStagePtr& stage,
        const SdfPath& path,
        bool isOverride )
    : m_forceCreateNewGeo( false )
{
    initUsdPrim( stage, path, isOverride );
}

GusdCurvesWrapper::
GusdCurvesWrapper(
        const UsdGeomBasisCurves&   usdCurves, 
        UsdTimeCode                 time,
        GusdPurposeSet              purposes )
    : GusdPrimWrapper( time, purposes )
    , m_usdCurves( usdCurves )
    , m_forceCreateNewGeo( false )
{
}

GusdCurvesWrapper::
~GusdCurvesWrapper()
{}

bool GusdCurvesWrapper::
initUsdPrim(const UsdStagePtr& stage,
            const SdfPath& path,
            bool asOverride)
{
    bool newPrim = true;
    m_forceCreateNewGeo = false;
    if( asOverride ) {
        UsdPrim existing = stage->GetPrimAtPath( path );
        if( existing ) {
            newPrim = false;
            m_usdCurves = UsdGeomBasisCurves(stage->OverridePrim( path ));
        }
        else {
            // When fracturing, we want to override the outside surfaces and create
            // new inside surfaces in one export. So if we don't find an existing prim
            // with the given path, create a new one.
            m_usdCurves = UsdGeomBasisCurves::Define( stage, path );
            m_forceCreateNewGeo = true;
        }
    }
    else {
        m_usdCurves = UsdGeomBasisCurves::Define( stage, path );  
    }
    if( !m_usdCurves || !m_usdCurves.GetPrim().IsValid() ) {
        TF_WARN( "Unable to create %s curves '%s'.", newPrim ? "new" : "override", path.GetText() );
    }
    return bool( m_usdCurves );
}

GT_PrimitiveHandle GusdCurvesWrapper::
defineForWrite(
        const GT_PrimitiveHandle& sourcePrim,
        const UsdStagePtr& stage,
        const SdfPath& path,
        const GusdContext& ctxt)
{
    if( sourcePrim->getPrimitiveType() != GT_PRIM_CURVE_MESH ) {
        TF_WARN( "Invalid prim" );
        return GT_PrimitiveHandle();
    }

    auto sourceCurves = UTverify_cast<const GT_PrimCurveMesh*>(sourcePrim.get());

    // For most types, the prim wrapper base classes decides what type USD prim to 
    // create based on the type of the GT prim. However, Basis curves and NURBs share
    // the same GT type.
    // We have some legacy code that turns all curves into NURBs. This causes a problem
    // with overlays, so we add a check to make sure we are overlaying the proper type.

    if( sourceCurves->getBasis() == GT_BASIS_BSPLINE && sourceCurves->knots() ) {

        bool validNURB = true;
        if( ctxt.writeOverlay ) {
            UsdPrim existing = stage->GetPrimAtPath( path );
            if( existing && existing.IsA<UsdGeomBasisCurves>() ) {
                validNURB = false;
            }
        }

        if( validNURB ) {
            return new GusdNURBSCurvesWrapper( 
                            sourcePrim, 
                            stage, 
                            path, 
                            ctxt.writeOverlay );
        }
    }
    return new GusdCurvesWrapper( 
                    sourcePrim,
                    stage, 
                    path, 
                    ctxt.writeOverlay );
}

GT_PrimitiveHandle GusdCurvesWrapper::
defineForRead(
        const UsdGeomImageable& sourcePrim, 
        UsdTimeCode             time,
        GusdPurposeSet          purposes )
{
    return new GusdCurvesWrapper( 
                    UsdGeomBasisCurves( sourcePrim.GetPrim() ),
                    time, 
                    purposes );
}

bool GusdCurvesWrapper::
redefine( const UsdStagePtr& stage,
          const SdfPath& path,
          const GusdContext& ctxt,
          const GT_PrimitiveHandle& sourcePrim )
{
    initUsdPrim( stage, path, ctxt.writeOverlay );
    clearCaches();
    return true;
}

bool 
GusdCurvesWrapper::refine( 
    GT_Refine& refiner, 
    const GT_RefineParms* parms) const
{
    if(!isValid()) 
        return false;

    bool refineForViewport = GT_GEOPrimPacked::useViewportLOD(parms);

    const UsdGeomBasisCurves& usdCurves = m_usdCurves;

    GT_AttributeListHandle gtVertexAttrs = new GT_AttributeList( new GT_AttributeMap() );
    GT_AttributeListHandle gtUniformAttrs = new GT_AttributeList( new GT_AttributeMap() );
    GT_AttributeListHandle gtDetailAttrs = new GT_AttributeList( new GT_AttributeMap() );

    GT_Basis basis = GT_BASIS_INVALID;
    if( refineForViewport ) {
        basis = GT_BASIS_LINEAR;
    }
    else {

        TfToken type;
        usdCurves.GetTypeAttr().Get( &type, m_time );
        if( type == UsdGeomTokens->linear ) {
            basis = GT_BASIS_LINEAR;
        }
        else {

            TfToken usdBasis;
            usdCurves.GetBasisAttr().Get( &usdBasis, m_time );
            auto const& it = usdToGtBasisTranslation.find( usdBasis );
            if( it != usdToGtBasisTranslation.end() ) {
                basis = it->second;
            }
        }
        if( basis == GT_BASIS_INVALID ) {
            TF_WARN( "Usupported curve basis" );
            return false;
        }
    }

    bool wrap = false;

    TfToken usdWrap;
    usdCurves.GetWrapAttr().Get( &usdWrap, m_time );
    if( usdWrap == UsdGeomTokens->periodic ) {
        wrap = true;
    }

   // vertex counts
    UsdAttribute countsAttr = usdCurves.GetCurveVertexCountsAttr();
    if(!countsAttr)
        return false;

    VtIntArray usdCounts;
    countsAttr.Get(&usdCounts, m_time);
    auto gtVertexCounts = new GusdGT_VtArray<int32>( usdCounts );

    // point positions
    UsdAttribute pointsAttr = usdCurves.GetPointsAttr();
    if(!pointsAttr)
        return false;
    VtVec3fArray usdPoints;
    pointsAttr.Get(&usdPoints, m_time);

    UT_IntrusivePtr<GT_Int32Array> segEndPointIndicies;
    int numSegmentEndPoints = usdPoints.size();
    if( !refineForViewport ) {

        // In USD, primvars for bezier curves are stored on the endpoints of 
        // each segment. In Houdini these need to be point attributes. Create a
        // LUT to map these indexes.

        if( basis == GT_BASIS_BEZIER ) {

            segEndPointIndicies = new GT_Int32Array( usdPoints.size(), 1 );  

            GT_Offset srcIdx = 0;
            GT_Offset dstIdx = 0;
            for( const auto& c : usdCounts ) {
                for( int i = 0, segs = c / 3; i < segs; ++i ) {
                    segEndPointIndicies->set( srcIdx, dstIdx++ );
                    segEndPointIndicies->set( srcIdx, dstIdx++ );
                    segEndPointIndicies->set( srcIdx, dstIdx++ );
                    ++srcIdx;
                }
                if( !wrap ) {
                    segEndPointIndicies->set( srcIdx++, dstIdx++ );
                }
            }
            numSegmentEndPoints = srcIdx;
        }
        else if ( basis == GT_BASIS_BSPLINE || 
                  basis == GT_BASIS_CATMULLROM ) {

            // For non-periodic bsplines and catroms there are 2 less segment end points
            // then there are vertices. Just dupe the first and last values.
            
            if( !wrap ) {
                segEndPointIndicies = new GT_Int32Array( usdPoints.size(), 1 );  

                GT_Offset srcIdx = 0;
                GT_Offset dstIdx = 0;
                for( const auto& c : usdCounts ) {
                    segEndPointIndicies->set( srcIdx, dstIdx++ );
                    for( int i = 0; i < c - 2 ; ++i ) {
                        segEndPointIndicies->set( srcIdx++, dstIdx++ );
                    }
                    segEndPointIndicies->set( srcIdx, dstIdx++ );
                }
                numSegmentEndPoints = srcIdx;
            }
        }
        else if( basis != GT_BASIS_LINEAR ) {
            TF_WARN( "Can't map curve primvar. Unsupported curve type" );
        }
    }

    auto gtPoints = new GusdGT_VtArray<GfVec3f>(usdPoints,GT_TYPE_POINT);
    gtVertexAttrs = gtVertexAttrs->addAttribute( "P", gtPoints, true );

    if( !refineForViewport ) {

        UsdAttribute widthsAttr = usdCurves.GetWidthsAttr();
        if(widthsAttr && widthsAttr.HasAuthoredValueOpinion() ) {

            VtFloatArray usdWidths;
            widthsAttr.Get(&usdWidths, m_time);

            _validateData( "pscale", "widths", 
                        usdCurves.GetPrim().GetPath().GetText(),
                        new GusdGT_VtArray<fpreal32>(usdWidths),
                        usdCurves.GetWidthsInterpolation(),
                        segEndPointIndicies,
                        gtPoints->entries(),
                        gtVertexCounts->entries(),
                        numSegmentEndPoints,
                        &gtVertexAttrs,
                        &gtUniformAttrs,
                        &gtDetailAttrs );
        }

        // velocities
        UsdAttribute velAttr = usdCurves.GetVelocitiesAttr();
        if( velAttr && velAttr.HasAuthoredValueOpinion() ) {

            VtVec3fArray usdVelocities;
            velAttr.Get(&usdVelocities, m_time);

            GT_DataArrayHandle gtVelocities = 
                new GusdGT_VtArray<GfVec3f>(usdVelocities,GT_TYPE_VECTOR);

            gtVertexAttrs = gtVertexAttrs->addAttribute( "v", gtVelocities, true );
        }

        // normals
        UsdAttribute normAttr = usdCurves.GetNormalsAttr();
        if(normAttr && normAttr.HasAuthoredValueOpinion()) {
            VtVec3fArray usdNormals;
            normAttr.Get(&usdNormals, m_time);

            _validateData( "N", "normals", 
                        usdCurves.GetPrim().GetPath().GetText(),
                        new GusdGT_VtArray<GfVec3f>(usdNormals,GT_TYPE_NORMAL),
                        usdCurves.GetNormalsInterpolation(),
                        segEndPointIndicies,
                        gtPoints->entries(),
                        gtVertexCounts->entries(),
                        numSegmentEndPoints,
                        &gtVertexAttrs,
                        &gtUniformAttrs,
                        &gtDetailAttrs );
        }

        // Load primvars. segEndPointIndicies are used if we need to expand primvar arrays
        // from a value at segment end points to values in point attributes.
        loadPrimvars( m_time, parms, 
                      usdCounts.size(),
                      usdPoints.size(),
                      numSegmentEndPoints,
                      usdCurves.GetPath().GetString(),
                      NULL,
                      &gtVertexAttrs,
                      &gtUniformAttrs,
                      &gtDetailAttrs,
                      segEndPointIndicies );
    } 
    else {

        UsdGeomPrimvar colorPrimvar = usdCurves.GetPrimvar(GusdTokens->Cd);
        if( !colorPrimvar || !colorPrimvar.GetAttr().HasAuthoredValueOpinion() ) {
            colorPrimvar = usdCurves.GetPrimvar(GusdTokens->displayColor);
        }

        if( colorPrimvar && colorPrimvar.GetAttr().HasAuthoredValueOpinion()) {

            // cerr << "curve color primvar " << colorPrimvar.GetBaseName() << "\t" << colorPrimvar.GetTypeName() << "\t" << colorPrimvar.GetInterpolation() << endl;

            GT_DataArrayHandle gtData = convertPrimvarData( colorPrimvar, m_time );
            if( gtData ) {
                if( colorPrimvar.GetInterpolation() == UsdGeomTokens->constant ) {

                    gtDetailAttrs = gtDetailAttrs->addAttribute( "Cd", gtData, true );
                }
                else if( colorPrimvar.GetInterpolation() == UsdGeomTokens->uniform ) {

                    gtUniformAttrs = gtUniformAttrs->addAttribute( "Cd", gtData, true );
                }
                else if( colorPrimvar.GetInterpolation() == UsdGeomTokens->vertex ||
                       ( colorPrimvar.GetInterpolation() == UsdGeomTokens->varying && 
                            basis == GT_BASIS_LINEAR )) {

                    gtVertexAttrs = gtVertexAttrs->addAttribute( "Cd", gtData, true );
                }
                else {

                    // In this case there is one value per segment end point

                    auto segEndPointIndicies = new GT_Int32Array( usdPoints.size(), 1 );  

                    GT_Offset srcIdx = 0;
                    GT_Offset dstIdx = 0;
                    if( basis == GT_BASIS_BEZIER ) { 
                        for( const auto& c : usdCounts ) {
                            for( int i = 0, segs = c / 3; i < segs; ++i ) {
                                segEndPointIndicies->set( srcIdx, dstIdx++ );
                                segEndPointIndicies->set( srcIdx, dstIdx++ );
                                segEndPointIndicies->set( srcIdx, dstIdx++ );
                                ++srcIdx;
                            }
                            if( !wrap ) {
                                segEndPointIndicies->set( srcIdx++, dstIdx++ );
                            }
                        }
                    }
                    else if ( basis == GT_BASIS_BSPLINE || basis == GT_BASIS_CATMULLROM ) {
                        for( const auto& c : usdCounts ) {
                            segEndPointIndicies->set( srcIdx, dstIdx++ );
                            for( int i = 0; i < c; ++i ) {
                                segEndPointIndicies->set( srcIdx++, dstIdx++ );
                            }
                            if( !wrap ) {
                                segEndPointIndicies->set( srcIdx, dstIdx++ );
                            }
                        }
                    }
                    gtData = new GT_DAIndirect( segEndPointIndicies, gtData );
                    gtVertexAttrs = gtVertexAttrs->addAttribute( "Cd", gtData, true );
                }
            }
        }

        UsdGeomPrimvar alphaPrimvar = usdCurves.GetPrimvar(GusdTokens->Alpha);
        if( !alphaPrimvar || !alphaPrimvar.GetAttr().HasAuthoredValueOpinion() ) {
            alphaPrimvar = usdCurves.GetPrimvar(GusdTokens->displayOpacity);
        }

        if( alphaPrimvar && alphaPrimvar.GetAttr().HasAuthoredValueOpinion()) {

            // cerr << "curve color primvar " << alphaPrimvar.GetBaseName() << "\t" << alphaPrimvar.GetTypeName() << "\t" << alphaPrimvar.GetInterpolation() << endl;

            GT_DataArrayHandle gtData = convertPrimvarData( alphaPrimvar, m_time );
            if( gtData ) {
                if( alphaPrimvar.GetInterpolation() == UsdGeomTokens->constant ) {

                    gtDetailAttrs = gtDetailAttrs->addAttribute( "Alpha", gtData, true );
                }
                else if( alphaPrimvar.GetInterpolation() == UsdGeomTokens->uniform ) {

                    gtUniformAttrs = gtUniformAttrs->addAttribute( "Alpha", gtData, true );
                }
                else if( alphaPrimvar.GetInterpolation() == UsdGeomTokens->vertex ||
                       ( alphaPrimvar.GetInterpolation() == UsdGeomTokens->varying && 
                            basis == GT_BASIS_LINEAR )) {

                    gtVertexAttrs = gtVertexAttrs->addAttribute( "Alpha", gtData, true );
                }
                else {

                    // In this case there is one value per segment end point

                    auto segEndPointIndicies = new GT_Int32Array( usdPoints.size(), 1 );  

                    GT_Offset srcIdx = 0;
                    GT_Offset dstIdx = 0;
                    if( basis == GT_BASIS_BEZIER ) { 
                        for( const auto& c : usdCounts ) {
                            for( int i = 0, segs = c / 3; i < segs; ++i ) {
                                segEndPointIndicies->set( srcIdx, dstIdx++ );
                                segEndPointIndicies->set( srcIdx, dstIdx++ );
                                segEndPointIndicies->set( srcIdx, dstIdx++ );
                                ++srcIdx;
                            }
                            if( !wrap ) {
                                segEndPointIndicies->set( srcIdx++, dstIdx++ );
                            }
                        }
                    }
                    else if ( basis == GT_BASIS_BSPLINE || basis == GT_BASIS_CATMULLROM ) {
                        for( const auto& c : usdCounts ) {
                            segEndPointIndicies->set( srcIdx, dstIdx++ );
                            for( int i = 0; i < c; ++i ) {
                                segEndPointIndicies->set( srcIdx++, dstIdx++ );
                            }
                            if( !wrap ) {
                                segEndPointIndicies->set( srcIdx, dstIdx++ );
                            }
                        }
                    }
                    gtData = new GT_DAIndirect( segEndPointIndicies, gtData );
                    gtVertexAttrs = gtVertexAttrs->addAttribute( "Alpha", gtData, true );
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
        wrap );

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

namespace {

void
_validateData( 
        const char*             destName, 
        const char*             srcName,
        const char*             primName,
        GT_DataArrayHandle      data, 
        const TfToken&          interpolation,
        GT_DataArrayHandle      segEndPointIndicies,
        int                     numCurves,
        int                     numPoints,
        int                     numSegmentEndPoints,
        GT_AttributeListHandle* vertexAttrs,
        GT_AttributeListHandle* uniformAttrs,
        GT_AttributeListHandle* detailAttrs )
{
    if( interpolation == UsdGeomTokens->varying && segEndPointIndicies ) {

        if( data->entries() < numSegmentEndPoints ) {
            TF_WARN( "Not enough values found for attribute: %s:%s",
                     primName, srcName );
        }
        else {
            // if necessary, expand primvar values from samples at segment
            // ends to point attributes.

            data = new GT_DAIndirect( segEndPointIndicies, data );
            *vertexAttrs = (*vertexAttrs)->addAttribute( destName, data, true );
        }
    }
    else if( interpolation == UsdGeomTokens->vertex || 
             interpolation == UsdGeomTokens->varying ) {

        if( data->entries() < numPoints ) {
            TF_WARN( "Not enough values found for attribute: %s:%s",
                    primName, srcName );  
        }
        else {
            *vertexAttrs =  (*vertexAttrs)->addAttribute( destName, data, true );
        }
    }                 
    else if( interpolation == UsdGeomTokens->uniform ) {
        if( data->entries() < numCurves ) {
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


const char* GusdCurvesWrapper::
className() const
{
    return "GusdCurvesWrapper";
}


void GusdCurvesWrapper::
enlargeBounds(UT_BoundingBox boxes[], int nsegments) const
{
    // TODO
}


int GusdCurvesWrapper::
getMotionSegments() const
{
    // TODO
    return 1;
}


int64 GusdCurvesWrapper::
getMemoryUsage() const
{
    // TODO
    return 0;
}


GT_PrimitiveHandle GusdCurvesWrapper::
doSoftCopy() const
{
    // TODO
    return GT_PrimitiveHandle(new GusdCurvesWrapper(*this));
}


bool GusdCurvesWrapper::isValid() const
{
    return bool( m_usdCurves );
}




bool GusdCurvesWrapper::
updateFromGTPrim(const GT_PrimitiveHandle&  sourcePrim,
                 const UT_Matrix4D&         houXform,
                 const GusdContext&         ctxt,
                 GusdSimpleXformCache&      xformCache
                  )
{
    if( !m_usdCurves ) {
        TF_WARN( "Attempting to update invalid curve prim" );
        return false;
    }

    const GT_PrimCurveMesh* gtCurves
        = dynamic_cast<const GT_PrimCurveMesh*>(sourcePrim.get());
    if(!gtCurves) {
        TF_WARN( "Attempting to update invalid curve prim" );
        return false;
    }

    bool writeOverlay = ctxt.writeOverlay && !m_forceCreateNewGeo;

    // While I suppose we could write both points and transforms, it gets confusing,
    // and I don't thik its necessary so lets not.
    bool overlayTransforms = ctxt.overlayTransforms && !ctxt.overlayPoints;

    // USD only supports linear and cubic curves. Houdini supports higher order
    // curves but we just issue a warning when we see them.

    // The Houdini APIs, support closed curves, but in practice we don't see
    // the wrap attribute ever being non-zero. Instead was see an extra segment
    // that overlaps the first segment.

    // USD expects primvars (and widths) specified for the end points of segments
    // while Houdini uses point attributes. 

    if( !gtCurves->isUniformOrder() ) {
        TF_WARN("Non-uniform curve order not supported" );
        return false;
    }

    int order = gtCurves->uniformOrder();
    if( order != 2 && order != 4 ) {
        TF_WARN("USD only supports linear and cubic curves." );
        return false;
    }
    GT_Basis basis = gtCurves->getBasis();

    bool closed = gtCurves->getWrap();

    if( !writeOverlay || ctxt.overlayAll ) {

        m_usdCurves.CreateTypeAttr().Set( 
            order == 2 ? UsdGeomTokens->linear : UsdGeomTokens->cubic );

        if( order == 4 ) {

            auto it = gtToUsdBasisTranslation.find( basis );
            if( it == gtToUsdBasisTranslation.end() ) {
                TF_WARN( "Unsupported curve basis '%s'.", GTbasis( basis ));
                return false;
            }
            m_usdCurves.CreateBasisAttr().Set( it->second );
        }

        m_usdCurves.CreateWrapAttr().Set( 
            closed ? UsdGeomTokens->periodic : UsdGeomTokens->nonperiodic );
    }

    UsdTimeCode geoTime = ctxt.time;
    if( ctxt.writeStaticGeo ) {
        geoTime = UsdTimeCode::Default();
    }

    GfMatrix4d xform = computeTransform( 
                            m_usdCurves.GetPrim().GetParent(),
                            geoTime,
                            houXform,
                            xformCache );

    GfMatrix4d loc_xform = computeTransform( 
                            m_usdCurves.GetPrim(),
                            geoTime,
                            houXform,
                            xformCache );

    // If we are writing points for an overlay but not writing transforms, 
    // then we have to transform the points into the proper space.
    bool transformPoints = 
        writeOverlay && ctxt.overlayPoints && 
        !GusdUT_Gf::Cast(loc_xform).isIdentity();

    GT_Owner attrOwner = GT_OWNER_INVALID;
    GT_DataArrayHandle houAttr;
    UsdAttribute usdAttr;
    
    if( !writeOverlay && ctxt.purpose != UsdGeomTokens->default_ ) {
        m_usdCurves.GetPurposeAttr().Set( ctxt.purpose );
    }

    // intrinsic attributes ----------------------------------------------------

    if( !writeOverlay || ctxt.overlayAll || overlayTransforms || ctxt.overlayPoints) {

        // extent ------------------------------------------------------------------
        houAttr = GusdGT_Utils::getExtentsArray(sourcePrim);
        usdAttr = m_usdCurves.GetExtentAttr();
        if(houAttr && usdAttr && transformPoints ) {
             houAttr = GusdGT_Utils::transformPoints( houAttr, loc_xform );
        }
        updateAttributeFromGTPrim( GT_OWNER_INVALID, "extents", houAttr, usdAttr, geoTime );
    }

    // transform ---------------------------------------------------------------
    if( !writeOverlay || ctxt.overlayAll || overlayTransforms) {
        updateTransformFromGTPrim( xform, geoTime, 
                                   ctxt.granularity == GusdContext::PER_FRAME );
    }

    // visibility ---------------------------------------------------------------
    updateVisibilityFromGTPrim(sourcePrim, geoTime, 
                               (!ctxt.writeOverlay || ctxt.overlayAll) && 
                                ctxt.granularity == GusdContext::PER_FRAME );

    if( !writeOverlay || ctxt.overlayAll || ctxt.overlayPoints ) {
        
        // P
        houAttr = sourcePrim->findAttribute("P", attrOwner, 0);
        usdAttr = m_usdCurves.GetPointsAttr();
        if(houAttr && usdAttr && transformPoints ) {
            houAttr = GusdGT_Utils::transformPoints( houAttr, loc_xform );
        }
        updateAttributeFromGTPrim( attrOwner, "P", houAttr, usdAttr, geoTime );
    }
    
    if( !writeOverlay || ctxt.overlayAll ) {

        UsdTimeCode topologyTime = ctxt.time;
        if( ctxt.writeStaticTopology ) {
            topologyTime = UsdTimeCode::Default();
        }

        // Vertex counts
        houAttr = gtCurves->getCurveCounts();
        usdAttr = m_usdCurves.GetCurveVertexCountsAttr();

        // Houdini repeats point for closed beziers so we need to 
        if( order == 4 && closed ) {
            auto modCounts = new GT_Real32Array( houAttr->entries(), 1 );
            for( GT_Size i = 0; i < houAttr->entries(); ++i ) { 
                modCounts->set( houAttr->getValue<fpreal32>( i ) - 4, i );
            }
            houAttr = modCounts;
        }
        updateAttributeFromGTPrim( GT_OWNER_INVALID, "vertexcounts",
                                   houAttr, usdAttr, topologyTime );
    }

    if( !writeOverlay || ctxt.overlayAll || ctxt.overlayPoints ) {
        // N
        houAttr = sourcePrim->findAttribute("N", attrOwner, 0);
        usdAttr = m_usdCurves.GetNormalsAttr();
        updateAttributeFromGTPrim( attrOwner, "N", houAttr, usdAttr, geoTime );

        // v
        houAttr = sourcePrim->findAttribute("v", attrOwner, 0);
        usdAttr = m_usdCurves.GetVelocitiesAttr();
        updateAttributeFromGTPrim( attrOwner, "v", houAttr, usdAttr, geoTime );
        
        // pscale & width
        houAttr = sourcePrim->findAttribute("width", attrOwner, 0);
        if(!houAttr) {
            houAttr = sourcePrim->findAttribute("pscale", attrOwner, 0);
        }

        usdAttr = m_usdCurves.GetWidthsAttr();
        updateAttributeFromGTPrim( attrOwner, "width", houAttr, usdAttr, geoTime );
        m_usdCurves.SetWidthsInterpolation( UsdGeomTokens->vertex );
    }

    // -------------------------------------------------------------------------
    
    // primvars ----------------------------------------------------------------
    
    if( !writeOverlay || ctxt.overlayAll || ctxt.overlayPrimvars ) {

        UsdTimeCode primvarTime = ctxt.time;
        if( ctxt.writeStaticPrimvars ) {
            primvarTime = UsdTimeCode::Default();
        }
        
        // TODO check that varying & facevarying working -- houdini might not support
        // facevarying through GT
        GusdGT_AttrFilter filter = ctxt.attributeFilter;

        filter.appendPattern(GT_OWNER_VERTEX, "^P ^N ^v ^width ^pscale");
        if(const GT_AttributeListHandle vtxAttrs = sourcePrim->getVertexAttributes()) {
            GusdGT_AttrFilter::OwnerArgs owners;
            owners << GT_OWNER_VERTEX;
            filter.setActiveOwners(owners);
            updatePrimvarFromGTPrim( vtxAttrs, filter, UsdGeomTokens->vertex, primvarTime );
        }
        if(const GT_AttributeListHandle constAttrs = sourcePrim->getDetailAttributes()) {
            GusdGT_AttrFilter::OwnerArgs owners;
            owners << GT_OWNER_CONSTANT;
            filter.setActiveOwners(owners);
            updatePrimvarFromGTPrim( constAttrs, filter, UsdGeomTokens->constant, primvarTime );
        }
        if(const GT_AttributeListHandle uniformAttrs = sourcePrim->getUniformAttributes()) {
            GusdGT_AttrFilter::OwnerArgs owners;
            owners << GT_OWNER_UNIFORM;
            filter.setActiveOwners(owners);
            updatePrimvarFromGTPrim( uniformAttrs, filter, UsdGeomTokens->uniform, primvarTime );
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
            updatePrimvarFromGTPrim( attrList, filter, s_ownerToUsdInterpCurve[own], primvarTime );
        }
        // If we have a "Alpha" attribute, write it as both "Alpha" and "displayAlpha".
        if(GT_DataArrayHandle Alpha = sourcePrim->findAttribute( "Alpha", own, 0 )) {

            GT_AttributeMapHandle attrMap = new GT_AttributeMap();
            GT_AttributeListHandle attrList = new GT_AttributeList( attrMap );
            attrList = attrList->addAttribute( "displayOpacity", Alpha, true );
            GusdGT_AttrFilter filter( "*" );
            GusdGT_AttrFilter::OwnerArgs owners;
            owners << own;
            filter.setActiveOwners(owners);
            updatePrimvarFromGTPrim( attrList, filter, s_ownerToUsdInterpCurve[own], primvarTime );
        }
    }

    // -------------------------------------------------------------------------
    return GusdPrimWrapper::updateFromGTPrim(sourcePrim, houXform, ctxt, xformCache);
}

PXR_NAMESPACE_CLOSE_SCOPE


