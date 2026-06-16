//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/timeCode.h"
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
#include <unordered_map>
#include <unordered_set>
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

// ========================================================================== //
// Shared support for the deferred-rule validators                            //
// ========================================================================== //

constexpr double _NurbsTol = 1e-11;   // BA.3xx/4xx weight/knot tolerance
constexpr double _DomainTol = 1e-6;   // BA.56x/57x span tolerance
constexpr double _CurveEps = 1e-4;    // BA.53x/54x/55x unit/orthogonal tolerance
constexpr double _TwoPi = 6.283185307179586;

void
_Err(UsdValidationErrorVector *errors, const TfToken &name,
     const UsdPrim &prim, const std::string &msg,
     UsdValidationErrorType severity = UsdValidationErrorType::Error)
{
    errors->emplace_back(name, severity, _PrimSites(prim), msg);
}

template <class T>
VtArray<T>
_ReadName(const UsdPrim &prim, const std::string &name)
{
    return _Read<T>(prim.GetAttribute(TfToken(name)));
}

bool
_IsAuthored(const UsdPrim &prim, const std::string &name)
{
    const UsdAttribute a = prim.GetAttribute(TfToken(name));
    return a && a.HasAuthoredValue();
}

bool
_HasAppliedSchema(const UsdPrim &prim, const TfToken &schemaToken)
{
    for (const TfToken &s : prim.GetAppliedSchemas()) {
        if (s == schemaToken) {
            return true;
        }
    }
    return false;
}

size_t
_CountToken(const VtArray<TfToken> &arr, const TfToken &tok)
{
    size_t c = 0;
    for (const TfToken &t : arr) {
        if (t == tok) {
            ++c;
        }
    }
    return c;
}

// Per-Brep prefix-offset partitions. Each vector has length numBreps+1 and
// holds cumulative counts so that the objects of Brep ii occupy the half-open
// index range [arr[ii], arr[ii+1]) in the corresponding flat array. (Objects
// related to a single Brep are stored consecutively.)
//
// Only the levels that have an explicit per-Brep *count* array are tracked
// here, so every partition below is exact for any number of Breps. There is no
// per-Brep count for edges or vertices, so those cannot be partitioned
// reliably from the flat data (a reference-derived partition mis-attributes
// non-contiguous or orphaned entities); index-range checks on edges/vertices
// therefore validate against global bounds, and containment uses the union of
// all brep:extent boxes.
struct _BrepOffsets {
    size_t numBreps = 0;
    std::vector<size_t> region, shell, faceuse, face, loop, edgeuse, wireEdge;
    bool ok = false;
};

_BrepOffsets
_ComputeOffsets(const UsdSolidBrepArray &brep)
{
    _BrepOffsets o;
    const VtArray<unsigned int> regionCount
        = _Read<unsigned int>(brep.GetBrepRegionCountAttr());
    const VtArray<unsigned int> regionShellCount
        = _Read<unsigned int>(brep.GetRegionShellCountAttr());
    const VtArray<unsigned int> shellFaceuseCount
        = _Read<unsigned int>(brep.GetShellFaceuseCountAttr());
    const VtArray<unsigned int> shellWireEdgeCount
        = _Read<unsigned int>(brep.GetShellWireEdgeCountAttr());
    const VtArray<unsigned int> faceLoopCount
        = _Read<unsigned int>(brep.GetFaceLoopCountAttr());
    const VtArray<unsigned int> loopEdgeuseCount
        = _Read<unsigned int>(brep.GetLoopEdgeuseCountAttr());

    const size_t n = regionCount.size();
    o.numBreps = n;
    if (n == 0) {
        return o;
    }

    const auto sumRange
        = [](const VtArray<unsigned int> &a, size_t lo, size_t hi) {
              size_t s = 0;
              for (size_t i = lo; i < hi && i < a.size(); ++i) {
                  s += a[i];
              }
              return s;
          };

    o.region.assign(n + 1, 0);
    for (size_t b = 0; b < n; ++b) {
        o.region[b + 1] = o.region[b] + regionCount[b];
    }
    o.shell.assign(n + 1, 0);
    for (size_t b = 0; b < n; ++b) {
        o.shell[b + 1]
            = o.shell[b] + sumRange(regionShellCount, o.region[b], o.region[b + 1]);
    }
    o.faceuse.assign(n + 1, 0);
    o.wireEdge.assign(n + 1, 0);
    for (size_t b = 0; b < n; ++b) {
        o.faceuse[b + 1]
            = o.faceuse[b] + sumRange(shellFaceuseCount, o.shell[b], o.shell[b + 1]);
        o.wireEdge[b + 1]
            = o.wireEdge[b]
            + sumRange(shellWireEdgeCount, o.shell[b], o.shell[b + 1]);
    }
    o.face.assign(n + 1, 0);
    for (size_t b = 0; b < n; ++b) {
        o.face[b + 1] = o.face[b] + (o.faceuse[b + 1] - o.faceuse[b]) / 2;
    }
    o.loop.assign(n + 1, 0);
    for (size_t b = 0; b < n; ++b) {
        o.loop[b + 1]
            = o.loop[b] + sumRange(faceLoopCount, o.face[b], o.face[b + 1]);
    }
    o.edgeuse.assign(n + 1, 0);
    for (size_t b = 0; b < n; ++b) {
        o.edgeuse[b + 1]
            = o.edgeuse[b] + sumRange(loopEdgeuseCount, o.loop[b], o.loop[b + 1]);
    }
    o.ok = true;
    return o;
}

// --- NURBS stratum helpers (shared by surface / edge3d / curveUv) --------- //

void
_CheckNurbOrderPositive(const UsdPrim &prim, const char *ba, const char *label,
                        const VtArray<unsigned int> &order,
                        const VtArray<unsigned int> &vtxCount,
                        bool allowZeroSentinel,
                        UsdValidationErrorVector *errors)
{
    for (size_t i = 0; i < order.size(); ++i) {
        const unsigned int vc = i < vtxCount.size() ? vtxCount[i] : 0u;
        if (allowZeroSentinel && order[i] == 0u && vc == 0u) {
            continue;
        }
        if (order[i] == 0u) {
            _Err(errors, UsdSolidValidationErrorNameTokens->nurbNonPositiveOrder,
                 prim,
                 TfStringPrintf("[%s] BrepArray <%s>: %s order[%zu] = 0 must be "
                                "positive.",
                                ba, prim.GetPath().GetText(), label, i));
        }
    }
}

void
_CheckNurbOrderLEVtx(const UsdPrim &prim, const char *ba, const char *label,
                     const VtArray<unsigned int> &order,
                     const VtArray<unsigned int> &vtxCount,
                     bool allowZeroSentinel, UsdValidationErrorVector *errors)
{
    (void)allowZeroSentinel; // sentinel subsumed by the order==0 skip below
    const size_t m = std::min(order.size(), vtxCount.size());
    for (size_t i = 0; i < m; ++i) {
        // order == 0 cannot violate order > vertexCount; it (incl. the all-zero
        // curveUv sentinel) is handled by the order-positivity check.
        if (order[i] == 0u) {
            continue;
        }
        if (order[i] > vtxCount[i]) {
            _Err(errors,
                 UsdSolidValidationErrorNameTokens->nurbOrderExceedsVertexCount,
                 prim,
                 TfStringPrintf("[%s] BrepArray <%s>: %s order[%zu] = %u exceeds "
                                "vertexCount %u.",
                                ba, prim.GetPath().GetText(), label, i, order[i],
                                vtxCount[i]));
        }
    }
}

void
_CheckNurbOrderMin2(const UsdPrim &prim, const char *label,
                    const VtArray<unsigned int> &order,
                    const VtArray<unsigned int> &vtxCount,
                    bool allowZeroSentinel, UsdValidationErrorVector *errors)
{
    for (size_t i = 0; i < order.size(); ++i) {
        const unsigned int vc = i < vtxCount.size() ? vtxCount[i] : 0u;
        if (allowZeroSentinel && order[i] == 0u && vc == 0u) {
            continue;
        }
        if (order[i] < 2u) {
            _Err(errors, UsdSolidValidationErrorNameTokens->nurbOrderBelowMinimum,
                 prim,
                 TfStringPrintf("[BA.590] BrepArray <%s>: %s order[%zu] = %u must "
                                "be >= 2 (degree >= 1).",
                                prim.GetPath().GetText(), label, i, order[i]));
            break;
        }
    }
}

void
_CheckNurbVtxGEOrder(const UsdPrim &prim, const char *label,
                     const VtArray<unsigned int> &order,
                     const VtArray<unsigned int> &vtxCount,
                     bool allowZeroSentinel, UsdValidationErrorVector *errors)
{
    if (order.size() != vtxCount.size()) {
        return;
    }
    for (size_t i = 0; i < order.size(); ++i) {
        if (allowZeroSentinel && order[i] == 0u && vtxCount[i] == 0u) {
            continue;
        }
        if (vtxCount[i] < order[i]) {
            _Err(errors,
                 UsdSolidValidationErrorNameTokens->nurbVertexCountBelowOrder,
                 prim,
                 TfStringPrintf("[BA.591] BrepArray <%s>: %s vertexCount[%zu] = "
                                "%u is less than order %u.",
                                prim.GetPath().GetText(), label, i, vtxCount[i],
                                order[i]));
            break;
        }
    }
}

void
_CheckNurbWeights(const UsdPrim &prim, const char *ba, const char *label,
                  const VtArray<double> &weights,
                  UsdValidationErrorVector *errors)
{
    for (size_t i = 0; i < weights.size(); ++i) {
        if (weights[i] < _NurbsTol) {
            _Err(errors, UsdSolidValidationErrorNameTokens->nurbNonPositiveWeight,
                 prim,
                 TfStringPrintf("[%s] BrepArray <%s>: %s weight[%zu] = %g must be "
                                "positive.",
                                ba, prim.GetPath().GetText(), label, i,
                                weights[i]));
        }
    }
}

