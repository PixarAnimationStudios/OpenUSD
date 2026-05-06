//
// Copyright 2025 Apple
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

/// \file usdPmc/usdPmcEncodeSession.cpp
///
/// Implementation of the PMC encoding session for USD meshes.
/// This file contains the core logic for converting USD mesh data into PMC
/// compressed format, handling geometry, attributes, primvars, subsets, and
/// creases.
///
/// The encoding session:
/// - Extracts and quantizes mesh geometry (vertices, faces, indices)
/// - Processes all mesh attributes and primvars with appropriate type
///   conversion
/// - Handles geometry subsets as face groups
/// - Manages crease data for subdivision surfaces
/// - Applies quantization and scaling for optimal compression
/// - Generates JSON metadata for proper reconstruction during decoding

#include "usdPmcEncodeSession.hpp"
#include "usdPmcConstantPrivate.hpp"
#include "usdPmcEncoder.hpp"

#include "pxr/pxr.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/base/gf/traits.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/vt/types.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdGeom/subset.h"
#include "pxr/usd/usdGeom/tokens.h"

#include <string>
#include <iostream>
#include <vector>
#include <functional>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

//=============================================================================
// Helpers

namespace {

/// Validate expected values during PMC encoding operations.
template<typename T>
struct Expect {
    const T expected;
    constexpr Expect(T expected) : expected(expected) {}

    /// Check that x matches expected value; throws std::runtime_error if not.
    const T& operator=(const T& x) const;
};

template<typename T>
const T&
Expect<T>::operator=(const T& x) const
{
    if (x != expected) {
        auto xi = std::underlying_type_t<T>(x);
        throw std::runtime_error("Expectation failure " + std::to_string(xi));
    }
    return x;
}

/// Extract typed values from USD attributes at earliest time code.
template<typename T>
T
GetAs(const UsdAttribute& attr)
{
    T values;
    attr.Get(&values, UsdTimeCode::EarliestTime());
    return values;
};

/// Template struct to get VtValue type indices at compile time.
template<typename T>
struct VtVtindex {
    static constexpr int index = VtGetKnownValueTypeIndex<T>();
};

template<typename T>
constexpr auto VtVtindex_v = VtVtindex<T>::index;

/// Type trait to determine the component count of vector types.
template<typename T, typename Enable = void>
constexpr size_t oneextent_v = std::max(size_t(1), std::extent_v<T>);

template<typename T>
constexpr size_t oneextent_v<T, typename std::enable_if_t<GfIsGfVec<T>::value>> = T::dimension;

/// Analyzes the VtValue type to determine how many components each element has.
int
GetExtentFromType(const VtValue& vtv)
{
    switch (vtv.GetKnownValueTypeIndex()) {
        case VtVtindex_v<VtArray<GfVec2i>>: return 2;
        case VtVtindex_v<VtArray<GfVec2f>>: return 2;
        case VtVtindex_v<VtArray<GfVec2h>>: return 2;
        case VtVtindex_v<VtArray<GfVec2d>>: return 2;
        case VtVtindex_v<VtArray<GfVec3i>>: return 3;
        case VtVtindex_v<VtArray<GfVec3f>>: return 3;
        case VtVtindex_v<VtArray<GfVec3h>>: return 3;
        case VtVtindex_v<VtArray<GfVec3d>>: return 3;
        case VtVtindex_v<VtArray<GfVec4i>>: return 4;
        case VtVtindex_v<VtArray<GfVec4f>>: return 4;
        case VtVtindex_v<VtArray<GfVec4h>>: return 4;
        case VtVtindex_v<VtArray<GfVec4d>>: return 4;

        case VtVtindex_v<VtArray<bool>>: return 1;
        case VtVtindex_v<VtArray<int>>: return 1;
        case VtVtindex_v<VtArray<float>>: return 1;
        case VtVtindex_v<VtArray<double>>: return 1;
        case VtVtindex_v<VtArray<GfHalf>>: return 1;
    }

    // can't handle other types
    throw std::runtime_error("unknown extent for type");
}

struct Qparams
{
    /// Quantization precision: number of fractional bits to retain
    int fracbits = 10;

