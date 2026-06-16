//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdSolid/brepArray.h"
#include "pxr/usd/usdSolid/tokens.h"
#include "pxr/usdValidation/usdSolidValidators/validatorTokens.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/timeRange.h"
#include "pxr/usdValidation/usdValidation/validator.h"

#include <cmath>
#include <cstddef>
#include <set>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Tolerance used for unit-length and orthogonality checks on analytic
// surface axis frames.
constexpr double _FrameTol = 1e-6;
// pi/2, used to bound cone semiAngle.
constexpr double _HalfPi = 1.5707963267948966;

UsdValidationErrorSites
_PrimSites(const UsdPrim &prim)
{
    return { UsdValidationErrorSite(prim.GetStage(), prim.GetPath()) };
}

template <class T>
VtArray<T>
_Read(const UsdAttribute &attr)
{
    VtArray<T> value;
    if (attr) {
        attr.Get(&value);
    }
    return value;
}

size_t
_Sum(const VtArray<unsigned int> &values)
{
    size_t sum = 0;
    for (const unsigned int v : values) {
        sum += v;
    }
    return sum;
}

void
_CheckSize(const UsdPrim &prim, const char *attrName, size_t actual,
           size_t expected, const std::string &expectedDesc,
           const TfToken &errorName, UsdValidationErrorVector *errors)
{
    if (actual != expected) {
        errors->emplace_back(
            errorName, UsdValidationErrorType::Error, _PrimSites(prim),
            TfStringPrintf(
                "BrepArray <%s>: attribute %s has size %zu but expected %zu "
                "(%s).",
                prim.GetPath().GetText(), attrName, actual, expected,
                expectedDesc.c_str()));
    }
}

void
_CheckAllowedTokens(const UsdPrim &prim, const VtArray<TfToken> &values,
                    const std::set<TfToken> &allowed, const char *attrName,
                    const TfToken &errorName,
                    UsdValidationErrorVector *errors)
{
    for (size_t i = 0; i < values.size(); ++i) {
        if (allowed.find(values[i]) == allowed.end()) {
            errors->emplace_back(
                errorName, UsdValidationErrorType::Error, _PrimSites(prim),
                TfStringPrintf(
                    "BrepArray <%s>: %s value '%s' at index %zu is not an "
                    "allowed token.",
                    prim.GetPath().GetText(), attrName,
                    values[i].GetText(), i));
        }
    }
}

// -------------------------------------------------------------------------- //
// BrepArrayStructure                                                         //
// -------------------------------------------------------------------------- //
UsdValidationErrorVector
_BrepArrayStructure(const UsdPrim &usdPrim,
                    const UsdValidationTimeRange & /*timeRange*/)
{
    if (!(usdPrim && usdPrim.IsA<UsdSolidBrepArray>())) {
        return {};
    }
    const UsdSolidBrepArray brep(usdPrim);

    const UsdAttribute tolAttr = brep.GetBrepIntersectTol3dAttr();
    const UsdAttribute extentAttr = brep.GetBrepExtentAttr();
    const UsdAttribute regionCountAttr = brep.GetBrepRegionCountAttr();

    UsdValidationErrorVector errors;

    // BA.005: required Brep schema attributes must be authored.
    std::vector<std::string> missing;
    if (!tolAttr.HasAuthoredValue()) {
        missing.emplace_back("brep:intersectTol3d");
    }
    if (!extentAttr.HasAuthoredValue()) {
        missing.emplace_back("brep:extent");
    }
    if (!regionCountAttr.HasAuthoredValue()) {
        missing.emplace_back("brep:regionCount");
    }
    if (!missing.empty()) {
        errors.emplace_back(
            UsdSolidValidationErrorNameTokens->missingBrepAttributes,
            UsdValidationErrorType::Error, _PrimSites(usdPrim),
            TfStringPrintf(
                "BrepArray <%s> is missing required brep attribute(s): %s.",
                usdPrim.GetPath().GetText(),
                TfStringJoin(missing, ", ").c_str()));
    }

    const VtArray<double> tol = _Read<double>(tolAttr);
    const VtArray<GfVec3d> extent = _Read<GfVec3d>(extentAttr);
    const VtArray<unsigned int> regionCount
        = _Read<unsigned int>(regionCountAttr);

    // BA.000 / BA.020: array sizes must be consistent with the number of
    // Breps. brep:regionCount and brep:intersectTol3d have one entry per Brep;
    // brep:extent has two (min, max corner) entries per Brep.
    const size_t numBreps = regionCount.size();
    if (tol.size() != numBreps || extent.size() != 2 * numBreps) {
        errors.emplace_back(
            UsdSolidValidationErrorNameTokens->inconsistentBrepArraySizes,
            UsdValidationErrorType::Error, _PrimSites(usdPrim),
            TfStringPrintf(
                "BrepArray <%s>: brep-level array sizes are inconsistent. For "
                "%zu Brep(s) (brep:regionCount size), expected "
                "brep:intersectTol3d size %zu (got %zu) and brep:extent size "
                "%zu (got %zu).",
                usdPrim.GetPath().GetText(), numBreps, numBreps, tol.size(),
                2 * numBreps, extent.size()));
    }

    // BA.010: brep:intersectTol3d values must be positive.
    for (size_t i = 0; i < tol.size(); ++i) {
        if (tol[i] <= 0.0) {
            errors.emplace_back(
                UsdSolidValidationErrorNameTokens->nonPositiveIntersectTol3d,
                UsdValidationErrorType::Error, _PrimSites(usdPrim),
                TfStringPrintf(
                    "BrepArray <%s>: brep:intersectTol3d[%zu] = %g is not "
                    "positive.",
                    usdPrim.GetPath().GetText(), i, tol[i]));
        }
    }

    // BA.025 / BA.030 / BA.035: each brep:extent bounding box corner pair must
    // be ordered min <= max on every axis.
    const char *const axisNames[3] = { "X", "Y", "Z" };
    for (size_t box = 0; 2 * box + 1 < extent.size(); ++box) {
        const GfVec3d &mn = extent[2 * box];
        const GfVec3d &mx = extent[2 * box + 1];
        for (int a = 0; a < 3; ++a) {
            if (mn[a] > mx[a]) {
                errors.emplace_back(
                    UsdSolidValidationErrorNameTokens->invalidExtentOrder,
                    UsdValidationErrorType::Error, _PrimSites(usdPrim),
                    TfStringPrintf(
                        "BrepArray <%s>: brep:extent for Brep %zu has %smin "
                        "(%g) > %smax (%g).",
                        usdPrim.GetPath().GetText(), box, axisNames[a],
                        mn[a], axisNames[a], mx[a]));
            }
        }
    }

    return errors;
}