// Knot vector size + monotonicity for a single (order, vertexCount) direction.
// Only validated when the knot array is authored (non-empty), matching the
// reference which gates these checks on present knot data.
void
_CheckNurbKnots1D(const UsdPrim &prim, const char *baSize, const char *baMono,
                  const char *label, const VtArray<unsigned int> &order,
                  const VtArray<unsigned int> &vtxCount,
                  const VtArray<double> &knots, UsdValidationErrorVector *errors)
{
    if (knots.empty()) {
        return;
    }
    const size_t m = std::min(order.size(), vtxCount.size());
    size_t expected = 0;
    for (size_t i = 0; i < m; ++i) {
        expected += static_cast<size_t>(order[i]) + vtxCount[i];
    }
    if (knots.size() != expected) {
        _Err(errors, UsdSolidValidationErrorNameTokens->nurbKnotCountMismatch,
             prim,
             TfStringPrintf("[%s] BrepArray <%s>: %s knot vector size %zu but "
                            "expected %zu (sum of order + vertexCount).",
                            baSize, prim.GetPath().GetText(), label,
                            knots.size(), expected));
    }
    size_t off = 0;
    for (size_t i = 0; i < m; ++i) {
        const size_t cnt = static_cast<size_t>(order[i]) + vtxCount[i];
        if (off + cnt > knots.size()) {
            break;
        }
        for (size_t k = 1; k < cnt; ++k) {
            if (knots[off + k] < knots[off + k - 1] - _NurbsTol) {
                _Err(errors,
                     UsdSolidValidationErrorNameTokens->nurbKnotNotMonotonic,
                     prim,
                     TfStringPrintf("[%s] BrepArray <%s>: %s knot vector %zu is "
                                    "not non-decreasing.",
                                    baMono, prim.GetPath().GetText(), label, i));
                break;
            }
        }
        off += cnt;
    }
}

// An authored attribute's *defined* type (UsdAttribute::GetTypeName) is the
// schema type for builtin attributes, so it cannot reveal a wrong authored
// type; and the held C++ value type cannot distinguish role-aliased types such
// as point3d[] vs vector3d[] (both VtArray<GfVec3d>). Inspect the strongest
// authored attribute spec's typeName, which preserves the authored role.
bool
_AuthoredTypeIn(const UsdPrim &prim, const char *attr,
                const std::vector<SdfValueTypeName> &acceptable,
                std::string *got)
{
    const UsdAttribute a = prim.GetAttribute(TfToken(attr));
    if (!a || !a.HasAuthoredValue()) {
        return false;
    }
    for (const SdfPropertySpecHandle &spec :
         a.GetPropertyStack(UsdTimeCode::Default())) {
        const SdfAttributeSpecHandle attrSpec
            = TfDynamic_cast<SdfAttributeSpecHandle>(spec);
        if (!attrSpec || !attrSpec->GetTypeName()) {
            continue;
        }
        // Strongest authored typeName wins.
        const SdfValueTypeName authored = attrSpec->GetTypeName();
        for (const SdfValueTypeName &ok : acceptable) {
            if (authored == ok) {
                return false;
            }
        }
        if (got) {
            *got = authored.GetAsToken().GetString();
        }
        return true;
    }
    return false;
}

bool
_AuthoredTypeMismatch(const UsdPrim &prim, const char *attr,
                      const SdfValueTypeName &expected, std::string *got)
{
    return _AuthoredTypeIn(prim, attr, { expected }, got);
}

void
_CheckNurbType(const UsdPrim &prim, const char *ba, const char *attr,
               const SdfValueTypeName &expected,
               UsdValidationErrorVector *errors)
{
    std::string got;
    if (_AuthoredTypeMismatch(prim, attr, expected, &got)) {
        _Err(errors, UsdSolidValidationErrorNameTokens->nurbInvalidDataType,
             prim,
             TfStringPrintf("[%s] BrepArray <%s>: attribute %s has type '%s' but "
                            "expected '%s'.",
                            ba, prim.GetPath().GetText(), attr, got.c_str(),
                            expected.GetAsToken().GetText()));
    }
}

// --- Analytic curve helpers (BA.53x/54x/55x) ------------------------------ //

void
_CheckCurveArraySize(const UsdPrim &prim, const char *ba, const char *shape,
                     const char *inst, const char *attr, size_t actual,
                     size_t expected, UsdValidationErrorVector *errors)
{
    if (actual != expected) {
        _Err(errors,
             UsdSolidValidationErrorNameTokens->analyticCurveArraySizeMismatch,
             prim,
             TfStringPrintf("[%s] BrepArray <%s>: %s %s %s size %zu but expected "
                            "%zu.",
                            ba, prim.GetPath().GetText(), shape, inst, attr,
                            actual, expected));
    }
}

void
_CheckCurveUnit(const UsdPrim &prim, const char *ba, const char *shape,
                const char *inst, const char *attr, const TfToken &errTok,
                const VtArray<GfVec3d> &vecs, UsdValidationErrorVector *errors)
{
    for (size_t i = 0; i < vecs.size(); ++i) {
        if (std::abs(vecs[i].GetLength() - 1.0) > _CurveEps) {
            _Err(errors, errTok, prim,
                 TfStringPrintf("[%s] BrepArray <%s>: %s %s %s[%zu] is not unit "
                                "length (length %g).",
                                ba, prim.GetPath().GetText(), shape, inst, attr,
                                i, vecs[i].GetLength()));
            break;
        }
    }
}

void
_CheckCurveOrtho(const UsdPrim &prim, const char *ba, const char *shape,
                 const char *inst, const VtArray<GfVec3d> &axis,
                 const VtArray<GfVec3d> &ref, UsdValidationErrorVector *errors)
{
    const size_t m = std::min(axis.size(), ref.size());
    for (size_t i = 0; i < m; ++i) {
        if (std::abs(GfDot(axis[i], ref[i])) > _CurveEps) {
            _Err(errors,
                 UsdSolidValidationErrorNameTokens
                     ->analyticCurveAxisRefDirectionNotOrthogonal,
                 prim,
                 TfStringPrintf("[%s] BrepArray <%s>: %s %s axis and "
                                "refDirection at index %zu are not orthogonal "
                                "(dot %g).",
                                ba, prim.GetPath().GetText(), shape, inst, i,
                                GfDot(axis[i], ref[i])));
            break;
        }
    }
}

void
_CheckCurveRadii(const UsdPrim &prim, const char *ba, const char *shape,
                 const char *inst, const char *attr, const VtArray<double> &r,
                 UsdValidationErrorVector *errors)
{
    for (size_t i = 0; i < r.size(); ++i) {
        if (r[i] <= 0.0) {
            _Err(errors,
                 UsdSolidValidationErrorNameTokens->analyticCurveNonPositiveRadius,
                 prim,
                 TfStringPrintf("[%s] BrepArray <%s>: %s %s %s[%zu] = %g must be "
                                "positive.",
                                ba, prim.GetPath().GetText(), shape, inst, attr,
                                i, r[i]));
            break;
        }
    }
}

void
_CheckCircleInstance(const UsdPrim &prim, const char *inst, size_t count,
                     UsdValidationErrorVector *errors)
{
    if (count == 0) {
        return;
    }
    const std::string b = std::string("brep:") + inst + ":curve3d:circle:";
    const VtArray<GfVec3d> center = _ReadName<GfVec3d>(prim, b + "center");
    const VtArray<GfVec3d> axis = _ReadName<GfVec3d>(prim, b + "axis");
    const VtArray<GfVec3d> ref = _ReadName<GfVec3d>(prim, b + "refDirection");
    const VtArray<double> radius = _ReadName<double>(prim, b + "radius");
    _CheckCurveArraySize(prim, "BA.530", "circle", inst, "center",
                         center.size(), count, errors);
    _CheckCurveArraySize(prim, "BA.530", "circle", inst, "axis", axis.size(),
                         count, errors);
    _CheckCurveArraySize(prim, "BA.530", "circle", inst, "refDirection",
                         ref.size(), count, errors);
    _CheckCurveArraySize(prim, "BA.530", "circle", inst, "radius", radius.size(),
                         count, errors);
    _CheckCurveRadii(prim, "BA.531", "circle", inst, "radius", radius, errors);
    _CheckCurveUnit(prim, "BA.532", "circle", inst, "axis",
                    UsdSolidValidationErrorNameTokens->analyticCurveAxisNotUnitLength,
                    axis, errors);
    _CheckCurveUnit(
        prim, "BA.533", "circle", inst, "refDirection",
        UsdSolidValidationErrorNameTokens->analyticCurveRefDirectionNotUnitLength,
        ref, errors);
    _CheckCurveOrtho(prim, "BA.534", "circle", inst, axis, ref, errors);
}

void
_CheckLineInstance(const UsdPrim &prim, const char *inst, size_t count,
                   UsdValidationErrorVector *errors)
{
    if (count == 0) {
        return;
    }
    const std::string b = std::string("brep:") + inst + ":curve3d:line:";
    const VtArray<GfVec3d> origin = _ReadName<GfVec3d>(prim, b + "origin");
    const VtArray<GfVec3d> direction = _ReadName<GfVec3d>(prim, b + "direction");
    _CheckCurveArraySize(prim, "BA.540", "line", inst, "origin", origin.size(),
                         count, errors);
    _CheckCurveArraySize(prim, "BA.540", "line", inst, "direction",
                         direction.size(), count, errors);
    _CheckCurveUnit(prim, "BA.541", "line", inst, "direction",
                    UsdSolidValidationErrorNameTokens->lineDirectionNotUnitLength,
                    direction, errors);
}