    /// Quantization maximum significant digit limit. Adjust fracbits so as
    /// not to produce more than maxsigbits.
    int maxsigbits = 14;
};

struct MinMax {
    double min = std::numeric_limits<double>().max();
    double max = std::numeric_limits<double>().lowest();
};

std::vector<MinMax>
GetMinMax(const VtValue& vals)
{
    auto fnGfVec = [](const auto& vta) {
        using T = typename std::decay_t<decltype(vta)>::value_type;
        std::vector<MinMax> minmax(T::dimension);
        for (const auto& sv : vta)
            for (int k = 0; k < T::dimension; k++) {
                minmax[k].min = std::min(minmax[k].min, double(sv[k]));
                minmax[k].max = std::max(minmax[k].max, double(sv[k]));
            }
        return minmax;
    };

    auto fnScalar = [](const auto& vta) {
        using T = typename std::decay_t<decltype(vta)>::value_type;
        std::vector<MinMax> minmax(1);
        for (const auto v : vta) {
            minmax[0].min = std::min(minmax[0].min, double(v));
            minmax[0].max = std::max(minmax[0].max, double(v));
        }
        return minmax;
    };

    switch (vals.GetKnownValueTypeIndex()) {
#define CASE(T, fn) VtVtindex_v<T>: return fn(vals.UncheckedGet<T>())
        case CASE(VtArray<GfVec2f>, fnGfVec);
        case CASE(VtArray<GfVec2h>, fnGfVec);
        case CASE(VtArray<GfVec2d>, fnGfVec);
        case CASE(VtArray<GfVec3f>, fnGfVec);
        case CASE(VtArray<GfVec3h>, fnGfVec);
        case CASE(VtArray<GfVec3d>, fnGfVec);
        case CASE(VtArray<GfVec4f>, fnGfVec);
        case CASE(VtArray<GfVec4h>, fnGfVec);
        case CASE(VtArray<GfVec4d>, fnGfVec);
        // todo: need to take into account elementsize
        case CASE(VtArray<float>, fnScalar);
        case CASE(VtArray<GfHalf>, fnScalar);
        case CASE(VtArray<double>, fnScalar);
        default: return {};
#undef CASE
    }
}

/// Determine number of fractional bits to scale values for quantization
/// parameters.
int
GetFracBits(const VtValue& vals, Qparams qp)
{
    // integer bits, ignores sign
    int intbits = 0;
    for (auto [min, max] : GetMinMax(vals)) {
        int exp;
        std::frexp(std::max(std::abs(min), std::abs(max)), &exp);
        intbits = std::max(intbits, exp);
    }

    // number of fractional bits, limited by int + frac bits to maxsigbits
    intbits--;
    int fracbits = std::min(intbits + qp.fracbits, qp.maxsigbits) - intbits;
    return fracbits;
}

/// Explicitly constructs int from T.
template<typename T>
struct IntCast {
    int operator()(T v) const { return T(v); }
};

/// Integer case for scaling: perform cast instead.
template<typename T, typename Enable = void>
struct Q : public IntCast<T>
{};

/// Scale and round floating-point values by 2^fracbits.
template<typename T>
struct Q<T, typename std::enable_if_t<std::is_floating_point_v<T>>> {
    const int _fracbits;
    int operator()(T v) const {
        return int(std::round(std::scalbnf(v, _fracbits)));
    }
};

/// Sequentially apply op to each elementary value in the VtArray.
template<typename T, typename Op>
void
ScanVtArray(const VtArray<T>& vta, Op op)
{
    for (const auto& sv : vta) {
        if constexpr (GfIsGfVec<T>::value)
            for (int i = 0; i < T::dimension; i++)
                op(sv[i]);
        else
            op(sv);
    }
}

/// Convert source buffer to std::vector<int> by applying unary operation to
/// each element.
template<typename T, typename Op>
pmc::ArrayBuffer
ToPmc(const VtArray<T>& src, std::vector<int>& conv, Op op)
{
    // Convert and flatten buffer
    conv.resize(src.size() * oneextent_v<T>);
    ScanVtArray(src, [it = conv.begin(), op](auto v) mutable {
        *it++ = op(v);
    });

    pmc::ArrayBuffer result;
    result.data = (uint8_t*) conv.data();
    result.offset = 0;
    result.stride = oneextent_v<T> * sizeof(int);
    result.vectorCount = src.size();
    result.componentsPerVector = oneextent_v<T>;
    result.dataType = pmc::DataType::Int32;
    return result;
}

/// Special case, no conversion required.
pmc::ArrayBuffer
ToPmc(const VtArray<int>& src)
{
    pmc::ArrayBuffer result;
    result.data = (uint8_t*) src.data();
    result.offset = 0;
    result.stride = sizeof(int);
    result.vectorCount = src.size();
    result.componentsPerVector = 1;
    result.dataType = pmc::DataType::Int32;
    return result;
}

/// Convert source array to destination vector<int> with optional scaling.
pmc::ArrayBuffer
ToPmc(const VtValue& src, std::vector<int>& dst, int fracbits)
{
    switch (src.GetKnownValueTypeIndex()) {
#define CASE(T, op) VtVtindex_v<T>: return ToPmc(src.UncheckedGet<T>(), dst, op)
        case CASE(VtArray<GfVec2i>, IntCast<int>{});
        case CASE(VtArray<GfVec2f>, Q<float>{fracbits});
        case CASE(VtArray<GfVec2h>, Q<float>{fracbits});
        case CASE(VtArray<GfVec2d>, Q<double>{fracbits});
        case CASE(VtArray<GfVec3i>, IntCast<int>{});
        case CASE(VtArray<GfVec3f>, Q<float>{fracbits});
        case CASE(VtArray<GfVec3h>, Q<float>{fracbits});
        case CASE(VtArray<GfVec3d>, Q<double>{fracbits});
        case CASE(VtArray<GfVec4i>, IntCast<int>{});
        case CASE(VtArray<GfVec4f>, Q<float>{fracbits});
        case CASE(VtArray<GfVec4h>, Q<float>{fracbits});
        case CASE(VtArray<GfVec4d>, Q<double>{fracbits});
        case CASE(VtArray<bool>, IntCast<int>{});
        case CASE(VtArray<int>, IntCast<int>{});
        case CASE(VtArray<float>, Q<float>{fracbits});
        case CASE(VtArray<GfHalf>, Q<float>{fracbits});
        case CASE(VtArray<double>, Q<double>{fracbits});
#undef CASE
    }

    // can't handle other types
    throw std::runtime_error("missing buffer type converter");
}

/// Look-up an attribute's options-specified quantization parameters.
// First looks up options["qbits"][name]; if not found, then with name = "*";
// if still not found, uses default values.
Qparams QparamsFromOptions(const VtDictionary& options, const TfToken& name)
{
    auto it = options.find("qbits");
    if (it == options.end() || !it->second.IsHolding<VtDictionary>())
        return {};

    const auto vtv =
        [dict = it->second.UncheckedGet<VtDictionary>(), name]() -> VtValue {
            for (const auto* str : {name.data(), "*"})
                if (auto it = dict.find(str); it != dict.end())
                    return it->second;
            return {};
        }();

    if (!vtv.IsEmpty())
        if (int nbits = vtv.Get<int>(); vtv.IsHolding<int>())
            return { .fracbits = nbits, .maxsigbits = nbits };

    return {};
}

//=============================================================================
// :: Mesh properties

pmc::MeshFaceType
GetMeshFaceTypeFromFaceVertexCounts(const VtArray<int> fvcs)
{
    const auto minmax = std::minmax_element(fvcs.cbegin(), fvcs.cend());
    const int min = *minmax.first;
    const int max = *minmax.second;

    if (max == min && min == 3) return pmc::MeshFaceType::TRIANGULAR;
    if (max == min && min == 4) return pmc::MeshFaceType::QUADRILATERAL;
    if (max == 4 && min == 3)   return pmc::MeshFaceType::TRIANGULAR_QUADRILATERAL;
    return pmc::MeshFaceType::POLYGONAL;
}

pmc::AttributeScope
GetScopeFromUsd(pxr::TfToken interp)
{
    if (interp == pxr::UsdGeomTokens->vertex)       return pmc::AttributeScope::VERTEX;
    if (interp == pxr::UsdGeomTokens->varying)      return pmc::AttributeScope::VERTEX;
    if (interp == pxr::UsdGeomTokens->faceVarying)  return pmc::AttributeScope::CORNER;
    if (interp == pxr::UsdGeomTokens->uniform)      return pmc::AttributeScope::FACE;

    throw std::runtime_error(
        std::string("cannot convert interpolation type ") + interp.GetString());
}

/// Try and guess the attribute type from the name.
pmc::AttributeType
GuessAttributeType(const TfToken pvRole, const TfToken pvName)
{
    if (pvRole == pxr::SdfValueRoleNames->Color)              return pmc::AttributeType::COLOR;
    if (pvRole == pxr::SdfValueRoleNames->Normal)             return pmc::AttributeType::NORMAL;
    if (pvRole == pxr::SdfValueRoleNames->TextureCoordinate)  return pmc::AttributeType::TEX_COORD;

    if (pvName == "uv" || pvName == "UV" || pvName == "st")   return pmc::AttributeType::TEX_COORD;
    if (pvName == pxr::UsdGeomTokens->normals)                return pmc::AttributeType::NORMAL;
    if (pvName == "displayColor")                             return pmc::AttributeType::COLOR;
    
    std::string ps = pvName.GetString();
    std::string suff = std::string("_uv");
    // If pvName ends with "_uv"
    if (ps.compare(ps.length() - suff.length(), suff.length(), suff) == 0)
        return pmc::AttributeType::TEX_COORD;

    return pmc::AttributeType::USER_DEFINED;
}

//=============================================================================
// :: Encoder parameter selection

/// Pick an index coding strategy.
pmc::AttributeIndicesCodingStrategy
GetIndicesStrategyForAttr(const pmc::AttributeMeshpartInfo& ampi)
{
    if (!ampi.indexCount)
        return pmc::AttributeIndicesCodingStrategy::SKIP;

    if (!ampi.sparse) {
        switch (ampi.scope) {
            case pmc::AttributeScope::CORNER: return pmc::AttributeIndicesCodingStrategy::CORNER_BASED;
            case pmc::AttributeScope::VERTEX: return pmc::AttributeIndicesCodingStrategy::VERTEX_BASED;
            case pmc::AttributeScope::FACE:   return pmc::AttributeIndicesCodingStrategy::CORNER_BASED;
            case pmc::AttributeScope::EDGE:      break; /* this isn't supported */
            case pmc::AttributeScope::DERIVED:   break; /* sparse only */
            case pmc::AttributeScope::EXTERNAL:  break; /* sparse only */
            case pmc::AttributeScope::UNDEFINED: break; /* this should be removed from the API */
        }
        return pmc::AttributeIndicesCodingStrategy::SKIP;
    }

    /* sparse attribtues */
    switch (ampi.scope) {
        case pmc::AttributeScope::VERTEX:
            // todo: re-enable vertex based index coding for creases when crease
            //       data is ordered correctly.
            if ([[maybe_unused]] const bool creasesAreOrderedCorrectly = 0)
                if (ampi.type == pmc::AttributeType::CREASE)
                    return pmc::AttributeIndicesCodingStrategy::EDGE_BASED;
            [[fallthrough]];

        default:
            return pmc::AttributeIndicesCodingStrategy::SPARSE_BASED;
    }
}

/// Pick the traversal strategy.
pmc::TraversalStrategy
GetTraversalStrategyForAttr(const pmc::AttributeMeshpartInfo& ampi)
{
    return pmc::TraversalStrategy::ADAPTIVE;
}

/// Pick the traversal strategy.
pmc::PredictionStrategy
GetPredictionStrategyForAttr(const pmc::AttributeMeshpartInfo& ampi)
{
    switch (ampi.type) {
        case pmc::AttributeType::TEX_COORD: return pmc::PredictionStrategy::TEX_COORD_GEOMETRY_GUIDED;
        default:                            return pmc::PredictionStrategy::LINEAR;
    }
}

//=============================================================================

}    // namespace <anon>