// -------------------------------------------------------------------------- //
// BrepArrayTopology                                                          //
// -------------------------------------------------------------------------- //
UsdValidationErrorVector
_BrepArrayTopology(const UsdPrim &usdPrim,
                   const UsdValidationTimeRange & /*timeRange*/)
{
    if (!(usdPrim && usdPrim.IsA<UsdSolidBrepArray>())) {
        return {};
    }
    const UsdSolidBrepArray brep(usdPrim);

    const UsdAttribute regionCountAttr = brep.GetBrepRegionCountAttr();
    if (!regionCountAttr.HasAuthoredValue()) {
        // The BrepArrayStructure validator reports the missing attribute; we
        // cannot derive the topology sizes without it.
        return {};
    }

    UsdValidationErrorVector errors;

    const VtArray<unsigned int> regionCount
        = _Read<unsigned int>(regionCountAttr);

    // Region (BA.065): sized by sum(brep:regionCount).
    const size_t numRegions = _Sum(regionCount);
    const VtArray<unsigned int> regionShellCount
        = _Read<unsigned int>(brep.GetRegionShellCountAttr());
    const VtArray<TfToken> regionType
        = _Read<TfToken>(brep.GetRegionTypeAttr());
    const std::string regionsDesc
        = TfStringPrintf("sum of brep:regionCount = %zu", numRegions);
    _CheckSize(usdPrim, "region:shellCount", regionShellCount.size(),
               numRegions, regionsDesc,
               UsdSolidValidationErrorNameTokens->inconsistentRegionArraySizes,
               &errors);
    _CheckSize(usdPrim, "region:type", regionType.size(), numRegions,
               regionsDesc,
               UsdSolidValidationErrorNameTokens->inconsistentRegionArraySizes,
               &errors);

    // Shell (BA.080): sized by sum(region:shellCount).
    const size_t numShells = _Sum(regionShellCount);
    const VtArray<unsigned int> shellFaceuseCount
        = _Read<unsigned int>(brep.GetShellFaceuseCountAttr());
    const VtArray<unsigned int> shellWireEdgeCount
        = _Read<unsigned int>(brep.GetShellWireEdgeCountAttr());
    const VtArray<TfToken> shellPointType
        = _Read<TfToken>(brep.GetShellPointTypeAttr());
    const std::string shellsDesc
        = TfStringPrintf("sum of region:shellCount = %zu", numShells);
    _CheckSize(usdPrim, "shell:faceuseCount", shellFaceuseCount.size(),
               numShells, shellsDesc,
               UsdSolidValidationErrorNameTokens->inconsistentShellArraySizes,
               &errors);
    _CheckSize(usdPrim, "shell:wireEdgeCount", shellWireEdgeCount.size(),
               numShells, shellsDesc,
               UsdSolidValidationErrorNameTokens->inconsistentShellArraySizes,
               &errors);
    _CheckSize(usdPrim, "shell:pointType", shellPointType.size(), numShells,
               shellsDesc,
               UsdSolidValidationErrorNameTokens->inconsistentShellArraySizes,
               &errors);

    // Faceuse (BA.100): sized by sum(shell:faceuseCount).
    const size_t numFaceuses = _Sum(shellFaceuseCount);
    const VtArray<unsigned int> faceuseFaceIndex
        = _Read<unsigned int>(brep.GetFaceuseFaceIndexAttr());
    const VtArray<TfToken> faceuseOrientationType
        = _Read<TfToken>(brep.GetFaceuseOrientationTypeAttr());
    const std::string faceusesDesc
        = TfStringPrintf("sum of shell:faceuseCount = %zu", numFaceuses);
    _CheckSize(usdPrim, "faceuse:faceIndex", faceuseFaceIndex.size(),
               numFaceuses, faceusesDesc,
               UsdSolidValidationErrorNameTokens
                   ->inconsistentFaceuseArraySizes,
               &errors);
    _CheckSize(usdPrim, "faceuse:orientationType",
               faceuseOrientationType.size(), numFaceuses, faceusesDesc,
               UsdSolidValidationErrorNameTokens
                   ->inconsistentFaceuseArraySizes,
               &errors);

    // Face (BA.120 / BA.150): faceuses come in pairs, so there are
    // numFaceuses / 2 faces. face:range has two (UVmin, UVmax) entries per
    // face.
    const size_t numFaces = numFaceuses / 2;
    const VtArray<unsigned int> faceLoopCount
        = _Read<unsigned int>(brep.GetFaceLoopCountAttr());
    const VtArray<TfToken> faceSurfaceType
        = _Read<TfToken>(brep.GetFaceSurfaceTypeAttr());
    const VtArray<TfToken> faceTrimType
        = _Read<TfToken>(brep.GetFaceTrimTypeAttr());
    const VtArray<GfVec2d> faceRange = _Read<GfVec2d>(brep.GetFaceRangeAttr());
    const std::string facesDesc
        = TfStringPrintf("faceuse count / 2 = %zu", numFaces);
    _CheckSize(usdPrim, "face:loopCount", faceLoopCount.size(), numFaces,
               facesDesc,
               UsdSolidValidationErrorNameTokens->inconsistentFaceArraySizes,
               &errors);
    _CheckSize(usdPrim, "face:surfaceType", faceSurfaceType.size(), numFaces,
               facesDesc,
               UsdSolidValidationErrorNameTokens->inconsistentFaceArraySizes,
               &errors);
    _CheckSize(usdPrim, "face:trimType", faceTrimType.size(), numFaces,
               facesDesc,
               UsdSolidValidationErrorNameTokens->inconsistentFaceArraySizes,
               &errors);
    _CheckSize(usdPrim, "face:range", faceRange.size(), 2 * numFaces,
               TfStringPrintf("2 * number of faces = %zu", 2 * numFaces),
               UsdSolidValidationErrorNameTokens->inconsistentFaceArraySizes,
               &errors);

    // Loop (BA.165): sized by sum(face:loopCount).
    const size_t numLoops = _Sum(faceLoopCount);
    const VtArray<unsigned int> loopEdgeuseCount
        = _Read<unsigned int>(brep.GetLoopEdgeuseCountAttr());
    const VtArray<unsigned int> loopVertexIndex
        = _Read<unsigned int>(brep.GetLoopVertexIndexAttr());
    const std::string loopsDesc
        = TfStringPrintf("sum of face:loopCount = %zu", numLoops);
    _CheckSize(usdPrim, "loop:edgeuseCount", loopEdgeuseCount.size(), numLoops,
               loopsDesc,
               UsdSolidValidationErrorNameTokens->inconsistentLoopArraySizes,
               &errors);
    _CheckSize(usdPrim, "loop:vertexIndex", loopVertexIndex.size(), numLoops,
               loopsDesc,
               UsdSolidValidationErrorNameTokens->inconsistentLoopArraySizes,
               &errors);

    // Edgeuse (BA.180): sized by sum(loop:edgeuseCount).
    const size_t numEdgeuses = _Sum(loopEdgeuseCount);
    const std::string edgeusesDesc
        = TfStringPrintf("sum of loop:edgeuseCount = %zu", numEdgeuses);
    _CheckSize(usdPrim, "edgeuse:edgeIndex",
               _Read<unsigned int>(brep.GetEdgeuseEdgeIndexAttr()).size(),
               numEdgeuses, edgeusesDesc,
               UsdSolidValidationErrorNameTokens
                   ->inconsistentEdgeuseArraySizes,
               &errors);
    _CheckSize(usdPrim, "edgeuse:orientationType",
               _Read<TfToken>(brep.GetEdgeuseOrientationTypeAttr()).size(),
               numEdgeuses, edgeusesDesc,
               UsdSolidValidationErrorNameTokens
                   ->inconsistentEdgeuseArraySizes,
               &errors);
    _CheckSize(usdPrim, "edgeuse:nextRadialEUIndex",
               _Read<unsigned int>(
                   brep.GetEdgeuseNextRadialEUIndexAttr()).size(),
               numEdgeuses, edgeusesDesc,
               UsdSolidValidationErrorNameTokens
                   ->inconsistentEdgeuseArraySizes,
               &errors);
    _CheckSize(usdPrim, "edgeuse:thisRadialEntryType",
               _Read<TfToken>(
                   brep.GetEdgeuseThisRadialEntryTypeAttr()).size(),
               numEdgeuses, edgeusesDesc,
               UsdSolidValidationErrorNameTokens
                   ->inconsistentEdgeuseArraySizes,
               &errors);

    // Edge (BA.210): the number of edges is defined by edge:curveType. The
    // remaining edge arrays must match, and edge:range has two entries per
    // edge.
    const VtArray<TfToken> edgeCurveType
        = _Read<TfToken>(brep.GetEdgeCurveTypeAttr());
    const size_t numEdges = edgeCurveType.size();
    _CheckSize(usdPrim, "edge:vertexIndices",
               _Read<GfVec2i>(brep.GetEdgeVertexIndicesAttr()).size(),
               numEdges,
               TfStringPrintf("number of edges = %zu", numEdges),
               UsdSolidValidationErrorNameTokens->inconsistentEdgeArraySizes,
               &errors);
    _CheckSize(usdPrim, "edge:range",
               _Read<double>(brep.GetEdgeRangeAttr()).size(), 2 * numEdges,
               TfStringPrintf("2 * number of edges = %zu", 2 * numEdges),
               UsdSolidValidationErrorNameTokens->inconsistentEdgeArraySizes,
               &errors);

    // WireEdge (BA.250): sized by sum(shell:wireEdgeCount).
    const size_t numWireEdges = _Sum(shellWireEdgeCount);
    const std::string wireEdgesDesc
        = TfStringPrintf("sum of shell:wireEdgeCount = %zu", numWireEdges);
    _CheckSize(usdPrim, "wireEdge:curveType",
               _Read<TfToken>(brep.GetWireEdgeCurveTypeAttr()).size(),
               numWireEdges, wireEdgesDesc,
               UsdSolidValidationErrorNameTokens
                   ->inconsistentWireEdgeArraySizes,
               &errors);
    _CheckSize(usdPrim, "wireEdge:vertexIndices",
               _Read<GfVec2i>(brep.GetWireEdgeVertexIndicesAttr()).size(),
               numWireEdges, wireEdgesDesc,
               UsdSolidValidationErrorNameTokens
                   ->inconsistentWireEdgeArraySizes,
               &errors);
    _CheckSize(usdPrim, "wireEdge:range",
               _Read<double>(brep.GetWireEdgeRangeAttr()).size(),
               2 * numWireEdges,
               TfStringPrintf("2 * sum of shell:wireEdgeCount = %zu",
                              2 * numWireEdges),
               UsdSolidValidationErrorNameTokens
                   ->inconsistentWireEdgeArraySizes,
               &errors);

    return errors;
}