void
_CheckEllipseInstance(const UsdPrim &prim, const char *inst, size_t count,
                      UsdValidationErrorVector *errors)
{
    if (count == 0) {
        return;
    }
    const std::string b = std::string("brep:") + inst + ":curve3d:ellipse:";
    const VtArray<GfVec3d> center = _ReadName<GfVec3d>(prim, b + "center");
    const VtArray<GfVec3d> axis = _ReadName<GfVec3d>(prim, b + "axis");
    const VtArray<GfVec3d> ref = _ReadName<GfVec3d>(prim, b + "refDirection");
    const VtArray<double> xRadius = _ReadName<double>(prim, b + "xRadius");
    const VtArray<double> yRadius = _ReadName<double>(prim, b + "yRadius");
    _CheckCurveArraySize(prim, "BA.550", "ellipse", inst, "center",
                         center.size(), count, errors);
    _CheckCurveArraySize(prim, "BA.550", "ellipse", inst, "axis", axis.size(),
                         count, errors);
    _CheckCurveArraySize(prim, "BA.550", "ellipse", inst, "refDirection",
                         ref.size(), count, errors);
    _CheckCurveArraySize(prim, "BA.550", "ellipse", inst, "xRadius",
                         xRadius.size(), count, errors);
    _CheckCurveArraySize(prim, "BA.550", "ellipse", inst, "yRadius",
                         yRadius.size(), count, errors);
    _CheckCurveRadii(prim, "BA.551", "ellipse", inst, "xRadius", xRadius, errors);
    _CheckCurveRadii(prim, "BA.552", "ellipse", inst, "yRadius", yRadius, errors);
    _CheckCurveUnit(prim, "BA.553", "ellipse", inst, "axis",
                    UsdSolidValidationErrorNameTokens->analyticCurveAxisNotUnitLength,
                    axis, errors);
    _CheckCurveUnit(
        prim, "BA.554", "ellipse", inst, "refDirection",
        UsdSolidValidationErrorNameTokens->analyticCurveRefDirectionNotUnitLength,
        ref, errors);
    _CheckCurveOrtho(prim, "BA.555", "ellipse", inst, axis, ref, errors);
}

// -------------------------------------------------------------------------- //
// BrepArrayAuthorship                                                        //
// -------------------------------------------------------------------------- //
UsdValidationErrorVector
_BrepArrayAuthorship(const UsdPrim &usdPrim,
                     const UsdValidationTimeRange & /*timeRange*/)
{
    if (!(usdPrim && usdPrim.IsA<UsdSolidBrepArray>())) {
        return {};
    }
    struct Item {
        const char *attr;
        const char *family;
        const char *ba;
    };
    static const std::vector<Item> items = {
        { "region:shellCount", "region", "BA.070" },
        { "region:type", "region", "BA.070" },
        { "shell:faceuseCount", "shell", "BA.085" },
        { "shell:wireEdgeCount", "shell", "BA.085" },
        { "shell:pointType", "shell", "BA.085" },
        { "faceuse:faceIndex", "faceuse", "BA.105" },
        { "faceuse:orientationType", "faceuse", "BA.105" },
        { "face:loopCount", "face", "BA.125" },
        { "face:surfaceType", "face", "BA.125" },
        { "face:trimType", "face", "BA.125" },
        { "face:range", "face", "BA.125" },
        { "loop:edgeuseCount", "loop", "BA.170" },
        { "loop:vertexIndex", "loop", "BA.170" },
        { "edgeuse:edgeIndex", "edgeuse", "BA.185" },
        { "edgeuse:orientationType", "edgeuse", "BA.185" },
        { "edgeuse:nextRadialEUIndex", "edgeuse", "BA.185" },
        { "edgeuse:thisRadialEntryType", "edgeuse", "BA.185" },
        { "edge:curveType", "edge", "BA.215" },
        { "edge:vertexIndices", "edge", "BA.215" },
        { "edge:range", "edge", "BA.215" },
        { "wireEdge:curveType", "wireEdge", "BA.255" },
        { "wireEdge:vertexIndices", "wireEdge", "BA.255" },
        { "wireEdge:range", "wireEdge", "BA.255" },
        { "vertex:pointType", "vertex", "BA.300" },
    };
    UsdValidationErrorVector errors;
    for (const Item &it : items) {
        if (!_IsAuthored(usdPrim, it.attr)) {
            _Err(&errors, UsdSolidValidationErrorNameTokens->attributeNotAuthored,
                 usdPrim,
                 TfStringPrintf("[%s] BrepArray <%s>: %s attribute %s is not "
                                "authored.",
                                it.ba, usdPrim.GetPath().GetText(), it.family,
                                it.attr));
        }
    }
    return errors;
}

// -------------------------------------------------------------------------- //
// BrepArrayDataTypes                                                         //
// -------------------------------------------------------------------------- //
UsdValidationErrorVector
_BrepArrayDataTypes(const UsdPrim &usdPrim,
                    const UsdValidationTimeRange & /*timeRange*/)
{
    if (!(usdPrim && usdPrim.IsA<UsdSolidBrepArray>())) {
        return {};
    }
    struct Item {
        const char *attr;
        SdfValueTypeName type;
        const char *ba;
        bool vecRole = false; // accept any double-precision 3-vector role
    };
    const std::vector<Item> items = {
        { "brep:intersectTol3d", SdfValueTypeNames->DoubleArray, "BA.061" },
        { "brep:extent", SdfValueTypeNames->Double3Array, "BA.061" },
        { "brep:regionCount", SdfValueTypeNames->UIntArray, "BA.061" },
        { "region:shellCount", SdfValueTypeNames->UIntArray, "BA.076" },
        { "region:type", SdfValueTypeNames->TokenArray, "BA.076" },
        { "shell:faceuseCount", SdfValueTypeNames->UIntArray, "BA.091" },
        { "shell:wireEdgeCount", SdfValueTypeNames->UIntArray, "BA.091" },
        { "shell:pointType", SdfValueTypeNames->TokenArray, "BA.091" },
        { "faceuse:faceIndex", SdfValueTypeNames->UIntArray, "BA.116" },
        { "faceuse:orientationType", SdfValueTypeNames->TokenArray, "BA.116" },
        { "face:loopCount", SdfValueTypeNames->UIntArray, "BA.161" },
        { "face:surfaceType", SdfValueTypeNames->TokenArray, "BA.161" },
        { "face:trimType", SdfValueTypeNames->TokenArray, "BA.161" },
        { "face:range", SdfValueTypeNames->Double2Array, "BA.161" },
        { "loop:edgeuseCount", SdfValueTypeNames->UIntArray, "BA.176" },
        { "loop:vertexIndex", SdfValueTypeNames->UIntArray, "BA.176" },
        { "edgeuse:edgeIndex", SdfValueTypeNames->UIntArray, "BA.196" },
        { "edgeuse:orientationType", SdfValueTypeNames->TokenArray, "BA.196" },
        { "edgeuse:nextRadialEUIndex", SdfValueTypeNames->UIntArray, "BA.196" },
        { "edgeuse:thisRadialEntryType", SdfValueTypeNames->TokenArray,
          "BA.196" },
        { "edge:curveType", SdfValueTypeNames->TokenArray, "BA.237" },
        { "edge:vertexIndices", SdfValueTypeNames->Int2Array, "BA.237" },
        { "edge:range", SdfValueTypeNames->DoubleArray, "BA.237" },
        { "wireEdge:curveType", SdfValueTypeNames->TokenArray, "BA.291" },
        { "wireEdge:vertexIndices", SdfValueTypeNames->Int2Array, "BA.291" },
        { "wireEdge:range", SdfValueTypeNames->DoubleArray, "BA.291" },
        { "vertex:pointType", SdfValueTypeNames->TokenArray, "BA.316" },
        { "brep:vertexPoint:point:position", SdfValueTypeNames->Point3dArray,
          "BA.326", true },
        { "brep:shellPoint:point:position", SdfValueTypeNames->Point3dArray,
          "BA.327", true },
    };
    // point3d / vector3d / double3 all carry GfVec3d; the SimReady producer
    // authors positions as vector3d[] while the schema declares point3d[], so
    // accept any of these role-aliases rather than rejecting valid output.
    const std::vector<SdfValueTypeName> vec3Roles
        = { SdfValueTypeNames->Point3dArray, SdfValueTypeNames->Vector3dArray,
            SdfValueTypeNames->Double3Array };
    UsdValidationErrorVector errors;
    for (const Item &it : items) {
        std::string got;
        const bool mismatch
            = it.vecRole ? _AuthoredTypeIn(usdPrim, it.attr, vec3Roles, &got)
                         : _AuthoredTypeMismatch(usdPrim, it.attr, it.type, &got);
        if (mismatch) {
            _Err(&errors,
                 UsdSolidValidationErrorNameTokens->invalidAttributeDataType,
                 usdPrim,
                 TfStringPrintf("[%s] BrepArray <%s>: attribute %s has type '%s' "
                                "but expected '%s'.",
                                it.ba, usdPrim.GetPath().GetText(), it.attr,
                                got.c_str(), it.type.GetAsToken().GetText()));
        }
    }
    return errors;
}

