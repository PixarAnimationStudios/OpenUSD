//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include <numeric> // for std::iota
#include "hdPrman/basisCurves.h"

#include "hdPrman/renderParam.h"
#include "hdPrman/material.h"
#include "hdPrman/rixStrings.h"
#include "pxr/imaging/hd/basisCurvesTopology.h"

#include "RiTypesHelper.h"

PXR_NAMESPACE_OPEN_SCOPE

#if PXR_VERSION > 2011
HdPrman_BasisCurves::HdPrman_BasisCurves(SdfPath const& id)
    : BASE(id)
#else
HdPrman_BasisCurves::HdPrman_BasisCurves(SdfPath const& id,
                                         SdfPath const& instancerId)
    : BASE(id, instancerId)
#endif
{
}

HdDirtyBits
HdPrman_BasisCurves::GetInitialDirtyBitsMask() const
{
    // The initial dirty bits control what data is available on the first
    // run through _PopulateRtBasisCurves(), so it should list every data item
    // that _PopluateRtBasisCurves requests.
    int mask = HdChangeTracker::Clean
        | HdChangeTracker::DirtyPoints
        | HdChangeTracker::DirtyTopology
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyNormals
        | HdChangeTracker::DirtyWidths
        | HdChangeTracker::DirtyInstancer
        | HdChangeTracker::DirtyMaterialId
        ;

    return (HdDirtyBits)mask;
}

RtPrimVarList
HdPrman_BasisCurves::_ConvertGeometry(HdPrman_RenderParam *renderParam,
                                      HdSceneDelegate *sceneDelegate,
                                      const SdfPath &id,
                                      RtUString *primType,
                                      std::vector<HdGeomSubset> *geomSubsets)
{
    HdBasisCurvesTopology topology =
        GetBasisCurvesTopology(sceneDelegate);
    VtIntArray curveVertexCounts = topology.GetCurveVertexCounts();
    VtIntArray curveIndices = topology.GetCurveIndices();
    TfToken curveType = topology.GetCurveType();
    TfToken curveBasis = topology.GetCurveBasis();
    TfToken curveWrap = topology.GetCurveWrap();

    *primType = RixStr.k_Ri_Curves;

    // Note: 'nowrap' and 'nsegs' terminology below is to match
    // prman primvar docs, for ease of validation.
    const int numCurves = curveVertexCounts.size();
    const int nowrap = (curveWrap == HdTokens->periodic) ? 0 : 1;
    size_t vertexPrimvarCount = 0;
    size_t varyingPrimvarCount = 0;
    size_t facevaryingPrimvarCount = 0;
    if (curveType == HdTokens->cubic) {
        const int vstep = (curveBasis == HdTokens->bezier) ? 3 : 1;
        for (const int &nvertices: curveVertexCounts) {
            const int nsegs = (curveWrap == HdTokens->periodic) ? 
                nvertices / vstep : (nvertices - 4) / vstep + 1;
            varyingPrimvarCount += nsegs + nowrap;
            vertexPrimvarCount += nvertices;
            facevaryingPrimvarCount += nsegs + nowrap;
        }
    } else if (curveType == HdTokens->linear) {
        for (const int &nvertices: curveVertexCounts) {
            varyingPrimvarCount += nvertices;
            vertexPrimvarCount += nvertices;
            facevaryingPrimvarCount += nvertices;
        }
    } else {
        TF_CODING_ERROR("Unknown curveType %s\n", curveType.GetText());
    }

    RtPrimVarList primvars(
         numCurves, /* uniform */
         vertexPrimvarCount, /* vertex */
         varyingPrimvarCount, /* varying */
         facevaryingPrimvarCount /* facevarying */);

    if (curveType == HdTokens->cubic) {
        primvars.SetString(RixStr.k_Ri_type, RixStr.k_cubic);
        if (curveBasis == HdTokens->cubic) {
            primvars.SetString(RixStr.k_Ri_Basis, RixStr.k_cubic);
        } else if (curveBasis == HdTokens->bSpline) {
            primvars.SetString(RixStr.k_Ri_Basis, RixStr.k_bspline);
        } else if (curveBasis == HdTokens->bezier) {
            primvars.SetString(RixStr.k_Ri_Basis, RixStr.k_bezier);
        } else if (curveBasis == HdTokens->catmullRom) {
            primvars.SetString(RixStr.k_Ri_Basis, RixStr.k_catmullrom);
        } else {
            TF_CODING_ERROR("Unknown curveBasis %s\n", curveBasis.GetText());
        }
    } else if (curveType == HdTokens->linear) {
        primvars.SetString(RixStr.k_Ri_type, RixStr.k_linear);
    } else {
        TF_CODING_ERROR("Unknown curveType %s\n", curveType.GetText());
    }
    if (curveWrap == HdTokens->periodic) {
        primvars.SetString(RixStr.k_Ri_wrap, RixStr.k_periodic);
    } else {
        primvars.SetString(RixStr.k_Ri_wrap, RixStr.k_nonperiodic);
    }

    // Index data
    int const *nverticesData = (int const *)(&curveVertexCounts[0]);
    primvars.SetIntegerDetail(RixStr.k_Ri_nvertices, nverticesData,
                               RtDetailType::k_uniform);

    // Points
    HdPrman_ConvertPointsPrimvar(
        sceneDelegate,
        id,
        renderParam->GetShutterInterval(),
        primvars,
        vertexPrimvarCount);

    // Set element ID.  Overloaded use of "__faceIndex" to support picking...
    std::vector<int32_t> elementId(numCurves);
    std::iota(elementId.begin(), elementId.end(), 0);
    primvars.SetIntegerDetail(RixStr.k_faceindex, elementId.data(),
                               RtDetailType::k_uniform);

    HdPrman_ConvertPrimvars(
        sceneDelegate, id, primvars, numCurves, vertexPrimvarCount,
        varyingPrimvarCount, facevaryingPrimvarCount);

    return primvars;
}

PXR_NAMESPACE_CLOSE_SCOPE