// -------------------------------------------------------------------------- //
// BrepArrayTokenValues                                                       //
// -------------------------------------------------------------------------- //
UsdValidationErrorVector
_BrepArrayTokenValues(const UsdPrim &usdPrim,
                      const UsdValidationTimeRange & /*timeRange*/)
{
    if (!(usdPrim && usdPrim.IsA<UsdSolidBrepArray>())) {
        return {};
    }
    const UsdSolidBrepArray brep(usdPrim);

    static const std::set<TfToken> regionTypes
        = { TfToken("solidRegion"), TfToken("voidRegion") };
    static const std::set<TfToken> shellPointTypes
        = { TfToken("BrepPointAPI"), TfToken("none") };
    static const std::set<TfToken> orientationTypes
        = { TfToken("same"), TfToken("opposite") };
    static const std::set<TfToken> surfaceTypes
        = { TfToken("BrepSurfaceNurbAPI"), TfToken("BrepSurfacePlaneAPI"),
            TfToken("BrepSurfaceCylinderAPI"), TfToken("BrepSurfaceConeAPI"),
            TfToken("BrepSurfaceSphereAPI"), TfToken("BrepSurfaceTorusAPI") };
    static const std::set<TfToken> trimTypes
        = { TfToken("rectangular"), TfToken("general") };
    static const std::set<TfToken> radialEntryTypes
        = { TfToken("topEntry"), TfToken("bottomEntry") };
    static const std::set<TfToken> curveTypes
        = { TfToken("BrepCurve3dNurbAPI"), TfToken("BrepCurve3dLineAPI"),
            TfToken("BrepCurve3dCircleAPI"),
            TfToken("BrepCurve3dEllipseAPI") };
    static const std::set<TfToken> vertexPointTypes
        = { TfToken("BrepPointAPI") };

    UsdValidationErrorVector errors;

    _CheckAllowedTokens(usdPrim, _Read<TfToken>(brep.GetRegionTypeAttr()),
                        regionTypes, "region:type",
                        UsdSolidValidationErrorNameTokens->invalidRegionType,
                        &errors);
    _CheckAllowedTokens(
        usdPrim, _Read<TfToken>(brep.GetShellPointTypeAttr()), shellPointTypes,
        "shell:pointType",
        UsdSolidValidationErrorNameTokens->invalidShellPointType, &errors);
    _CheckAllowedTokens(
        usdPrim, _Read<TfToken>(brep.GetFaceuseOrientationTypeAttr()),
        orientationTypes, "faceuse:orientationType",
        UsdSolidValidationErrorNameTokens->invalidFaceuseOrientationType,
        &errors);
    _CheckAllowedTokens(
        usdPrim, _Read<TfToken>(brep.GetFaceSurfaceTypeAttr()), surfaceTypes,
        "face:surfaceType",
        UsdSolidValidationErrorNameTokens->invalidFaceSurfaceType, &errors);
    _CheckAllowedTokens(
        usdPrim, _Read<TfToken>(brep.GetFaceTrimTypeAttr()), trimTypes,
        "face:trimType",
        UsdSolidValidationErrorNameTokens->invalidFaceTrimType, &errors);
    _CheckAllowedTokens(
        usdPrim, _Read<TfToken>(brep.GetEdgeuseOrientationTypeAttr()),
        orientationTypes, "edgeuse:orientationType",
        UsdSolidValidationErrorNameTokens->invalidEdgeuseOrientationType,
        &errors);
    _CheckAllowedTokens(
        usdPrim, _Read<TfToken>(brep.GetEdgeuseThisRadialEntryTypeAttr()),
        radialEntryTypes, "edgeuse:thisRadialEntryType",
        UsdSolidValidationErrorNameTokens->invalidEdgeuseRadialEntryType,
        &errors);
    _CheckAllowedTokens(
        usdPrim, _Read<TfToken>(brep.GetEdgeCurveTypeAttr()), curveTypes,
        "edge:curveType",
        UsdSolidValidationErrorNameTokens->invalidEdgeCurveType, &errors);
    _CheckAllowedTokens(
        usdPrim, _Read<TfToken>(brep.GetWireEdgeCurveTypeAttr()), curveTypes,
        "wireEdge:curveType",
        UsdSolidValidationErrorNameTokens->invalidWireEdgeCurveType, &errors);
    _CheckAllowedTokens(
        usdPrim, _Read<TfToken>(brep.GetVertexPointTypeAttr()),
        vertexPointTypes, "vertex:pointType",
        UsdSolidValidationErrorNameTokens->invalidVertexPointType, &errors);

    return errors;
}