// -------------------------------------------------------------------------- //
// BrepArraySchemaUsage                                                       //
// -------------------------------------------------------------------------- //
UsdValidationErrorVector
_BrepArraySchemaUsage(const UsdPrim &usdPrim,
                      const UsdValidationTimeRange & /*timeRange*/)
{
    if (!(usdPrim && usdPrim.IsA<UsdSolidBrepArray>())) {
        return {};
    }
    const UsdSolidBrepArray brep(usdPrim);
    const VtArray<TfToken> faceSurfaceType
        = _Read<TfToken>(brep.GetFaceSurfaceTypeAttr());
    const VtArray<TfToken> vertexPointType
        = _Read<TfToken>(brep.GetVertexPointTypeAttr());

    struct Item {
        const char *schemaToken;  // GetAppliedSchemas() membership token
        const char *driverValue;  // value to count in the driver token array
        const char *presenceAttr; // attribute whose authorship proves data
        const char *ba;
        const char *label;
        bool isVertexDriver;      // drive off vertex:pointType, else face:surfaceType
    };
    static const std::vector<Item> items = {
        { "BrepPointAPI:vertexPoint", "BrepPointAPI",
          "brep:vertexPoint:point:position", "BA.305", "vertexPoint", true },
        { "BrepSurfaceSphereAPI", "BrepSurfaceSphereAPI",
          "brep:surface:sphere:center", "BA.485", "sphere", false },
        { "BrepSurfacePlaneAPI", "BrepSurfacePlaneAPI",
          "brep:surface:plane:origin", "BA.495", "plane", false },
        { "BrepSurfaceCylinderAPI", "BrepSurfaceCylinderAPI",
          "brep:surface:cylinder:origin", "BA.505", "cylinder", false },
        { "BrepSurfaceConeAPI", "BrepSurfaceConeAPI", "brep:surface:cone:origin",
          "BA.516", "cone", false },
        { "BrepSurfaceTorusAPI", "BrepSurfaceTorusAPI",
          "brep:surface:torus:origin", "BA.526", "torus", false },
    };
    UsdValidationErrorVector errors;
    for (const Item &it : items) {
        const VtArray<TfToken> &driver
            = it.isVertexDriver ? vertexPointType : faceSurfaceType;
        const size_t count = _CountToken(driver, TfToken(it.driverValue));
        const bool hasData = _IsAuthored(usdPrim, it.presenceAttr);
        const bool applied
            = _HasAppliedSchema(usdPrim, TfToken(it.schemaToken));
        if (count > 0 && !hasData) {
            _Err(&errors,
                 UsdSolidValidationErrorNameTokens->schemaUsageInconsistent,
                 usdPrim,
                 TfStringPrintf("[%s] BrepArray <%s>: %s usage is declared but "
                                "no %s data is authored.",
                                it.ba, usdPrim.GetPath().GetText(), it.label,
                                it.presenceAttr));
        }
        if (applied && count == 0) {
            _Err(&errors,
                 UsdSolidValidationErrorNameTokens->schemaUsageInconsistent,
                 usdPrim,
                 TfStringPrintf("[%s] BrepArray <%s>: %s is in apiSchemas but no "
                                "%s usage is declared.",
                                it.ba, usdPrim.GetPath().GetText(),
                                it.schemaToken, it.label));
        }
    }
    return errors;
}

// -------------------------------------------------------------------------- //
// BrepArrayReferences                                                        //
// -------------------------------------------------------------------------- //
UsdValidationErrorVector
_BrepArrayReferences(const UsdPrim &usdPrim,
                     const UsdValidationTimeRange & /*timeRange*/)
{
    if (!(usdPrim && usdPrim.IsA<UsdSolidBrepArray>())) {
        return {};
    }
    const UsdSolidBrepArray brep(usdPrim);
    const _BrepOffsets off = _ComputeOffsets(brep);
    if (!off.ok) {
        return {};
    }
    const size_t n = off.numBreps;
    UsdValidationErrorVector errors;

    // BA.115: faceuse:faceIndex within owning Brep's face partition.
    const VtArray<unsigned int> faceIndex
        = _Read<unsigned int>(brep.GetFaceuseFaceIndexAttr());
    for (size_t b = 0; b < n; ++b) {
        for (size_t fu = off.faceuse[b];
             fu < off.faceuse[b + 1] && fu < faceIndex.size(); ++fu) {
            if (faceIndex[fu] < off.face[b] || faceIndex[fu] >= off.face[b + 1]) {
                _Err(&errors,
                     UsdSolidValidationErrorNameTokens->faceuseFaceIndexOutOfRange,
                     usdPrim,
                     TfStringPrintf("[BA.115] BrepArray <%s>: faceuse:faceIndex"
                                    "[%zu] = %u is outside Brep %zu's face range "
                                    "[%zu, %zu).",
                                    usdPrim.GetPath().GetText(), fu,
                                    faceIndex[fu], b, off.face[b],
                                    off.face[b + 1]));
            }
        }
    }

    // BA.205: edgeuse:edgeIndex must reference a valid edge. Edges have no
    // per-Brep count array, so validate against the global edge range; for a
    // single Brep this is exactly that Brep's range.
    const VtArray<unsigned int> edgeIndex
        = _Read<unsigned int>(brep.GetEdgeuseEdgeIndexAttr());
    const size_t numEdges = _Read<TfToken>(brep.GetEdgeCurveTypeAttr()).size();
    for (size_t eu = 0; eu < edgeIndex.size(); ++eu) {
        if (edgeIndex[eu] >= numEdges) {
            _Err(&errors,
                 UsdSolidValidationErrorNameTokens->edgeuseEdgeIndexOutOfRange,
                 usdPrim,
                 TfStringPrintf("[BA.205] BrepArray <%s>: edgeuse:edgeIndex[%zu] "
                                "= %u is out of range [0, %zu).",
                                usdPrim.GetPath().GetText(), eu, edgeIndex[eu],
                                numEdges));
        }
    }

    // BA.200: edgeuse:nextRadialEUIndex within owning Brep's edgeuse partition.
    const VtArray<unsigned int> nextRadial
        = _Read<unsigned int>(brep.GetEdgeuseNextRadialEUIndexAttr());
    const size_t totalEu = nextRadial.size();
    for (size_t b = 0; b < n; ++b) {
        for (size_t eu = off.edgeuse[b];
             eu < off.edgeuse[b + 1] && eu < nextRadial.size(); ++eu) {
            if (nextRadial[eu] >= totalEu || nextRadial[eu] < off.edgeuse[b]
                || nextRadial[eu] >= off.edgeuse[b + 1]) {
                _Err(&errors,
                     UsdSolidValidationErrorNameTokens
                         ->edgeuseNextRadialIndexOutOfRange,
                     usdPrim,
                     TfStringPrintf("[BA.200] BrepArray <%s>: "
                                    "edgeuse:nextRadialEUIndex[%zu] = %u is "
                                    "outside Brep %zu's edgeuse range [%zu, %zu).",
                                    usdPrim.GetPath().GetText(), eu,
                                    nextRadial[eu], b, off.edgeuse[b],
                                    off.edgeuse[b + 1]));
            }
        }
    }

    // Vertices also have no per-Brep count array; validate vertex-index
    // references against the global vertex range (exact for a single Brep).
    const size_t vSize
        = _Read<TfToken>(brep.GetVertexPointTypeAttr()).size();

    // BA.175: a loop with no edgeuses must reference a valid vertex.
    const VtArray<unsigned int> loopEdgeuseCount
        = _Read<unsigned int>(brep.GetLoopEdgeuseCountAttr());
    const VtArray<unsigned int> loopVertexIndex
        = _Read<unsigned int>(brep.GetLoopVertexIndexAttr());
    for (size_t lp = 0;
         lp < loopEdgeuseCount.size() && lp < loopVertexIndex.size(); ++lp) {
        if (loopEdgeuseCount[lp] == 0u && loopVertexIndex[lp] >= vSize) {
            _Err(&errors,
                 UsdSolidValidationErrorNameTokens->loopVertexIndexOutOfRange,
                 usdPrim,
                 TfStringPrintf("[BA.175] BrepArray <%s>: loop:vertexIndex[%zu] = "
                                "%u (loop with no edgeuses) is out of range "
                                "[0, %zu).",
                                usdPrim.GetPath().GetText(), lp,
                                loopVertexIndex[lp], vSize));
        }
    }

    // BA.225: both components of each edge:vertexIndices pair must be valid.
    const VtArray<GfVec2i> edgeVtx
        = _Read<GfVec2i>(brep.GetEdgeVertexIndicesAttr());
    for (size_t e = 0; e < edgeVtx.size(); ++e) {
        for (int k = 0; k < 2; ++k) {
            const long long c = edgeVtx[e][k];
            if (c < 0 || c >= static_cast<long long>(vSize)) {
                _Err(&errors,
                     UsdSolidValidationErrorNameTokens->edgeVertexIndexOutOfRange,
                     usdPrim,
                     TfStringPrintf("[BA.225] BrepArray <%s>: edge:vertexIndices"
                                    "[%zu][%d] = %lld is out of range [0, %zu).",
                                    usdPrim.GetPath().GetText(), e, k, c, vSize));
            }
        }
    }

    // BA.265: both components of each wireEdge:vertexIndices pair must be valid.
    const VtArray<GfVec2i> wireVtx
        = _Read<GfVec2i>(brep.GetWireEdgeVertexIndicesAttr());
    for (size_t w = 0; w < wireVtx.size(); ++w) {
        for (int k = 0; k < 2; ++k) {
            const long long c = wireVtx[w][k];
            if (c < 0 || c >= static_cast<long long>(vSize)) {
                _Err(&errors,
                     UsdSolidValidationErrorNameTokens
                         ->wireEdgeVertexIndexOutOfRange,
                     usdPrim,
                     TfStringPrintf("[BA.265] BrepArray <%s>: wireEdge:"
                                    "vertexIndices[%zu][%d] = %lld is out of "
                                    "range [0, %zu).",
                                    usdPrim.GetPath().GetText(), w, k, c, vSize));
            }
        }
    }

    return errors;
}