//=============================================================================

/// Set up geometry data for PMC encoding.
void
PmcEncodeSession::_setupGeom()
{
    // Extract core UsdGeomMesh primitives with their standard specification
    const auto fvcs = GetAs<VtArray<int>>(_ugm.GetFaceVertexCountsAttr());
    const auto vidxs = GetAs<VtArray<int>>(_ugm.GetFaceVertexIndicesAttr());
    const auto vtxs = GetAs<VtValue>(_ugm.GetPointsAttr());

    // Validate that essential geometry data is present
    if (fvcs.empty() || vidxs.empty() || vtxs.IsEmpty()) {
        throw std::runtime_error("missing geometry");
    }

    // Configure geometry meshpart information
    _gmp.info.bitDepth = 0;
    _gmp.info.frameOrderCount = 0;
    _gmp.info.meshpartId = 0;
    _gmp.info.faceType = GetMeshFaceTypeFromFaceVertexCounts(fvcs);
    // todo: fix api to remove one of these
    _gmp.info.outputFaceCount = _gmp.info.faceCount = fvcs.size();
    _gmp.info.outputVertexCount = _gmp.info.vertexCount = vtxs.GetArraySize();
    _gmp.info.outputIndexCount =_gmp.info.indexCount = vidxs.size();

    // Calculate quantization parameters for vertex positions
    auto& usdname = UsdGeomTokens->points;
    int fb = GetFracBits(vtxs, QparamsFromOptions(_options, usdname));
    _gmp.info.coordSys = {1 << fb, 1};

    // Convert USD data to PMC buffer format
    _gmp.buffers.positions = ToPmc(vtxs, _converted.emplace_back(), fb);
    _gmp.buffers.faceDegrees = ToPmc(fvcs);
    _gmp.buffers.indices = ToPmc(vidxs);

    // Keep references to data that doesn't need conversion
    _keepAlive.emplace_back(fvcs);
    _keepAlive.emplace_back(vidxs);

    // Track which attributes have been processed
    processedAttributes.insert (_ugm.GetFaceVertexCountsAttr().GetName());
    processedAttributes.insert (_ugm.GetFaceVertexIndicesAttr().GetName());
    processedAttributes.insert (_ugm.GetPointsAttr().GetName());
}