// -------------------------------------------------------------------------- //
// BrepArrayRanges                                                            //
// -------------------------------------------------------------------------- //
UsdValidationErrorVector
_BrepArrayRanges(const UsdPrim &usdPrim,
                 const UsdValidationTimeRange & /*timeRange*/)
{
    if (!(usdPrim && usdPrim.IsA<UsdSolidBrepArray>())) {
        return {};
    }
    const UsdSolidBrepArray brep(usdPrim);

    UsdValidationErrorVector errors;

    // BA.140: every face must have at least one loop.
    const VtArray<unsigned int> faceLoopCount
        = _Read<unsigned int>(brep.GetFaceLoopCountAttr());
    for (size_t i = 0; i < faceLoopCount.size(); ++i) {
        if (faceLoopCount[i] < 1) {
            errors.emplace_back(
                UsdSolidValidationErrorNameTokens->invalidFaceLoopCount,
                UsdValidationErrorType::Error, _PrimSites(usdPrim),
                TfStringPrintf(
                    "BrepArray <%s>: face:loopCount[%zu] = 0; each face must "
                    "have at least one loop.",
                    usdPrim.GetPath().GetText(), i));
        }
    }

    // BA.155 / BA.160: face:range is stored as (UVmin, UVmax) pairs; the U and
    // V intervals must each be non-degenerate (max > min).
    const VtArray<GfVec2d> faceRange = _Read<GfVec2d>(brep.GetFaceRangeAttr());
    for (size_t face = 0; 2 * face + 1 < faceRange.size(); ++face) {
        const GfVec2d &uvMin = faceRange[2 * face];
        const GfVec2d &uvMax = faceRange[2 * face + 1];
        if (uvMax[0] <= uvMin[0]) {
            errors.emplace_back(
                UsdSolidValidationErrorNameTokens->degenerateFaceURange,
                UsdValidationErrorType::Error, _PrimSites(usdPrim),
                TfStringPrintf(
                    "BrepArray <%s>: face:range for face %zu has degenerate U "
                    "interval (Umin %g >= Umax %g).",
                    usdPrim.GetPath().GetText(), face, uvMin[0], uvMax[0]));
        }
        if (uvMax[1] <= uvMin[1]) {
            errors.emplace_back(
                UsdSolidValidationErrorNameTokens->degenerateFaceVRange,
                UsdValidationErrorType::Error, _PrimSites(usdPrim),
                TfStringPrintf(
                    "BrepArray <%s>: face:range for face %zu has degenerate V "
                    "interval (Vmin %g >= Vmax %g).",
                    usdPrim.GetPath().GetText(), face, uvMin[1], uvMax[1]));
        }
    }

    // BA.235: each edge:range (min, max) pair must be ordered.
    const VtArray<double> edgeRange = _Read<double>(brep.GetEdgeRangeAttr());
    for (size_t edge = 0; 2 * edge + 1 < edgeRange.size(); ++edge) {
        if (edgeRange[2 * edge] > edgeRange[2 * edge + 1]) {
            errors.emplace_back(
                UsdSolidValidationErrorNameTokens->invalidEdgeRangeOrder,
                UsdValidationErrorType::Error, _PrimSites(usdPrim),
                TfStringPrintf(
                    "BrepArray <%s>: edge:range for edge %zu is not ordered "
                    "(min %g > max %g).",
                    usdPrim.GetPath().GetText(), edge, edgeRange[2 * edge],
                    edgeRange[2 * edge + 1]));
        }
    }

    // BA.275: each wireEdge:range (min, max) pair must be ordered.
    const VtArray<double> wireEdgeRange
        = _Read<double>(brep.GetWireEdgeRangeAttr());
    for (size_t edge = 0; 2 * edge + 1 < wireEdgeRange.size(); ++edge) {
        if (wireEdgeRange[2 * edge] > wireEdgeRange[2 * edge + 1]) {
            errors.emplace_back(
                UsdSolidValidationErrorNameTokens->invalidWireEdgeRangeOrder,
                UsdValidationErrorType::Error, _PrimSites(usdPrim),
                TfStringPrintf(
                    "BrepArray <%s>: wireEdge:range for wireEdge %zu is not "
                    "ordered (min %g > max %g).",
                    usdPrim.GetPath().GetText(), edge,
                    wireEdgeRange[2 * edge], wireEdgeRange[2 * edge + 1]));
        }
    }

    return errors;
}