// -------------------------------------------------------------------------- //
// BrepArrayCompleteness                                                      //
// -------------------------------------------------------------------------- //
UsdValidationErrorVector
_BrepArrayCompleteness(const UsdPrim &usdPrim,
                       const UsdValidationTimeRange & /*timeRange*/)
{
    if (!(usdPrim && usdPrim.IsA<UsdSolidBrepArray>())) {
        return {};
    }
    const UsdSolidBrepArray brep(usdPrim);
    const _BrepOffsets off = _ComputeOffsets(brep);
    if (!off.ok) {
        return {};
    }
    const size_t n = off.numBreps;
    UsdValidationErrorVector errors;

    // BA.580: each face referenced by exactly two faceuses within its Brep.
    const VtArray<unsigned int> faceIndex
        = _Read<unsigned int>(brep.GetFaceuseFaceIndexAttr());
    if (!faceIndex.empty()) {
        for (size_t b = 0; b < n; ++b) {
            std::unordered_map<unsigned int, int> refCount;
            for (size_t fu = off.faceuse[b];
                 fu < off.faceuse[b + 1] && fu < faceIndex.size(); ++fu) {
                ++refCount[faceIndex[fu]];
            }
            for (size_t f = off.face[b]; f < off.face[b + 1]; ++f) {
                const auto it = refCount.find(static_cast<unsigned int>(f));
                const int c = it == refCount.end() ? 0 : it->second;
                if (c != 2) {
                    _Err(&errors,
                         UsdSolidValidationErrorNameTokens
                             ->faceusePairingViolation,
                         usdPrim,
                         TfStringPrintf("[BA.580] BrepArray <%s>: face %zu in "
                                        "Brep %zu is referenced by %d faceuses "
                                        "(expected exactly 2).",
                                        usdPrim.GetPath().GetText(), f, b, c));
                }
            }
        }
    }

    // BA.581: radial edgeuse chains must close into cycles.
    const VtArray<unsigned int> nextRadial
        = _Read<unsigned int>(brep.GetEdgeuseNextRadialEUIndexAttr());
    const size_t totalEu = nextRadial.size();
    if (totalEu > 0) {
        for (size_t b = 0; b < n; ++b) {
            const size_t euEnd = std::min(off.edgeuse[b + 1], totalEu);
            for (size_t start = std::min(off.edgeuse[b], totalEu);
                 start < euEnd; ++start) {
                const size_t maxSteps = off.edgeuse[b + 1] - off.edgeuse[b];
                size_t cur = start;
                bool closed = false;
                for (size_t step = 0; step < maxSteps; ++step) {
                    if (cur >= totalEu) {
                        break;
                    }
                    const unsigned int nxt = nextRadial[cur];
                    if (nxt >= totalEu) {
                        break;
                    }
                    if (nxt == start) {
                        closed = true;
                        break;
                    }
                    cur = nxt;
                }
                if (!closed) {
                    _Err(&errors,
                         UsdSolidValidationErrorNameTokens
                             ->radialEdgeuseChainNotClosed,
                         usdPrim,
                         TfStringPrintf("[BA.581] BrepArray <%s>: radial edgeuse "
                                        "chain starting at edgeuse %zu does not "
                                        "close.",
                                        usdPrim.GetPath().GetText(), start));
                }
            }
        }
    }

    // BA.582: every edge must be referenced by at least one edgeuse. Iterate the
    // authored edge array directly (the per-Brep offsets undercount orphans).
    const VtArray<TfToken> edgeCurveType
        = _Read<TfToken>(brep.GetEdgeCurveTypeAttr());
    const VtArray<unsigned int> edgeIndex
        = _Read<unsigned int>(brep.GetEdgeuseEdgeIndexAttr());
    if (!edgeCurveType.empty()) {
        std::unordered_set<unsigned int> referenced(edgeIndex.begin(),
                                                    edgeIndex.end());
        for (size_t e = 0; e < edgeCurveType.size(); ++e) {
            if (referenced.find(static_cast<unsigned int>(e))
                == referenced.end()) {
                _Err(&errors, UsdSolidValidationErrorNameTokens->orphanEdge,
                     usdPrim,
                     TfStringPrintf("[BA.582] BrepArray <%s>: edge %zu is not "
                                    "referenced by any edgeuse (orphan edge).",
                                    usdPrim.GetPath().GetText(), e));
            }
        }
    }

    return errors;
}

// -------------------------------------------------------------------------- //
// BrepArrayContainment                                                       //
// -------------------------------------------------------------------------- //
bool
_FloatClose(double a, double b)
{
    return std::abs(a - b)
        <= std::max(1e-5 * std::max(std::abs(a), std::abs(b)), 1e-6);
}

UsdValidationErrorVector
_BrepArrayContainment(const UsdPrim &usdPrim,
                      const UsdValidationTimeRange & /*timeRange*/)
{
    if (!(usdPrim && usdPrim.IsA<UsdSolidBrepArray>())) {
        return {};
    }
    const UsdSolidBrepArray brep(usdPrim);
    const VtArray<GfVec3d> extent = _Read<GfVec3d>(brep.GetBrepExtentAttr());
    const size_t numBoxes = extent.size() / 2;
    UsdValidationErrorVector errors;

    // BA.040/045/050: each brep:extent box within the prim's extent.
    const VtArray<GfVec3f> primExtent = _ReadName<GfVec3f>(usdPrim, "extent");
    if (primExtent.size() >= 2) {
        const char *const axisNames[3] = { "X", "Y", "Z" };
        const char *const baCodes[3] = { "BA.040", "BA.045", "BA.050" };
        for (size_t b = 0; b < numBoxes; ++b) {
            const GfVec3d &mn = extent[2 * b];
            const GfVec3d &mx = extent[2 * b + 1];
            for (int a = 0; a < 3; ++a) {
                const double pmn = primExtent[0][a];
                const double pmx = primExtent[1][a];
                if ((mn[a] < pmn && !_FloatClose(mn[a], pmn))
                    || (mx[a] > pmx && !_FloatClose(mx[a], pmx))) {
                    _Err(&errors,
                         UsdSolidValidationErrorNameTokens
                             ->brepExtentOutsidePrimExtent,
                         usdPrim,
                         TfStringPrintf("[%s] BrepArray <%s>: brep:extent for "
                                        "Brep %zu exceeds the prim's %s extent "
                                        "[%g, %g] (box [%g, %g]).",
                                        baCodes[a], usdPrim.GetPath().GetText(),
                                        b, axisNames[a], pmn, pmx, mn[a], mx[a]));
                }
            }
        }
    }

    if (numBoxes == 0) {
        return errors;
    }

    // A point is contained if it lies within ANY brep:extent box (within
    // tolerance). Per-point Brep attribution is not derivable from the flat
    // data for vertices/control points (no per-Brep count array), so the union
    // is used; for a single Brep this is exactly that Brep's box.
    const auto insideAnyExtent = [&](const GfVec3d &p) {
        for (size_t b = 0; b < numBoxes; ++b) {
            const GfVec3d &mn = extent[2 * b];
            const GfVec3d &mx = extent[2 * b + 1];
            bool inside = true;
            for (int k = 0; k < 3; ++k) {
                if (p[k] < mn[k] - _NurbsTol || p[k] > mx[k] + _NurbsTol) {
                    inside = false;
                    break;
                }
            }
            if (inside) {
                return true;
            }
        }
        return false;
    };

    // BA.310: vertex positions lie on the solid boundary, so they must be
    // within a brep:extent box (Error; reports every offending vertex).
    const VtArray<GfVec3d> vpos
        = _ReadName<GfVec3d>(usdPrim, "brep:vertexPoint:point:position");
    for (size_t v = 0; v < vpos.size(); ++v) {
        if (!insideAnyExtent(vpos[v])) {
            _Err(&errors,
                 UsdSolidValidationErrorNameTokens
                     ->vertexPositionOutsideBrepExtent,
                 usdPrim,
                 TfStringPrintf("[BA.310] BrepArray <%s>: vertex position %zu "
                                "lies outside all brep:extent boxes.",
                                usdPrim.GetPath().GetText(), v));
        }
    }

    // BA.365 / BA.465: NURBS control hulls can legitimately extend beyond the
    // surface (and hence the extent) for rational/curved geometry, so a control
    // point outside the extent is reported as a Warning, not an Error. One
    // finding per stratum keeps the output quiet on valid curved breps.
    const VtArray<GfVec3d> edgeCv = _ReadName<GfVec3d>(
        usdPrim, "brep:edge3dNurb:curve3d:nurb:controlVertices");
    for (size_t c = 0; c < edgeCv.size(); ++c) {
        if (!insideAnyExtent(edgeCv[c])) {
            _Err(&errors,
                 UsdSolidValidationErrorNameTokens->controlPointOutsideBrepExtent,
                 usdPrim,
                 TfStringPrintf("[BA.365] BrepArray <%s>: edge3dNurb control "
                                "vertex %zu lies outside all brep:extent boxes "
                                "(NURBS control hulls may legitimately exceed the "
                                "surface bounds).",
                                usdPrim.GetPath().GetText(), c),
                 UsdValidationErrorType::Warn);
            break;
        }
    }
    const VtArray<GfVec3d> surfCv
        = _ReadName<GfVec3d>(usdPrim, "brep:surface:nurb:controlVertices");
    for (size_t c = 0; c < surfCv.size(); ++c) {
        if (!insideAnyExtent(surfCv[c])) {
            _Err(&errors,
                 UsdSolidValidationErrorNameTokens->controlPointOutsideBrepExtent,
                 usdPrim,
                 TfStringPrintf("[BA.465] BrepArray <%s>: surface NURBS control "
                                "vertex %zu lies outside all brep:extent boxes "
                                "(NURBS control hulls may legitimately exceed the "
                                "surface bounds).",
                                usdPrim.GetPath().GetText(), c),
                 UsdValidationErrorType::Warn);
            break;
        }
    }

    return errors;
}