pmc::AttributeMeshpart&
PmcEncodeSession::_setupAttr(VtValue vals, VtArray<int> idxs, int fracbits = 0)
{
    auto& amp = _amps.emplace_back();
    _gmp.attMeshparts.push_back(&amp);

    amp.info.frameOrderCount = 0,
    amp.info.meshpartId = 0,
    amp.info.attributeId = _amps.size() - 1;
    amp.info.bitDepth = 0;
    amp.info.componentsPerVector = GetExtentFromType(vals);
    amp.info.type = pmc::AttributeType::USER_DEFINED;
    amp.info.vectorCount = vals.GetArraySize(),
    amp.info.indexCount = idxs.size(),
    amp.info.explicitIndices = !idxs.empty();

    if (fracbits)
        amp.info.coordSys = {1 << fracbits, 1};

    // todo: don't need to keep vals alive if conversion was performed
    amp.buffers.values = ToPmc(vals, _converted.emplace_back(), fracbits);
    _keepAlive.emplace_back(vals);

    if (!idxs.empty()) {
        amp.buffers.indices = ToPmc(idxs);
        _keepAlive.emplace_back(idxs);
    }

    return amp;
}

/// Generate PMC JSON attribute usability information.
static std::string
JsonAuiForAttr(const UsdAttribute& attr)
{
    std::ostringstream os;
    os << "{\"" << kUSDJsonMainKey << "\":{";

    // todo: this returns things like 'color3f[]', whereas it should be 'float'
    //       (as long as the role can be determined from amp.info.type)
    // Replaced, because we need the array type
    // os << "\"tn\":\"" << attr.GetTypeName().GetScalarType() << '"';
    os << "\"" << kUSDJsonTypeNameKey << "\":\"" << attr.GetTypeName() << '"';

    os << "}}";

    return std::move(os).str();
}

