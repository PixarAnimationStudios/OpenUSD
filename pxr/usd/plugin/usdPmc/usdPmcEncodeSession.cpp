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

#include <algorithm>
#include <cstdlib>
#include <string>
#include <iostream>
#include <vector>
#include <functional>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

//=============================================================================
// Coordinate system definition

struct PmcEncodeSession::CoordSys {
    pmc::Rational scale;
    std::vector<int> origin;
};

//=============================================================================
// Helpers

namespace {

using CoordSys = PmcEncodeSession::CoordSys;

/// Template utility for validating expected values during PMC encoding
/// operations.
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

/// Type trait to determine the component count of vector types.
template<typename T, typename Enable = void>
constexpr size_t oneextent_v = std::max(size_t(1), std::extent_v<T>);

template<typename T>
constexpr size_t oneextent_v<T, typename std::enable_if_t<GfIsGfVec<T>::value>> = T::dimension;

/// Analyzes the VtValue type to determine how many components each element has.
int
GetExtentFromType(const VtValue& vtv)
{
    if (vtv.IsEmpty())
        return 0;

    switch (vtv.GetKnownValueTypeIndex()) {
        case VtGetKnownValueTypeIndex<VtArray<GfVec2i>>(): return 2;
        case VtGetKnownValueTypeIndex<VtArray<GfVec2f>>(): return 2;
        case VtGetKnownValueTypeIndex<VtArray<GfVec2h>>(): return 2;
        case VtGetKnownValueTypeIndex<VtArray<GfVec2d>>(): return 2;
        case VtGetKnownValueTypeIndex<VtArray<GfVec3i>>(): return 3;
        case VtGetKnownValueTypeIndex<VtArray<GfVec3f>>(): return 3;
        case VtGetKnownValueTypeIndex<VtArray<GfVec3h>>(): return 3;
        case VtGetKnownValueTypeIndex<VtArray<GfVec3d>>(): return 3;
        case VtGetKnownValueTypeIndex<VtArray<GfVec4i>>(): return 4;
        case VtGetKnownValueTypeIndex<VtArray<GfVec4f>>(): return 4;
        case VtGetKnownValueTypeIndex<VtArray<GfVec4h>>(): return 4;
        case VtGetKnownValueTypeIndex<VtArray<GfVec4d>>(): return 4;

        case VtGetKnownValueTypeIndex<VtArray<bool>>(): return 1;
        case VtGetKnownValueTypeIndex<VtArray<int>>(): return 1;
        case VtGetKnownValueTypeIndex<VtArray<float>>(): return 1;
        case VtGetKnownValueTypeIndex<VtArray<double>>(): return 1;
        case VtGetKnownValueTypeIndex<VtArray<GfHalf>>(): return 1;
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

// Determine min&max bounds of buffer values.
std::vector<MinMax>
GetMinMax(const pmc::ArrayBuffer& vals)
{
    auto fn = [&vals](const auto* ptr) {
        std::vector<MinMax> minmax(vals.componentsPerVector);
        for (int i = 0; i < vals.vectorCount; i++)
            for (int k = 0; k < vals.componentsPerVector; k++, ptr++) {
                minmax[k].min = std::min(minmax[k].min, double(*ptr));
                minmax[k].max = std::max(minmax[k].max, double(*ptr));
            }
        return minmax;
    };

    switch (vals.dataType) {
        case pmc::DataType::Float32: return fn((const float*)vals.data);
        case pmc::DataType::Float64: return fn((const double*)vals.data);
        default: return {};
    }
}

/// Derive coding coordinate system using number of fractional bits to scale
// values for quantization parameters.
CoordSys
MakeCoordSysByFracBits(const pmc::ArrayBuffer& vals, const Qparams& qp)
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

    CoordSys csys;
    csys.scale.p = fracbits >= 0 ? 1 << fracbits : 1;
    csys.scale.q = fracbits < 0 ? 1 << -fracbits : 1;
    csys.origin.assign(vals.componentsPerVector, 0);
    return csys;
}

/// Derive coding coordinate system mapping bounding box to maxsigbits.
CoordSys
MakeCoordSysByBBox(const pmc::ArrayBuffer& vals, const Qparams& qp)
{
    const auto minmaxs = GetMinMax(vals);
    double range = 0.;
    for (auto [min, max] : minmaxs)
        range = std::max(range, max - min);

    // NB: rounding signed values may cause qbits to be exceeded;
    // eg: {-1,1} -> Round[{-127.5,127.5}]
    auto scale = ((1 << qp.maxsigbits) - 1) / range;
    for (auto [min, max] : minmaxs) {
      auto qMax = int(std::round(max * scale));
      auto qMin = int(std::round(min * scale));
      if (qMax - qMin >= (1 << qp.maxsigbits)) {
        scale = ((1 << qp.maxsigbits) - 2) / range;
        break;
      }
    }

    CoordSys csys;
    int32_t exp;
    csys.scale.p = (int32_t) std::scalbn(std::frexp(scale, &exp), 24);
    csys.scale.q = 1 << (24 - exp);
    while (!(csys.scale.p & 1) && !(csys.scale.q & 1)) {
        csys.scale.q >>= 1;
        csys.scale.p >>= 1;
    }

    scale = float(csys.scale);
    for (auto [min, max] : minmaxs)
        csys.origin.push_back(int(std::round(min * scale)));

    return csys;
}

/// Derive coding coordinate system using environment defined method.
// If environment variable USD_PMC_COORDSYS_BBOX is 1, bbox method is used.
// Otherwise, the more appropriate fractional bit method is used.
CoordSys
MakeCoordSys(const pmc::ArrayBuffer& vals, const Qparams& qp)
{
    static bool coordSysModeIsFracBits = [](){
        const auto ev = getenv("USD_PMC_COORDSYS_BBOX");
        return (!ev || ev[0] != '1');
    }();
    if (!coordSysModeIsFracBits)
        return MakeCoordSysByBBox(vals, qp);
    return MakeCoordSysByFracBits(vals, qp);
}

/// Derive coding coordinate system for mesh geometry.
CoordSys
MakeCoordSys(const pmc::GeometryMeshpart& gmp, const Qparams& qp)
{
    return MakeCoordSys(gmp.buffers.positions, qp);
}

/// Derive coding coordinate system for attributes.
CoordSys
MakeCoordSys(const pmc::AttributeMeshpart& amp, const Qparams& qp)
{
    // for Normals, don't offset; choose a sensible scale.
    if (amp.info.type == pmc::AttributeType::NORMAL) {
        CoordSys cs;
        cs.scale = {1 << (qp.maxsigbits - 1), 1};
        cs.origin.assign(amp.info.componentsPerVector, 0);
        return cs;
    }

    return MakeCoordSys(amp.buffers.values, qp);
}

/// Insert coordnate system into geometry info
void
operator<<(pmc::GeometryInfo& gi, const CoordSys& csys)
{
    gi.coordSys = csys.scale;
    for (int k = 0; k < 3; k++)
        gi.coordSysOrigin[k] = csys.origin[k];
}

/// Insert coordnate system into attribute info
void
operator<<(pmc::AttributeInfo& ai, const CoordSys& csys)
{
    auto& dst = ai.coordSys.emplace();
    dst.scale = csys.scale;
    for (auto origink : csys.origin) {
        dst.origin.push_back(origink);
        dst.originScaleLog2.push_back(0);
    }
}

// Size of pmc buffer type
size_t
GetStride(pmc::DataType type)
{
    switch (type) {
    case pmc::DataType::Float32: return sizeof(float);
    case pmc::DataType::Float64: return sizeof(double);
    case pmc::DataType::Int32: return sizeof(int32_t);
    case pmc::DataType::UInt32: return sizeof(uint32_t);
    }
    throw std::runtime_error("unknown type");
}

/// Wrap data in pmc::ArrayBuffer
pmc::ArrayBuffer
ToPmc(uint8_t* data, size_t width, size_t length, pmc::DataType type)
{
    pmc::ArrayBuffer result;
    result.data = data;
    result.offset = 0;
    result.stride = width * GetStride(type);
    result.vectorCount = length;
    result.componentsPerVector = width;
    result.dataType = type;
    return result;
}

/// Wrap VtArray in pmc::ArrayBuffer
template<typename T>
pmc::ArrayBuffer
ToPmc(const VtArray<T>& src, pmc::DataType type)
{
    return ToPmc((uint8_t*)src.data(), oneextent_v<T>, src.size(), type);
}

/// Wrap VtArray<int> in pmc::ArrayBuffer
pmc::ArrayBuffer
ToPmc(const VtArray<int>& src)
{
    return ToPmc(src, pmc::DataType::Int32);
}

/// Wrap VtArray-containing VtValue in pmc::ArrayBuffer
pmc::ArrayBuffer
ToPmc(const VtValue& src)
{
    if (src.IsEmpty())
        return ToPmc(nullptr, 0, 0, pmc::DataType::Int32);

    switch (src.GetKnownValueTypeIndex()) {
#define CASE(T, type) case VtGetKnownValueTypeIndex<T>(): \
            return ToPmc(src.UncheckedGet<T>(), type)
        CASE(VtArray<GfVec2i>, pmc::DataType::Int32);
        CASE(VtArray<GfVec2f>, pmc::DataType::Float32);
        //CASE(VtArray<GfVec2h>, pmc::DataType::Float16);
        CASE(VtArray<GfVec2d>, pmc::DataType::Float64);
        CASE(VtArray<GfVec3i>, pmc::DataType::Int32);
        CASE(VtArray<GfVec3f>, pmc::DataType::Float32);
        //CASE(VtArray<GfVec3h>, pmc::DataType::Float16);
        CASE(VtArray<GfVec3d>, pmc::DataType::Float64);
        CASE(VtArray<GfVec4i>, pmc::DataType::Int32);
        CASE(VtArray<GfVec4f>, pmc::DataType::Float32);
        //CASE(VtArray<GfVec4h>, pmc::DataType::Float16);
        CASE(VtArray<GfVec4d>, pmc::DataType::Float64);
        //CASE(VtArray<bool>, pmc::DataType::Int8);
        CASE(VtArray<int>, pmc::DataType::Int32);
        CASE(VtArray<float>, pmc::DataType::Float32);
        //CASE(VtArray<GfHalf>, pmc::DataType::Float16);
        CASE(VtArray<double>, pmc::DataType::Float64);
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

    // If pvName ends with "_uv"
    constexpr std::string_view suff {"_uv"};
    std::string_view ps = pvName.GetString();
    if (ps.compare(ps.length() - suff.length(), suff.length(), suff) == 0)
        return pmc::AttributeType::TEX_COORD;

    return pmc::AttributeType::USER_DEFINED_START;
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
            case pmc::AttributeScope::CORNER: return pmc::AttributeIndicesCodingStrategy::ADAPTIVE;
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
    using AT = pmc::AttributeType;
    switch (ampi.type) {
        case AT::TEX_COORD: return pmc::TraversalStrategy::CONNECTIVITY_GUIDED;
        case AT::NORMAL:    return pmc::TraversalStrategy::GEOMETRY_DEFINED;
        default:            return pmc::TraversalStrategy::ADAPTIVE;
    }
}

/// Pick the traversal strategy.
pmc::PredictionStrategy
GetPredictionStrategyForAttr(const pmc::AttributeMeshpartInfo& ampi)
{
    using AT = pmc::AttributeType;
    using PS = pmc::PredictionStrategy;
    switch (ampi.type) {
        case AT::TEX_COORD:     return PS::TEX_COORD_GEOMETRY_GUIDED;
        case AT::NORMAL:        return PS::UNITARY_OCTAHEDRAL_NORMAL_VECTOR;
        default:                return PS::LINEAR;
    }
}

//=============================================================================
// :: Quantization

/// Scale and round floating-point values according to coordSys.
struct Quantizer {
    float _scale;
    std::vector<float> _offset;

    Quantizer(float scale, int32_t* origin, size_t width);
    int32_t* operator()(int32_t* dst, const double* src, size_t width) const;
};

Quantizer::Quantizer(float scale, int32_t* origin, size_t width)
    : _scale(scale)
{
    auto invScale = 1.0 / _scale;
    for (size_t k = 0; k < width; k++)
        _offset.push_back(origin[k] * invScale);
}

int32_t*
Quantizer::operator()(int32_t* dst, const double* src, size_t width) const
{
    for (size_t k = 0; k < width; k++)
        dst[k] = int(std::round((src[k] - _offset[k]) * _scale));
    return dst + width;
}

struct QuantizerOctahedral {
    double _oneOcs;
    QuantizerOctahedral(float scale) : _oneOcs(scale) {}
    int32_t* operator()(int32_t* dst, const double* src, size_t width) const;
};

int32_t*
QuantizerOctahedral::operator()(int32_t* dst, const double* src, size_t width)
const {
    double sum = std::abs(src[0]) + std::abs(src[1]) + std::abs(src[2]);
    if (sum == 0.0)
        sum = 1.0;

    double vScaled[3];
    for (int32_t k = 0; k < 3; ++k) {
        // Normalize `value` in terms of L1-norm, scale by a factor of one.
        vScaled[k] = src[k] / sum * _oneOcs;
        // Using std::trunc instead of std::round to make sure that
        // oneOcs - 3 <= sumInt <= oneOcs when exiting this loop
        dst[k] = (int32_t)std::trunc(vScaled[k]);
    }

    double sumInt = std::abs(dst[0]) + std::abs(dst[1]) + std::abs(dst[2]);
    if (_oneOcs - sumInt > 1) {
        for (int32_t k = 0; k < 3; ++k) {
            sumInt -= std::abs(dst[k]);
            dst[k] += vScaled[k] < 0 ? -1 : 1;
            sumInt += std::abs(dst[k]);
        }
    }

    // make sure sumInt == oneOcs
    if (sumInt != _oneOcs) {
        int err = _oneOcs - sumInt;
        int bl = -1;
        double ba = std::numeric_limits<double>::max();

        for (int32_t l = 0; l < 3; ++l) {
            double dot = 0;
            double n_a = 0;
            double val[3];
            for (int32_t k = 0; k < 3; ++k) {
                val[k] = dst[k] + (k == l ? (vScaled[k] < 0 ? -err : err) : 0);
                dot += vScaled[k] * val[k];
                n_a += vScaled[k] * vScaled[k];
            }

            double w = dot > 0.0 ? n_a / dot : 1.0;
            double dist = 0.0;
            for (int32_t k = 0; k < 3; ++k) {
                double d = vScaled[k] - w * val[k];
                dist += d * d;
            }

            if (dist < ba) {
                ba = dist;
                bl = l;
            }
        }

        dst[bl] += vScaled[bl] < 0 ? -err : err;
    }

    return dst + 3;
}

//=============================================================================
// :: Buffer conversion, applying convert(...) to each vector

pmc::ArrayBuffer
ConvertBuffer(
    pmc::ArrayBuffer& buffer,
    std::vector<int32_t>& backing,
    std::function<int32_t*(int32_t*,double*,size_t)> convert)
{
    if (buffer.dataType == pmc::DataType::Int32)
        return buffer;

    const auto fn = [&](auto* buf) {
        using T = std::remove_pointer_t<decltype(buf)>;
        std::vector<double> tmp(buffer.componentsPerVector);
        int32_t* out = backing.data();
        for (size_t i = 0; i < size_t(buffer.vectorCount); i++) {
            std::copy_n(buffer.vectorAtIndex<T>(i), tmp.size(), tmp.data());
            out = convert(out, tmp.data(), tmp.size());
        }
    };

    backing.resize(buffer.componentsPerVector * buffer.vectorCount);
    switch (buffer.dataType) {
        case pmc::DataType::Float32: fn((float*) buffer.data); break;
        case pmc::DataType::Float64: fn((double*) buffer.data); break;
        case pmc::DataType::Int32:   fn((int32_t*) buffer.data); break;
        case pmc::DataType::UInt32:  fn((uint32_t*) buffer.data); break;
        default: return buffer;
    }

    return ToPmc((uint8_t*)backing.data(), buffer.componentsPerVector,
            buffer.vectorCount, pmc::DataType::Int32);
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
    _gmp.info.frameOrderCount = 0;
    _gmp.info.meshpartId = 0;
    _gmp.info.faceType = GetMeshFaceTypeFromFaceVertexCounts(fvcs);
    _gmp.info.faceCount = fvcs.size();
    _gmp.info.vertexCount = vtxs.GetArraySize();
    _gmp.info.indexCount = vidxs.size();

    // Convert USD data to PMC buffer format
    _gmp.buffers.positions = ToPmc(vtxs);
    _gmp.buffers.faceDegrees = ToPmc(fvcs);
    _gmp.buffers.indices = ToPmc(vidxs);

    // Keep references to data alive
    _keepAlive.emplace_back(vtxs);
    _keepAlive.emplace_back(fvcs);
    _keepAlive.emplace_back(vidxs);

    // Calculate quantization parameters for vertex positions
    auto& usdname = UsdGeomTokens->points;
    _gmp.info << MakeCoordSys(_gmp, QparamsFromOptions(_options, usdname));

    // Track which attributes have been processed
    processedAttributes.insert (_ugm.GetFaceVertexCountsAttr().GetName());
    processedAttributes.insert (_ugm.GetFaceVertexIndicesAttr().GetName());
    processedAttributes.insert (_ugm.GetPointsAttr().GetName());
}

pmc::AttributeMeshpart&
PmcEncodeSession::_setupAttr(VtValue vals, VtArray<int> idxs)
{
    auto& amp = _amps.emplace_back();
    _gmp.attMeshparts.push_back(&amp);

    amp.info.frameOrderCount = 0,
    amp.info.meshpartId = 0,
    amp.info.attributeId = _amps.size() - 1;
    amp.info.componentsPerVector = GetExtentFromType(vals);
    amp.info.type = pmc::AttributeType::USER_DEFINED_START;
    // todo: fix api to remove one of these
    amp.info.outputVectorCount = amp.info.vectorCount = vals.GetArraySize(),
    amp.info.outputIndexCount = amp.info.indexCount = idxs.size(),
    amp.info.explicitIndices = !idxs.empty();

    if (!vals.IsEmpty()) {
        amp.buffers.values = ToPmc(vals);
        _keepAlive.emplace_back(vals);
    }

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

    auto& amp = _setupAttr(vals, idxs);
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

    amp.info << MakeCoordSys(amp, QparamsFromOptions(_options, pvName));

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
    amp.info.type = pmc::AttributeType::FACE_GROUP;
    amp.info.scope = pmc::AttributeScope::FACE;
    amp.info.indicesInterpretation = pmc::IndicesInterpretation::SCOPE_INDEXING;
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

    using pmc::IndicesInterpretation;
    auto& ampIdxs = _setupAttr(lens, idxs);
    ampIdxs.info.type = pmc::AttributeType::CREASE;
    ampIdxs.info.scope = pmc::AttributeScope::VERTEX;
    ampIdxs.info.indicesInterpretation = IndicesInterpretation::SCOPE_INDEXING;
    ampIdxs.info.sparse = true;

    auto& usdname = UsdGeomTokens->creaseSharpnesses;
    auto& ampVals = _setupAttr(vals, {});
    ampVals.info.type = pmc::AttributeType::SHARPNESS;
    ampVals.info.scope = pmc::AttributeScope::DERIVED;
    ampVals.info.derivedScope.scopedAttributeId = ampIdxs.info.attributeId;
    ampVals.info.indicesInterpretation = IndicesInterpretation::VALUE_INDEXING;
    ampVals.info.sparse = true;
    ampVals.info.jsonCustomAui = JsonAuiForAttr(attrVals);
    ampVals.info << MakeCoordSys(ampVals, QparamsFromOptions(_options, usdname));

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
        auto& usdname = UsdGeomTokens->normals;
        auto vals = GetAs<VtValue>(attr);
        auto& amp = _setupAttr(vals, {});
        amp.info.type = pmc::AttributeType::NORMAL;
        amp.info.scope = GetScopeFromUsd(_ugm.GetNormalsInterpolation());
        amp.info.jsonCustomAui = JsonAuiForAttr(attr);
        amp.info.name = attr.GetName();
        amp.info << MakeCoordSys(amp, QparamsFromOptions(_options, usdname));
        processedAttributes.insert (amp.info.name);
    }

    if (const auto attr = _ugm.GetHoleIndicesAttr(); attr.HasAuthoredValue()) {
        using pmc::IndicesInterpretation;
        auto& amp = _setupAttr({}, GetAs<VtArray<int>>(attr));
        amp.info.type = pmc::AttributeType::HOLE;
        amp.info.scope = pmc::AttributeScope::FACE;
        amp.info.indicesInterpretation = IndicesInterpretation::SCOPE_INDEXING;
        amp.info.sparse = true;
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
    pmc::GeometryEncodingParameters geomOpts;

    // Configure attribute encoding parameters
    pmc::AttributeEncodingParameters attrOpts;

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

    // Temporary buffer for conversions
    std::vector<int32_t> tmp;

    // Encode geometry meshpart
    if (1) {
        Quantizer q(float(_gmp.info.coordSys), _gmp.info.coordSysOrigin, 3);
        _gmp.buffers.positions = ConvertBuffer(_gmp.buffers.positions, tmp, q);
    }
    constexpr Expect throwOnError {pmc::Error::OK};
    throwOnError = _enc.encode(_gmp, dstBuf, geomOpts);

    // Encode all attribute meshparts
    for (auto& amp : _amps) {
        attrOpts.indicesCodingStrategy = GetIndicesStrategyForAttr(amp.info);
        attrOpts.traversalStrategy = GetTraversalStrategyForAttr(amp.info);
        attrOpts.predictionStrategy = GetPredictionStrategyForAttr(amp.info);

        if (amp.info.coordSys) {
            auto& cs = *amp.info.coordSys;
            if (amp.info.type == pmc::AttributeType::NORMAL) {
                QuantizerOctahedral q(float(cs.scale));
                amp.buffers.values = ConvertBuffer(amp.buffers.values, tmp, q);
            } else {
                Quantizer q(float(cs.scale), cs.origin.data(), cs.origin.size());
                amp.buffers.values = ConvertBuffer(amp.buffers.values, tmp, q);
            }
        }

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
    return _encode();
}

PXR_NAMESPACE_CLOSE_SCOPE