// -------------------------------------------------------------------------- //
// BrepArraySpans                                                             //
// -------------------------------------------------------------------------- //
UsdValidationErrorVector
_BrepArraySpans(const UsdPrim &usdPrim,
                const UsdValidationTimeRange & /*timeRange*/)
{
    if (!(usdPrim && usdPrim.IsA<UsdSolidBrepArray>())) {
        return {};
    }
    const UsdSolidBrepArray brep(usdPrim);
    UsdValidationErrorVector errors;

    // BA.560-565: analytic surface face:range domain limits.
    const VtArray<TfToken> faceSurfaceType
        = _Read<TfToken>(brep.GetFaceSurfaceTypeAttr());
    const VtArray<GfVec2d> faceRange = _Read<GfVec2d>(brep.GetFaceRangeAttr());
    const size_t numFaces = faceSurfaceType.size();
    if (faceRange.size() >= 2 * numFaces) {
        const TfToken sphere("BrepSurfaceSphereAPI");
        const TfToken cylinder("BrepSurfaceCylinderAPI");
        const TfToken cone("BrepSurfaceConeAPI");
        const TfToken torus("BrepSurfaceTorusAPI");
        for (size_t fi = 0; fi < numFaces; ++fi) {
            const GfVec2d &uvMin = faceRange[2 * fi];
            const GfVec2d &uvMax = faceRange[2 * fi + 1];
            const double uSpan = uvMax[0] - uvMin[0];
            const double vSpan = uvMax[1] - uvMin[1];
            const TfToken &t = faceSurfaceType[fi];
            if (t == sphere) {
                if (uSpan > _TwoPi + _DomainTol) {
                    _Err(&errors,
                         UsdSolidValidationErrorNameTokens
                             ->surfaceDomainSpanExceeded,
                         usdPrim,
                         TfStringPrintf("[BA.560] BrepArray <%s>: sphere face %zu "
                                        "U span %g exceeds 2*pi.",
                                        usdPrim.GetPath().GetText(), fi, uSpan));
                }
                if (uvMin[1] < -_HalfPi - _DomainTol
                    || uvMax[1] > _HalfPi + _DomainTol) {
                    _Err(&errors,
                         UsdSolidValidationErrorNameTokens
                             ->sphereVDomainOutOfBounds,
                         usdPrim,
                         TfStringPrintf("[BA.561] BrepArray <%s>: sphere face %zu "
                                        "V range [%g, %g] is outside "
                                        "[-pi/2, pi/2].",
                                        usdPrim.GetPath().GetText(), fi, uvMin[1],
                                        uvMax[1]));
                }
            } else if (t == cylinder && uSpan > _TwoPi + _DomainTol) {
                _Err(&errors,
                     UsdSolidValidationErrorNameTokens->surfaceDomainSpanExceeded,
                     usdPrim,
                     TfStringPrintf("[BA.562] BrepArray <%s>: cylinder face %zu U "
                                    "span %g exceeds 2*pi.",
                                    usdPrim.GetPath().GetText(), fi, uSpan));
            } else if (t == cone && uSpan > _TwoPi + _DomainTol) {
                _Err(&errors,
                     UsdSolidValidationErrorNameTokens->surfaceDomainSpanExceeded,
                     usdPrim,
                     TfStringPrintf("[BA.563] BrepArray <%s>: cone face %zu U "
                                    "span %g exceeds 2*pi.",
                                    usdPrim.GetPath().GetText(), fi, uSpan));
            } else if (t == torus) {
                if (uSpan > _TwoPi + _DomainTol) {
                    _Err(&errors,
                         UsdSolidValidationErrorNameTokens
                             ->surfaceDomainSpanExceeded,
                         usdPrim,
                         TfStringPrintf("[BA.564] BrepArray <%s>: torus face %zu "
                                        "U span %g exceeds 2*pi.",
                                        usdPrim.GetPath().GetText(), fi, uSpan));
                }
                if (vSpan > _TwoPi + _DomainTol) {
                    _Err(&errors,
                         UsdSolidValidationErrorNameTokens
                             ->surfaceDomainSpanExceeded,
                         usdPrim,
                         TfStringPrintf("[BA.565] BrepArray <%s>: torus face %zu "
                                        "V span %g exceeds 2*pi.",
                                        usdPrim.GetPath().GetText(), fi, vSpan));
                }
            }
        }
    }

    // BA.570/571: circle/ellipse edge & wireEdge parameter spans.
    const TfToken circle("BrepCurve3dCircleAPI");
    const TfToken ellipse("BrepCurve3dEllipseAPI");
    struct Kind {
        VtArray<TfToken> curveType;
        VtArray<double> range;
        const char *label;
    };
    const std::vector<Kind> kinds = {
        { _Read<TfToken>(brep.GetEdgeCurveTypeAttr()),
          _Read<double>(brep.GetEdgeRangeAttr()), "edge" },
        { _Read<TfToken>(brep.GetWireEdgeCurveTypeAttr()),
          _Read<double>(brep.GetWireEdgeRangeAttr()), "wireEdge" },
    };
    for (const Kind &kind : kinds) {
        const size_t numE = kind.curveType.size();
        if (kind.range.size() < 2 * numE) {
            continue;
        }
        for (size_t ei = 0; ei < numE; ++ei) {
            const double span = kind.range[2 * ei + 1] - kind.range[2 * ei];
            if (kind.curveType[ei] == circle && span > _TwoPi + _DomainTol) {
                _Err(&errors,
                     UsdSolidValidationErrorNameTokens->curveParamSpanExceeded,
                     usdPrim,
                     TfStringPrintf("[BA.570] BrepArray <%s>: circle %s %zu "
                                    "parameter span %g exceeds 2*pi.",
                                    usdPrim.GetPath().GetText(), kind.label, ei,
                                    span));
            } else if (kind.curveType[ei] == ellipse
                       && span > _TwoPi + _DomainTol) {
                _Err(&errors,
                     UsdSolidValidationErrorNameTokens->curveParamSpanExceeded,
                     usdPrim,
                     TfStringPrintf("[BA.571] BrepArray <%s>: ellipse %s %zu "
                                    "parameter span %g exceeds 2*pi.",
                                    usdPrim.GetPath().GetText(), kind.label, ei,
                                    span));
            }
        }
    }

    return errors;
}

// -------------------------------------------------------------------------- //
// BrepArrayAnalyticCurves                                                    //
// -------------------------------------------------------------------------- //
UsdValidationErrorVector
_BrepArrayAnalyticCurves(const UsdPrim &usdPrim,
                         const UsdValidationTimeRange & /*timeRange*/)
{
    if (!(usdPrim && usdPrim.IsA<UsdSolidBrepArray>())) {
        return {};
    }
    const UsdSolidBrepArray brep(usdPrim);
    const VtArray<TfToken> edgeCurveType
        = _Read<TfToken>(brep.GetEdgeCurveTypeAttr());
    const VtArray<TfToken> wireCurveType
        = _Read<TfToken>(brep.GetWireEdgeCurveTypeAttr());

    const TfToken circle("BrepCurve3dCircleAPI");
    const TfToken line("BrepCurve3dLineAPI");
    const TfToken ellipse("BrepCurve3dEllipseAPI");

    UsdValidationErrorVector errors;

    _CheckCircleInstance(usdPrim, "edge3dCircle",
                         _CountToken(edgeCurveType, circle), &errors);
    _CheckCircleInstance(usdPrim, "wireEdge3dCircle",
                         _CountToken(wireCurveType, circle), &errors);
    _CheckLineInstance(usdPrim, "edge3dLine",
                       _CountToken(edgeCurveType, line), &errors);
    _CheckLineInstance(usdPrim, "wireEdge3dLine",
                       _CountToken(wireCurveType, line), &errors);
    _CheckEllipseInstance(usdPrim, "edge3dEllipse",
                          _CountToken(edgeCurveType, ellipse), &errors);
    _CheckEllipseInstance(usdPrim, "wireEdge3dEllipse",
                          _CountToken(wireCurveType, ellipse), &errors);

    return errors;
}