/// Setup a single primvar.
void
PmcEncodeSession::_setupPrimvar(const UsdGeomPrimvar& pv)
{
    // Ignore constant primitives (no point encoding them)
    if (pv.GetInterpolation() == pxr::UsdGeomTokens->constant) {
        return;
    }

    auto pvName = pv.GetPrimvarName();

    const auto vals = GetAs<VtValue>(pv);
    const auto idxs = GetAs<VtArray<int>>(pv.GetIndicesAttr());
    if (!vals.IsArrayValued() || vals.IsHolding<VtArray<std::string>>() || vals.IsHolding<VtArray<TfToken>>()) {
        return;
    }

    int fb = GetFracBits(vals, QparamsFromOptions(_options, pvName));
    auto& amp = _setupAttr(vals, idxs, fb);
    amp.info.scope = GetScopeFromUsd(pv.GetInterpolation());
    amp.info.type = GuessAttributeType(pv.GetTypeName().GetRole(), pvName);

    // flat arrays may have elementSize set manually, eg: jointIndices
    // todo: check these are treated as values ...
    if (auto width = pv.GetElementSize(); width > 1) {
        amp.info.componentsPerVector = width;
        amp.info.vectorCount /= width;
        amp.buffers.values.componentsPerVector = width;
        amp.buffers.values.vectorCount /= width;
        amp.buffers.values.stride *= width;
    }

    // metadata
    amp.info.name = pv.GetName();
    amp.info.jsonCustomAui = JsonAuiForAttr(pv);
    processedAttributes.insert (amp.info.name);
    if (pv.IsIndexed())
        processedAttributes.insert (amp.info.name + ":indices");
}