// -------------------------------------------------------------------------- //
// BrepArrayAnalyticSurfaces                                                  //
// -------------------------------------------------------------------------- //

// A radius-like parameter and whether it must be strictly positive (false =>
// non-negative is sufficient, as for cone apex radius).
struct _RadiusParam {
    TfToken attr;
    const char *name;
    bool strictlyPositive;
};

// Description of one analytic surface type's parameter attributes.
struct _SurfaceDesc {
    TfToken faceSurfaceType;     // face:surfaceType token value
    const char *label;           // human-readable surface type
    TfToken axisAttr;            // surface frame axis (unit)
    const char *axisName;
    TfToken refDirAttr;          // surface frame reference direction (unit)
    const char *refDirName;
    std::vector<_RadiusParam> radii;
    TfToken semiAngleAttr;       // empty token unless a cone
    const char *semiAngleName;
};

void
_CheckAnalyticSurface(const UsdPrim &usdPrim, const _SurfaceDesc &desc,
                      size_t count, UsdValidationErrorVector *errors)
{
    const VtArray<GfVec3d> axis
        = _Read<GfVec3d>(usdPrim.GetAttribute(desc.axisAttr));
    const VtArray<GfVec3d> refDir
        = _Read<GfVec3d>(usdPrim.GetAttribute(desc.refDirAttr));

    const std::string countDesc = TfStringPrintf(
        "number of faces with face:surfaceType '%s' = %zu", desc.label, count);

    // BA.480/490/500/510/520: parameter array sizes must match the face count.
    _CheckSize(usdPrim, desc.axisName, axis.size(), count, countDesc,
               UsdSolidValidationErrorNameTokens
                   ->inconsistentAnalyticSurfaceCount,
               errors);
    _CheckSize(usdPrim, desc.refDirName, refDir.size(), count, countDesc,
               UsdSolidValidationErrorNameTokens
                   ->inconsistentAnalyticSurfaceCount,
               errors);

    // BA.481/501/511/521/522: radius positivity (or non-negativity).
    for (const _RadiusParam &radius : desc.radii) {
        const VtArray<double> values
            = _Read<double>(usdPrim.GetAttribute(radius.attr));
        _CheckSize(usdPrim, radius.name, values.size(), count, countDesc,
                   UsdSolidValidationErrorNameTokens
                       ->inconsistentAnalyticSurfaceCount,
                   errors);
        for (size_t i = 0; i < values.size(); ++i) {
            const bool bad = radius.strictlyPositive ? (values[i] <= 0.0)
                                                     : (values[i] < 0.0);
            if (bad) {
                errors->emplace_back(
                    UsdSolidValidationErrorNameTokens
                        ->nonPositiveSurfaceRadius,
                    UsdValidationErrorType::Error, _PrimSites(usdPrim),
                    TfStringPrintf(
                        "BrepArray <%s>: %s surface %s[%zu] = %g must be %s.",
                        usdPrim.GetPath().GetText(), desc.label, radius.name,
                        i, values[i],
                        radius.strictlyPositive ? "positive"
                                                : "non-negative"));
            }
        }
    }

    // BA.482/483 (and analogues): axis and refDirection must be unit length.
    for (size_t i = 0; i < axis.size(); ++i) {
        if (std::abs(axis[i].GetLength() - 1.0) > _FrameTol) {
            errors->emplace_back(
                UsdSolidValidationErrorNameTokens->nonUnitSurfaceAxis,
                UsdValidationErrorType::Error, _PrimSites(usdPrim),
                TfStringPrintf(
                    "BrepArray <%s>: %s surface %s[%zu] is not unit length "
                    "(length %g).",
                    usdPrim.GetPath().GetText(), desc.label, desc.axisName, i,
                    axis[i].GetLength()));
        }
    }
    for (size_t i = 0; i < refDir.size(); ++i) {
        if (std::abs(refDir[i].GetLength() - 1.0) > _FrameTol) {
            errors->emplace_back(
                UsdSolidValidationErrorNameTokens->nonUnitSurfaceRefDirection,
                UsdValidationErrorType::Error, _PrimSites(usdPrim),
                TfStringPrintf(
                    "BrepArray <%s>: %s surface %s[%zu] is not unit length "
                    "(length %g).",
                    usdPrim.GetPath().GetText(), desc.label, desc.refDirName,
                    i, refDir[i].GetLength()));
        }
    }

    // BA.484 (and analogues): axis and refDirection must be orthogonal.
    const size_t frameCount = std::min(axis.size(), refDir.size());
    for (size_t i = 0; i < frameCount; ++i) {
        const double dot = GfDot(axis[i], refDir[i]);
        if (std::abs(dot) > _FrameTol) {
            errors->emplace_back(
                UsdSolidValidationErrorNameTokens->nonOrthogonalSurfaceAxes,
                UsdValidationErrorType::Error, _PrimSites(usdPrim),
                TfStringPrintf(
                    "BrepArray <%s>: %s surface %s and %s at index %zu are not "
                    "orthogonal (dot product %g).",
                    usdPrim.GetPath().GetText(), desc.label, desc.axisName,
                    desc.refDirName, i, dot));
        }
    }

    // BA.515: cone semiAngle must lie in the open interval (0, pi/2).
    if (!desc.semiAngleAttr.IsEmpty()) {
        const VtArray<double> semiAngle
            = _Read<double>(usdPrim.GetAttribute(desc.semiAngleAttr));
        _CheckSize(usdPrim, desc.semiAngleName, semiAngle.size(), count,
                   countDesc,
                   UsdSolidValidationErrorNameTokens
                       ->inconsistentAnalyticSurfaceCount,
                   errors);
        for (size_t i = 0; i < semiAngle.size(); ++i) {
            if (semiAngle[i] <= 0.0 || semiAngle[i] >= _HalfPi) {
                errors->emplace_back(
                    UsdSolidValidationErrorNameTokens->invalidConeSemiAngle,
                    UsdValidationErrorType::Error, _PrimSites(usdPrim),
                    TfStringPrintf(
                        "BrepArray <%s>: %s surface %s[%zu] = %g must lie in "
                        "the open interval (0, pi/2).",
                        usdPrim.GetPath().GetText(), desc.label,
                        desc.semiAngleName, i, semiAngle[i]));
            }
        }
    }
}