// -------------------------------------------------------------------------- //
// BrepArrayNurbs                                                             //
// -------------------------------------------------------------------------- //
UsdValidationErrorVector
_BrepArrayNurbs(const UsdPrim &usdPrim,
                const UsdValidationTimeRange & /*timeRange*/)
{
    if (!(usdPrim && usdPrim.IsA<UsdSolidBrepArray>())) {
        return {};
    }
    const UsdSolidBrepArray brep(usdPrim);
    const SdfValueTypeName uintA = SdfValueTypeNames->UIntArray;
    const SdfValueTypeName dblA = SdfValueTypeNames->DoubleArray;
    const SdfValueTypeName dbl2A = SdfValueTypeNames->Double2Array;
    const SdfValueTypeName p3A = SdfValueTypeNames->Point3dArray;

    UsdValidationErrorVector errors;

    // --- Surface NURBS (single-apply, no instance segment) --- //
    const VtArray<TfToken> faceSurfaceType
        = _Read<TfToken>(brep.GetFaceSurfaceTypeAttr());
    const size_t nSurf
        = _CountToken(faceSurfaceType, TfToken("BrepSurfaceNurbAPI"));
    const VtArray<unsigned int> uVC
        = _ReadName<unsigned int>(usdPrim, "brep:surface:nurb:uVertexCount");
    const VtArray<unsigned int> vVC
        = _ReadName<unsigned int>(usdPrim, "brep:surface:nurb:vVertexCount");
    const VtArray<unsigned int> uO
        = _ReadName<unsigned int>(usdPrim, "brep:surface:nurb:uOrder");
    const VtArray<unsigned int> vO
        = _ReadName<unsigned int>(usdPrim, "brep:surface:nurb:vOrder");
    if (nSurf > 0 || !uO.empty()) {
        if (uVC.size() != nSurf) {
            _Err(&errors, UsdSolidValidationErrorNameTokens->nurbSizeArrayMismatch,
                 usdPrim,
                 TfStringPrintf("[BA.420] BrepArray <%s>: brep:surface:nurb:"
                                "uVertexCount size %zu but expected %zu.",
                                usdPrim.GetPath().GetText(), uVC.size(), nSurf));
        }
        if (vVC.size() != nSurf) {
            _Err(&errors, UsdSolidValidationErrorNameTokens->nurbSizeArrayMismatch,
                 usdPrim,
                 TfStringPrintf("[BA.420] BrepArray <%s>: brep:surface:nurb:"
                                "vVertexCount size %zu but expected %zu.",
                                usdPrim.GetPath().GetText(), vVC.size(), nSurf));
        }
        if (uO.size() != nSurf) {
            _Err(&errors, UsdSolidValidationErrorNameTokens->nurbSizeArrayMismatch,
                 usdPrim,
                 TfStringPrintf("[BA.420] BrepArray <%s>: brep:surface:nurb:uOrder "
                                "size %zu but expected %zu.",
                                usdPrim.GetPath().GetText(), uO.size(), nSurf));
        }
        if (vO.size() != nSurf) {
            _Err(&errors, UsdSolidValidationErrorNameTokens->nurbSizeArrayMismatch,
                 usdPrim,
                 TfStringPrintf("[BA.420] BrepArray <%s>: brep:surface:nurb:vOrder "
                                "size %zu but expected %zu.",
                                usdPrim.GetPath().GetText(), vO.size(), nSurf));
        }
        _CheckNurbOrderPositive(usdPrim, "BA.425", "surface U", uO, uVC, false,
                                &errors);
        _CheckNurbOrderPositive(usdPrim, "BA.425", "surface V", vO, vVC, false,
                                &errors);
        _CheckNurbOrderLEVtx(usdPrim, "BA.430", "surface U", uO, uVC, false,
                             &errors);
        _CheckNurbOrderLEVtx(usdPrim, "BA.430", "surface V", vO, vVC, false,
                             &errors);
        _CheckNurbOrderMin2(usdPrim, "surface U", uO, uVC, false, &errors);
        _CheckNurbOrderMin2(usdPrim, "surface V", vO, vVC, false, &errors);
        _CheckNurbVtxGEOrder(usdPrim, "surface U", uO, uVC, false, &errors);
        _CheckNurbVtxGEOrder(usdPrim, "surface V", vO, vVC, false, &errors);

        const VtArray<GfVec3d> cv
            = _ReadName<GfVec3d>(usdPrim, "brep:surface:nurb:controlVertices");
        const VtArray<double> w
            = _ReadName<double>(usdPrim, "brep:surface:nurb:weights");
        size_t expectedCv = 0;
        const size_t ms = std::min(uVC.size(), vVC.size());
        for (size_t i = 0; i < ms; ++i) {
            expectedCv += static_cast<size_t>(uVC[i]) * vVC[i];
        }
        if (cv.size() != expectedCv || w.size() != expectedCv) {
            _Err(&errors,
                 UsdSolidValidationErrorNameTokens
                     ->nurbControlVertexWeightSizeMismatch,
                 usdPrim,
                 TfStringPrintf("[BA.435] BrepArray <%s>: surface controlVertices "
                                "(%zu) / weights (%zu) size but expected %zu "
                                "(sum of uVertexCount*vVertexCount).",
                                usdPrim.GetPath().GetText(), cv.size(), w.size(),
                                expectedCv));
        }
        _CheckNurbWeights(usdPrim, "BA.440", "surface", w, &errors);
        _CheckNurbKnots1D(
            usdPrim, "BA.445", "BA.455", "surface U", uO, uVC,
            _ReadName<double>(usdPrim, "brep:surface:nurb:uKnots"), &errors);
        _CheckNurbKnots1D(
            usdPrim, "BA.450", "BA.460", "surface V", vO, vVC,
            _ReadName<double>(usdPrim, "brep:surface:nurb:vKnots"), &errors);
        if (nSurf > 0 && uO.empty()) {
            _Err(&errors,
                 UsdSolidValidationErrorNameTokens->nurbSchemaDataIncomplete,
                 usdPrim,
                 TfStringPrintf("[BA.470] BrepArray <%s>: faces declare "
                                "BrepSurfaceNurbAPI but no brep:surface:nurb data "
                                "is authored.",
                                usdPrim.GetPath().GetText()));
        }
        if (_HasAppliedSchema(usdPrim, TfToken("BrepSurfaceNurbAPI"))
            && nSurf == 0) {
            _Err(&errors,
                 UsdSolidValidationErrorNameTokens->nurbSchemaUsageInconsistent,
                 usdPrim,
                 TfStringPrintf("[BA.470] BrepArray <%s>: BrepSurfaceNurbAPI is in "
                                "apiSchemas but no face uses "
                                "face:surfaceType=BrepSurfaceNurbAPI.",
                                usdPrim.GetPath().GetText()));
        }
        _CheckNurbType(usdPrim, "BA.471", "brep:surface:nurb:uOrder", uintA,
                       &errors);
        _CheckNurbType(usdPrim, "BA.471", "brep:surface:nurb:vOrder", uintA,
                       &errors);
        _CheckNurbType(usdPrim, "BA.471", "brep:surface:nurb:uVertexCount", uintA,
                       &errors);
        _CheckNurbType(usdPrim, "BA.471", "brep:surface:nurb:vVertexCount", uintA,
                       &errors);
        _CheckNurbType(usdPrim, "BA.471", "brep:surface:nurb:controlVertices",
                       p3A, &errors);
        _CheckNurbType(usdPrim, "BA.471", "brep:surface:nurb:weights", dblA,
                       &errors);
        _CheckNurbType(usdPrim, "BA.471", "brep:surface:nurb:uKnots", dblA,
                       &errors);
        _CheckNurbType(usdPrim, "BA.471", "brep:surface:nurb:vKnots", dblA,
                       &errors);
    }

    // --- Edge 3D NURBS (multi-apply instance edge3dNurb) --- //
    const VtArray<TfToken> edgeCurveType
        = _Read<TfToken>(brep.GetEdgeCurveTypeAttr());
    const size_t nEdge
        = _CountToken(edgeCurveType, TfToken("BrepCurve3dNurbAPI"));
    const VtArray<unsigned int> eO
        = _ReadName<unsigned int>(usdPrim, "brep:edge3dNurb:curve3d:nurb:order");
    const VtArray<unsigned int> eVC = _ReadName<unsigned int>(
        usdPrim, "brep:edge3dNurb:curve3d:nurb:vertexCount");
    if (nEdge > 0 || !eO.empty()) {
        if (eO.size() != nEdge) {
            _Err(&errors, UsdSolidValidationErrorNameTokens->nurbSizeArrayMismatch,
                 usdPrim,
                 TfStringPrintf("[BA.330] BrepArray <%s>: edge3dNurb order size "
                                "%zu but expected %zu.",
                                usdPrim.GetPath().GetText(), eO.size(), nEdge));
        }
        if (eVC.size() != nEdge) {
            _Err(&errors, UsdSolidValidationErrorNameTokens->nurbSizeArrayMismatch,
                 usdPrim,
                 TfStringPrintf("[BA.330] BrepArray <%s>: edge3dNurb vertexCount "
                                "size %zu but expected %zu.",
                                usdPrim.GetPath().GetText(), eVC.size(), nEdge));
        }
        _CheckNurbOrderPositive(usdPrim, "BA.335", "edge3d", eO, eVC, false,
                                &errors);
        _CheckNurbOrderLEVtx(usdPrim, "BA.340", "edge3d", eO, eVC, false,
                             &errors);
        _CheckNurbOrderMin2(usdPrim, "edge3d", eO, eVC, false, &errors);
        _CheckNurbVtxGEOrder(usdPrim, "edge3d", eO, eVC, false, &errors);

        const VtArray<GfVec3d> eCv = _ReadName<GfVec3d>(
            usdPrim, "brep:edge3dNurb:curve3d:nurb:controlVertices");
        const VtArray<double> eW
            = _ReadName<double>(usdPrim, "brep:edge3dNurb:curve3d:nurb:weights");
        size_t expectedCv = 0;
        for (unsigned int c : eVC) {
            expectedCv += c;
        }
        if (eCv.size() != expectedCv || eW.size() != expectedCv) {
            _Err(&errors,
                 UsdSolidValidationErrorNameTokens
                     ->nurbControlVertexWeightSizeMismatch,
                 usdPrim,
                 TfStringPrintf("[BA.345] BrepArray <%s>: edge3dNurb "
                                "controlVertices (%zu) / weights (%zu) size but "
                                "expected %zu (sum of vertexCount).",
                                usdPrim.GetPath().GetText(), eCv.size(),
                                eW.size(), expectedCv));
        }
        _CheckNurbWeights(usdPrim, "BA.350", "edge3d", eW, &errors);
        _CheckNurbKnots1D(
            usdPrim, "BA.355", "BA.360", "edge3d", eO, eVC,
            _ReadName<double>(usdPrim, "brep:edge3dNurb:curve3d:nurb:knots"),
            &errors);
        if (nEdge > 0 && eO.empty()) {
            _Err(&errors,
                 UsdSolidValidationErrorNameTokens->nurbSchemaDataIncomplete,
                 usdPrim,
                 TfStringPrintf("[BA.370] BrepArray <%s>: edges declare "
                                "BrepCurve3dNurbAPI but no brep:edge3dNurb data is "
                                "authored.",
                                usdPrim.GetPath().GetText()));
        }
        if (_HasAppliedSchema(usdPrim,
                              TfToken("BrepCurve3dNurbAPI:edge3dNurb"))
            && nEdge == 0) {
            _Err(&errors,
                 UsdSolidValidationErrorNameTokens->nurbSchemaUsageInconsistent,
                 usdPrim,
                 TfStringPrintf("[BA.370] BrepArray <%s>: "
                                "BrepCurve3dNurbAPI:edge3dNurb is in apiSchemas "
                                "but no edge uses edge:curveType="
                                "BrepCurve3dNurbAPI.",
                                usdPrim.GetPath().GetText()));
        }
        _CheckNurbType(usdPrim, "BA.371", "brep:edge3dNurb:curve3d:nurb:order",
                       uintA, &errors);
        _CheckNurbType(usdPrim, "BA.371",
                       "brep:edge3dNurb:curve3d:nurb:vertexCount", uintA,
                       &errors);
        _CheckNurbType(usdPrim, "BA.371",
                       "brep:edge3dNurb:curve3d:nurb:controlVertices", p3A,
                       &errors);
        _CheckNurbType(usdPrim, "BA.371", "brep:edge3dNurb:curve3d:nurb:weights",
                       dblA, &errors);
        _CheckNurbType(usdPrim, "BA.371", "brep:edge3dNurb:curve3d:nurb:knots",
                       dblA, &errors);
    }

    // --- WireEdge 3D NURBS (instance wireEdge3dNurb): schema usage + 590/591 - //
    const VtArray<TfToken> wireCurveType
        = _Read<TfToken>(brep.GetWireEdgeCurveTypeAttr());
    const size_t nWire
        = _CountToken(wireCurveType, TfToken("BrepCurve3dNurbAPI"));
    const VtArray<unsigned int> wO = _ReadName<unsigned int>(
        usdPrim, "brep:wireEdge3dNurb:curve3d:nurb:order");
    const VtArray<unsigned int> wVC = _ReadName<unsigned int>(
        usdPrim, "brep:wireEdge3dNurb:curve3d:nurb:vertexCount");
    if (nWire > 0 && wO.empty()) {
        _Err(&errors,
             UsdSolidValidationErrorNameTokens->nurbSchemaDataIncomplete, usdPrim,
             TfStringPrintf("[BA.290] BrepArray <%s>: wireEdges declare "
                            "BrepCurve3dNurbAPI but no brep:wireEdge3dNurb data is "
                            "authored.",
                            usdPrim.GetPath().GetText()));
    }
    if (_HasAppliedSchema(usdPrim, TfToken("BrepCurve3dNurbAPI:wireEdge3dNurb"))
        && nWire == 0) {
        _Err(&errors,
             UsdSolidValidationErrorNameTokens->nurbSchemaUsageInconsistent,
             usdPrim,
             TfStringPrintf("[BA.290] BrepArray <%s>: "
                            "BrepCurve3dNurbAPI:wireEdge3dNurb is in apiSchemas "
                            "but no wireEdge uses wireEdge:curveType="
                            "BrepCurve3dNurbAPI.",
                            usdPrim.GetPath().GetText()));
    }
    if (!wO.empty()) {
        _CheckNurbOrderMin2(usdPrim, "wireEdge3d", wO, wVC, false, &errors);
        _CheckNurbVtxGEOrder(usdPrim, "wireEdge3d", wO, wVC, false, &errors);
    }

    // --- Curve UV NURBS (single-apply, one trim curve per edgeuse) --- //
    const size_t euCount
        = _Read<unsigned int>(brep.GetEdgeuseEdgeIndexAttr()).size();
    const VtArray<unsigned int> cO
        = _ReadName<unsigned int>(usdPrim, "brep:curveUv:nurb:order");
    const VtArray<unsigned int> cVC
        = _ReadName<unsigned int>(usdPrim, "brep:curveUv:nurb:vertexCount");
    // BA.415 (schema-usage) is independent of whether curveUv value data is
    // authored: "API applied but no data / no edgeuses" is itself the failure.
    {
        const bool appliedUv
            = _HasAppliedSchema(usdPrim, TfToken("BrepCurveUvNurbAPI"));
        if (appliedUv && euCount > 0 && cO.empty()) {
            _Err(&errors,
                 UsdSolidValidationErrorNameTokens->nurbSchemaDataIncomplete,
                 usdPrim,
                 TfStringPrintf("[BA.415] BrepArray <%s>: BrepCurveUvNurbAPI is in "
                                "apiSchemas but no brep:curveUv:nurb data is "
                                "authored.",
                                usdPrim.GetPath().GetText()));
        }
        if (appliedUv && euCount == 0) {
            _Err(&errors,
                 UsdSolidValidationErrorNameTokens->nurbSchemaUsageInconsistent,
                 usdPrim,
                 TfStringPrintf("[BA.415] BrepArray <%s>: BrepCurveUvNurbAPI is in "
                                "apiSchemas but no edgeuses exist.",
                                usdPrim.GetPath().GetText()));
        }
    }
    const bool runUv = _IsAuthored(usdPrim, "brep:curveUv:nurb:order")
        || _IsAuthored(usdPrim, "brep:curveUv:nurb:vertexCount");
    if (runUv) {
        if (cO.size() != euCount) {
            _Err(&errors, UsdSolidValidationErrorNameTokens->nurbSizeArrayMismatch,
                 usdPrim,
                 TfStringPrintf("[BA.375] BrepArray <%s>: brep:curveUv:nurb:order "
                                "size %zu but expected %zu (edgeuse count).",
                                usdPrim.GetPath().GetText(), cO.size(), euCount));
        }
        if (cVC.size() != euCount) {
            _Err(&errors, UsdSolidValidationErrorNameTokens->nurbSizeArrayMismatch,
                 usdPrim,
                 TfStringPrintf("[BA.375] BrepArray <%s>: "
                                "brep:curveUv:nurb:vertexCount size %zu but "
                                "expected %zu (edgeuse count).",
                                usdPrim.GetPath().GetText(), cVC.size(),
                                euCount));
        }
        _CheckNurbOrderPositive(usdPrim, "BA.380", "curveUv", cO, cVC, true,
                                &errors);
        _CheckNurbOrderLEVtx(usdPrim, "BA.385", "curveUv", cO, cVC, true,
                             &errors);
        _CheckNurbOrderMin2(usdPrim, "curveUv", cO, cVC, true, &errors);
        _CheckNurbVtxGEOrder(usdPrim, "curveUv", cO, cVC, true, &errors);

        const VtArray<GfVec2d> cCv
            = _ReadName<GfVec2d>(usdPrim, "brep:curveUv:nurb:controlVertices");
        const VtArray<double> cW
            = _ReadName<double>(usdPrim, "brep:curveUv:nurb:weights");
        size_t expectedCv = 0;
        for (unsigned int c : cVC) {
            expectedCv += c;
        }
        if (cCv.size() != expectedCv) {
            _Err(&errors,
                 UsdSolidValidationErrorNameTokens
                     ->nurbControlVertexWeightSizeMismatch,
                 usdPrim,
                 TfStringPrintf("[BA.390] BrepArray <%s>: curveUv controlVertices "
                                "size %zu but expected %zu (sum of vertexCount).",
                                usdPrim.GetPath().GetText(), cCv.size(),
                                expectedCv));
        }
        if (!cW.empty() && cW.size() != expectedCv) {
            _Err(&errors,
                 UsdSolidValidationErrorNameTokens
                     ->nurbControlVertexWeightSizeMismatch,
                 usdPrim,
                 TfStringPrintf("[BA.405] BrepArray <%s>: curveUv weights size %zu "
                                "but expected %zu (sum of vertexCount).",
                                usdPrim.GetPath().GetText(), cW.size(),
                                expectedCv));
        }
        _CheckNurbWeights(usdPrim, "BA.410", "curveUv", cW, &errors);
        _CheckNurbKnots1D(usdPrim, "BA.395", "BA.400", "curveUv", cO, cVC,
                          _ReadName<double>(usdPrim, "brep:curveUv:nurb:knots"),
                          &errors);
        _CheckNurbType(usdPrim, "BA.416", "brep:curveUv:nurb:order", uintA,
                       &errors);
        _CheckNurbType(usdPrim, "BA.416", "brep:curveUv:nurb:vertexCount", uintA,
                       &errors);
        _CheckNurbType(usdPrim, "BA.416", "brep:curveUv:nurb:controlVertices",
                       dbl2A, &errors);
        _CheckNurbType(usdPrim, "BA.416", "brep:curveUv:nurb:knots", dblA,
                       &errors);
        _CheckNurbType(usdPrim, "BA.416", "brep:curveUv:nurb:weights", dblA,
                       &errors);
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

    registry.RegisterPluginValidator(
        UsdSolidValidatorNameTokens->brepArrayAuthorship, _BrepArrayAuthorship);

    registry.RegisterPluginValidator(
        UsdSolidValidatorNameTokens->brepArrayDataTypes, _BrepArrayDataTypes);

    registry.RegisterPluginValidator(
        UsdSolidValidatorNameTokens->brepArraySchemaUsage,
        _BrepArraySchemaUsage);

    registry.RegisterPluginValidator(
        UsdSolidValidatorNameTokens->brepArrayReferences, _BrepArrayReferences);

    registry.RegisterPluginValidator(
        UsdSolidValidatorNameTokens->brepArrayCompleteness,
        _BrepArrayCompleteness);

    registry.RegisterPluginValidator(
        UsdSolidValidatorNameTokens->brepArrayContainment,
        _BrepArrayContainment);

    registry.RegisterPluginValidator(
        UsdSolidValidatorNameTokens->brepArraySpans, _BrepArraySpans);

    registry.RegisterPluginValidator(
        UsdSolidValidatorNameTokens->brepArrayAnalyticCurves,
        _BrepArrayAnalyticCurves);

    registry.RegisterPluginValidator(
        UsdSolidValidatorNameTokens->brepArrayNurbs, _BrepArrayNurbs);
}

PXR_NAMESPACE_CLOSE_SCOPE