/// Build face group information from all subsets.
void
PmcEncodeSession::_setupGeomSubsets()
{
    auto sets = UsdGeomSubset::GetAllGeomSubsets(_ugm);
    if (sets.empty()) {
        return;
    }

    // Build array of lengths
    VtArray<int> vals;
    vals.reserve(sets.size());
    size_t totalLen = 0;
    for (const auto& set : sets) {
        auto len = GetAs<VtValue>(set.GetIndicesAttr()).GetArraySize();
        vals.push_back(len);
        totalLen += len;
    }

    // Catenate all the subset indices into a single array
    VtArray<int> idxs(totalLen);
    auto idxsIt = idxs.begin();
    for (const auto& set : sets) {
        const auto setIdxs = GetAs<VtArray<int>>(set.GetIndicesAttr());
        idxsIt = std::copy_n(setIdxs.begin(), setIdxs.size(), idxsIt);
    }

    auto& amp = _setupAttr(VtValue(vals), idxs);
    amp.info.type = pmc::AttributeType::FACE_GROUP_ID;
    amp.info.scope = pmc::AttributeScope::FACE;
    amp.info.sparse = true;

    // Generate list of subset names
    // todo: use proper json formatter to escape the string
    std::ostringstream os;
    os << "{\"" << kUSDJsonMainKey << "\":{\"" << kUSDJsonSubmeshNamesKey
       << "\":";
    char sep = '[';
    for (const auto& set : sets) {
        processedSubSets.insert (set.GetPrim().GetName());
        os << sep << '\"' << set.GetPrim().GetName() << '\"';
        sep = ',';
    }
    os << "]}}";

    amp.info.jsonCustomAui = std::move(os).str();
}

/// Setup crease data for PMC encoding.
void
PmcEncodeSession::_setupCreases()
{
    const auto attrIdxs = _ugm.GetCreaseIndicesAttr();
    const auto attrLens = _ugm.GetCreaseLengthsAttr();
    const auto attrVals = _ugm.GetCreaseSharpnessesAttr();

    for (const auto& attr : {attrIdxs, attrLens, attrVals})
        if (!attr.HasAuthoredValue())
            return;

    const auto idxs = GetAs<VtArray<int>>(attrIdxs);
    const auto lens = GetAs<VtValue>(attrLens);
    const auto vals = GetAs<VtValue>(attrVals);

    auto& ampIdxs = _setupAttr(lens, idxs);
    ampIdxs.info.type = pmc::AttributeType::CREASE;
    ampIdxs.info.scope = pmc::AttributeScope::VERTEX;
    ampIdxs.info.sparse = true;

    auto& usdname = UsdGeomTokens->creaseSharpnesses;
    int fb = GetFracBits(vals, QparamsFromOptions(_options, usdname));
    auto& ampVals = _setupAttr(vals, {}, fb);
    ampVals.info.type = pmc::AttributeType::SHARPNESS;
    ampVals.info.scope = pmc::AttributeScope::DERIVED;
    ampVals.info.derivedScope.scopedAttributeId = ampIdxs.info.attributeId;
    ampVals.info.sparse = true;
    ampVals.info.jsonCustomAui = JsonAuiForAttr(attrVals);

    processedAttributes.insert (_ugm.GetCreaseIndicesAttr().GetName());
    processedAttributes.insert (_ugm.GetCreaseLengthsAttr().GetName());
    processedAttributes.insert (_ugm.GetCreaseSharpnessesAttr().GetName());
}