UsdValidationErrorVector
_BrepArrayAnalyticSurfaces(const UsdPrim &usdPrim,
                           const UsdValidationTimeRange & /*timeRange*/)
{
    if (!(usdPrim && usdPrim.IsA<UsdSolidBrepArray>())) {
        return {};
    }
    const UsdSolidBrepArray brep(usdPrim);

    const VtArray<TfToken> faceSurfaceType
        = _Read<TfToken>(brep.GetFaceSurfaceTypeAttr());

    const std::vector<_SurfaceDesc> descs = {
        { TfToken("BrepSurfacePlaneAPI"), "plane",
          UsdSolidTokens->brepSurfacePlaneAxis, "brep:surface:plane:axis",
          UsdSolidTokens->brepSurfacePlaneRefDirection,
          "brep:surface:plane:refDirection",
          {},
          TfToken(), nullptr },
        { TfToken("BrepSurfaceCylinderAPI"), "cylinder",
          UsdSolidTokens->brepSurfaceCylinderAxis,
          "brep:surface:cylinder:axis",
          UsdSolidTokens->brepSurfaceCylinderRefDirection,
          "brep:surface:cylinder:refDirection",
          { { UsdSolidTokens->brepSurfaceCylinderRadius,
              "brep:surface:cylinder:radius", true } },
          TfToken(), nullptr },
        { TfToken("BrepSurfaceConeAPI"), "cone",
          UsdSolidTokens->brepSurfaceConeAxis, "brep:surface:cone:axis",
          UsdSolidTokens->brepSurfaceConeRefDirection,
          "brep:surface:cone:refDirection",
          { { UsdSolidTokens->brepSurfaceConeRadius,
              "brep:surface:cone:radius", false } },
          UsdSolidTokens->brepSurfaceConeSemiAngle,
          "brep:surface:cone:semiAngle" },
        { TfToken("BrepSurfaceSphereAPI"), "sphere",
          UsdSolidTokens->brepSurfaceSphereAxis, "brep:surface:sphere:axis",
          UsdSolidTokens->brepSurfaceSphereRefDirection,
          "brep:surface:sphere:refDirection",
          { { UsdSolidTokens->brepSurfaceSphereRadius,
              "brep:surface:sphere:radius", true } },
          TfToken(), nullptr },
        { TfToken("BrepSurfaceTorusAPI"), "torus",
          UsdSolidTokens->brepSurfaceTorusAxis, "brep:surface:torus:axis",
          UsdSolidTokens->brepSurfaceTorusRefDirection,
          "brep:surface:torus:refDirection",
          { { UsdSolidTokens->brepSurfaceTorusMajorRadius,
              "brep:surface:torus:majorRadius", true },
            { UsdSolidTokens->brepSurfaceTorusMinorRadius,
              "brep:surface:torus:minorRadius", true } },
          TfToken(), nullptr },
    };

    UsdValidationErrorVector errors;
    for (const _SurfaceDesc &desc : descs) {
        size_t count = 0;
        for (const TfToken &type : faceSurfaceType) {
            if (type == desc.faceSurfaceType) {
                ++count;
            }
        }
        // Skip work for surface types that are not used and have no authored
        // parameters; _CheckAnalyticSurface emits a size error if parameters
        // are authored without matching faces.
        const VtArray<GfVec3d> axis
            = _Read<GfVec3d>(usdPrim.GetAttribute(desc.axisAttr));
        if (count == 0 && axis.empty()) {
            continue;
        }
        _CheckAnalyticSurface(usdPrim, desc, count, &errors);
    }

    return errors;
}

} // anonymous namespace

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();

    registry.RegisterPluginValidator(
        UsdSolidValidatorNameTokens->brepArrayStructure, _BrepArrayStructure);

    registry.RegisterPluginValidator(
        UsdSolidValidatorNameTokens->brepArrayTopology, _BrepArrayTopology);

    registry.RegisterPluginValidator(
        UsdSolidValidatorNameTokens->brepArrayTokenValues,
        _BrepArrayTokenValues);

    registry.RegisterPluginValidator(
        UsdSolidValidatorNameTokens->brepArrayRanges, _BrepArrayRanges);

    registry.RegisterPluginValidator(
        UsdSolidValidatorNameTokens->brepArrayAnalyticSurfaces,
        _BrepArrayAnalyticSurfaces);
}

PXR_NAMESPACE_CLOSE_SCOPE