/// Find all attributes for coding.
void
PmcEncodeSession::_setupAttrs()
{
    for (auto& pv : UsdGeomPrimvarsAPI(_ugm).GetPrimvarsWithAuthoredValues()) {
        _setupPrimvar(pv);
    }

    // If there are non-primvar normals, code them after the primvar version
    // NB: for rendering, primvars should have priority; we preserve all data
    if (const auto attr = _ugm.GetNormalsAttr(); attr.HasAuthoredValue()) {
        auto vals = GetAs<VtValue>(attr);
        auto& usdname = UsdGeomTokens->normals;
        int fb = GetFracBits(vals, QparamsFromOptions(_options, usdname));
        auto& amp = _setupAttr(vals, {}, fb);
        amp.info.type = pmc::AttributeType::NORMAL;
        amp.info.scope = GetScopeFromUsd(_ugm.GetNormalsInterpolation());
        amp.info.jsonCustomAui = JsonAuiForAttr(attr);
        amp.info.name = attr.GetName();
        processedAttributes.insert (amp.info.name);
    }

    if (const auto attr = _ugm.GetHoleIndicesAttr(); attr.HasAuthoredValue()) {
        auto& amp = _setupAttr(GetAs<VtValue>(attr), {});
        amp.info.type = pmc::AttributeType::HOLE;
        amp.info.scope = pmc::AttributeScope::FACE;
        amp.info.jsonCustomAui = JsonAuiForAttr(attr);
        amp.info.name = attr.GetName();
        processedAttributes.insert (amp.info.name);
    }

    _setupGeomSubsets();
    _setupCreases();
}

/// Configure the PMC encoder with geometry and attribute information.
void
PmcEncodeSession::_configurePmc()
{
    constexpr Expect throwOnError {pmc::Error::OK};

    // Configure geometry meshpart
    throwOnError = _enc.configure(_gmp.info);

    // Configure all attribute meshparts
    for (const auto& amp : _amps) {
        throwOnError = _enc.configure(amp.info);
    }
}

/// Perform the actual PMC encoding operation.
std::vector<uint8_t>
PmcEncodeSession::_encode()
{
    // Configure geometry encoding parameters
    // todo: enable all of these options
    pmc::GeometryEncodingParameters geomOpts;
    geomOpts.deduplicateVertices = false;
    geomOpts.deduplicateVerticesSimple = false;

    // Configure attribute encoding parameters
    // todo: enable all of these options
    pmc::AttributeEncodingParameters attrOpts;
    attrOpts.deduplicateValuesBitfield = 0;

    // Estimate the total size needed for the output buffer
    size_t estSize = 0;
    estSize += _enc.estimateMaxEncodedSize(_gmp, geomOpts);
    for (const auto& amp : _amps) {
        attrOpts.indicesCodingStrategy = GetIndicesStrategyForAttr(amp.info);
        attrOpts.traversalStrategy = GetTraversalStrategyForAttr(amp.info);
        attrOpts.predictionStrategy = GetPredictionStrategyForAttr(amp.info);
        estSize += _enc.estimateMaxEncodedSize(amp, attrOpts);
    }

    // Allocate output buffer and create PMC byte buffer
    std::vector<uint8_t> dst(estSize);
    pmc::ByteBuffer dstBuf {dst.size(), 0, dst.data()};

    constexpr Expect throwOnError {pmc::Error::OK};

    // Encode geometry meshpart
    throwOnError = _enc.encode(_gmp, dstBuf, geomOpts);

    // Encode all attribute meshparts
    for (const auto& amp : _amps) {
        attrOpts.indicesCodingStrategy = GetIndicesStrategyForAttr(amp.info);
        attrOpts.traversalStrategy = GetTraversalStrategyForAttr(amp.info);
        attrOpts.predictionStrategy = GetPredictionStrategyForAttr(amp.info);

        throwOnError = _enc.encode(amp, dstBuf, attrOpts);
    }

    // Resize to actual encoded size and return
    dst.resize(dstBuf.size);
    return dst;
}

std::vector<uint8_t>
PmcEncodeSession::encode()
{
    // Set up core mesh geometry (vertices, faces, indices)
    _setupGeom();

    // Process all mesh attributes, primvars, subsets, and creases
    _setupAttrs();

    // Configure the PMC encoder with all mesh parts
    _configurePmc();

    // Perform the actual PMC encoding
    // TODO: ideally conversion should happen here for a fast-path failure
    return _encode();
}

PXR_NAMESPACE_CLOSE_SCOPE
