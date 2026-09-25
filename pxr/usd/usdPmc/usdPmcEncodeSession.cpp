//
// Copyright 2026 Apple
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

#include "usdPmcEncodeSession.h"
#include "usdPmcConstantPrivate.h"
#include "usdPmcEncoder.h"

#include "pxr/pxr.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/base/gf/traits.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/visitValue.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdGeom/subset.h"
#include "pxr/usd/usdGeom/tokens.h"

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <string>
#include <iostream>
#include <vector>
#include <functional>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

//=============================================================================
// Coordinate system definition

struct UsdPmc_EncodeSession::CoordSys {
    pmc::Rational scale;
    std::vector<int> origin;
};

//=============================================================================
// Helpers

namespace {

using CoordSys = UsdPmc_EncodeSession::CoordSys;

/// Template utility for validating expected values during PMC encoding
/// operations.
template<typename T>
struct _Expect {
    const T expected;
    constexpr _Expect(T expected) : expected(expected) {}

    /// Check that x matches expected value; throws std::runtime_error if not.
    const T& operator=(const T& x) const;
};

template<typename T>
const T&
_Expect<T>::operator=(const T& x) const
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
_GetAs(const UsdAttribute& attr)
{
    T values;
    attr.Get(&values, UsdTimeCode::EarliestTime());
    return values;
};

/// Type trait to determine the component count of vector types.
template<typename T, typename Enable = void>
constexpr size_t oneextent_v = std::max(size_t(1), std::extent_v<T>);

template<typename T>
constexpr size_t oneextent_v<T, 
typename std::enable_if_t<GfIsGfVec<T>::value>> = T::dimension;

/// Catches non-array and unknown/empty values.
template <class T>
struct _GetExtent {
    static int Visit() {
        throw std::runtime_error("unknown extent for type");
    }
};

/// Returns dimension for arrays of supported data types.
/// Unsupported vector element types will trigger the runtime exception.
template <class T>
struct _GetExtent<VtArray<T>> {
    static int Visit() {
        if constexpr (GfIsGfVec<T>::value ||
                      std::is_same_v<T, bool> ||
                      std::is_same_v<T, int> ||
                      std::is_same_v<T, float> ||
                      std::is_same_v<T, double> ||
                      std::is_same_v<T, GfHalf>) {
            return int(oneextent_v<T>);
        } else {
            throw std::runtime_error("unknown extent for type");
        }
    }
};

/// Analyzes the VtValue type to determine how many components each 
// element has.
int
_GetExtentFromType(const VtValue& vtv)
{
    if (vtv.IsEmpty())
        return 0;

    return VtVisitValueType<_GetExtent>(vtv);
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
_GetMinMax(const pmc::ArrayBuffer& vals)
{
    auto fn = [&vals](const auto* ptr) {
        std::vector<MinMax> minmax(vals.componentsPerVector);
        for (size_t i = 0; i < vals.vectorCount; i++)
            for (size_t k = 0; k < vals.componentsPerVector; k++, ptr++) {
                minmax[k].min = std::min(minmax[k].min, double(*ptr));
                minmax[k].max = std::max(minmax[k].max, double(*ptr));
            }
        return minmax;
    };

    switch (vals.dataType) {
        case pmc::DataType::Float32: return fn((const float*)vals.data);
        case pmc::DataType::Float64: return fn((const double*)vals.data);
        case pmc::DataType::Int32:   return fn((const int32_t*)vals.data);
        case pmc::DataType::UInt32:  return fn((const uint32_t*)vals.data);
    }

    throw std::runtime_error("cannot compute min/max for unknown data type");
}

/// Derive coding coordinate system using number of fractional bits to scale
// values for quantization parameters.
CoordSys
_MakeCoordSysByFracBits(const pmc::ArrayBuffer& vals, const Qparams& qp)
{
    // integer bits, ignores sign
    int intbits = 0;
    for (auto [min, max] : _GetMinMax(vals)) {
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

/// Derive coding coordinate system using fractional bit method.
CoordSys
_MakeCoordSys(const pmc::ArrayBuffer& vals, const Qparams& qp)
{
    return _MakeCoordSysByFracBits(vals, qp);
}

/// Derive coding coordinate system for mesh geometry.
CoordSys
_MakeCoordSys(const pmc::GeometryMeshpart& gmp, const Qparams& qp)
{
    return _MakeCoordSys(gmp.buffers.positions, qp);
}

/// Derive coding coordinate system for attributes.
CoordSys
_MakeCoordSys(const pmc::AttributeMeshpart& amp, const Qparams& qp)
{
    // for Normals, don't offset; choose a sensible scale.
    if (amp.info.type == pmc::AttributeType::NORMAL) {
        CoordSys cs;
        cs.scale = {1 << (qp.maxsigbits - 1), 1};
        cs.origin.assign(amp.info.componentsPerVector, 0);
        return cs;
    }

    return _MakeCoordSys(amp.buffers.values, qp);
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
_ToPmc(uint8_t* data, size_t width, size_t length, pmc::DataType type)
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
_ToPmc(const VtArray<T>& src, pmc::DataType type)
{
    return _ToPmc((uint8_t*)src.data(), oneextent_v<T>, src.size(), type);
}

/// Wrap VtArray<int> in pmc::ArrayBuffer
pmc::ArrayBuffer
_ToPmc(const VtArray<int>& src)
{
    return _ToPmc(src, pmc::DataType::Int32);
}

template <class Scalar>
constexpr std::optional<pmc::DataType>
_PmcScalarDataType()
{
    if constexpr (std::is_same_v<Scalar, int>)
        return pmc::DataType::Int32;
    else if constexpr (std::is_same_v<Scalar, float>)
        return pmc::DataType::Float32;
    else if constexpr (std::is_same_v<Scalar, double>)
        return pmc::DataType::Float64;
    else
        return std::nullopt;
}

template <class T>
constexpr std::optional<pmc::DataType>
_PmcElementDataType()
{
    if constexpr (GfIsGfVec<T>::value)
        return _PmcScalarDataType<typename T::ScalarType>();
    else if constexpr (std::is_arithmetic_v<T>)
        return _PmcScalarDataType<T>();
    else
        return std::nullopt;
}

struct _ToPmcVisitor {
    // Determine PMC element type for the VtArray and perform conversion
    template <class T>
    pmc::ArrayBuffer operator()(const VtArray<T>& src) const {
        constexpr std::optional<pmc::DataType> dataType =
            _PmcElementDataType<T>();
        if constexpr (dataType.has_value())
            return _ToPmc(src, *dataType);
        else
            return (*this)(VtValue{});
    }

    /// Non array types or arrays with unsupported element types.
    pmc::ArrayBuffer operator()(const VtValue&) const {
        throw std::runtime_error("missing buffer type converter");
    }
};

/// Wrap VtArray-containing VtValue in pmc::ArrayBuffer
pmc::ArrayBuffer
_ToPmc(const VtValue& src)
{
    if (src.IsEmpty())
        return _ToPmc(nullptr, 0, 0, pmc::DataType::Int32);

    return VtVisitValue(src, _ToPmcVisitor{});
}

Qparams _QparamsDefault(const TfToken& pvRole)
{
    if (pvRole == SdfValueRoleNames->Color) {
        return Qparams{8,8};
    }
    if (pvRole == SdfValueRoleNames->Normal) {
        return Qparams{10,10};
    }
    if (pvRole == SdfValueRoleNames->TextureCoordinate) {
        return Qparams{12,12};
    }
    return Qparams{14,14};
}

/// Look-up an attribute's options-specified quantization parameters.
// First looks up options["mesh-qbits"][name]; if not found, then with name = "*";
// if still not found, uses default values.
Qparams _QparamsFromOptions(const VtDictionary& options, const TfToken& name, const TfToken& pvRole)
{
    auto it = options.find("mesh-qbits");
    if (it == options.end() || !it->second.IsHolding<VtDictionary>())
        return _QparamsDefault(pvRole);

    const auto vtv =
        [dict = it->second.UncheckedGet<VtDictionary>(), name]() -> VtValue {
            for (const auto* str : {name.data(), "*"})
                if (auto it = dict.find(str); it != dict.end())
                    return it->second;
            return {};
        }();

    if (!vtv.IsEmpty())
        if (int nbits = vtv.Get<int>(); vtv.IsHolding<int>())
            return Qparams{nbits, nbits};

    return _QparamsDefault(pvRole);
}

//=============================================================================
// :: Mesh properties

pmc::MeshFaceType
_GetMeshFaceTypeFromFaceVertexCounts(const VtArray<int> fvcs)
{
    const auto minmax = std::minmax_element(fvcs.cbegin(), fvcs.cend());
    const int min = *minmax.first;
    const int max = *minmax.second;

    using MFT = pmc::MeshFaceType;
    if (max == min && min == 3) {
        return MFT::TRIANGULAR;
    }
    if (max == min && min == 4) {
        return MFT::QUADRILATERAL;
    }
    if (max == 4 && min == 3) {
        return MFT::TRIANGULAR_QUADRILATERAL;
    }
    return MFT::POLYGONAL;
}

pmc::AttributeScope
_GetScopeFromUsd(TfToken interp)
{
    using AS = pmc::AttributeScope;
    if (interp == UsdGeomTokens->vertex) {
        return AS::VERTEX;
    }
    if (interp == UsdGeomTokens->varying) {
        return AS::VERTEX;
    }
    if (interp == UsdGeomTokens->faceVarying) {
        return AS::CORNER;
    }
    if (interp == UsdGeomTokens->uniform) {
        return AS::FACE;
    }

    throw std::runtime_error(
        std::string("cannot convert interpolation type ") + interp.GetString());
}

/// Try and guess the attribute type from the name.
pmc::AttributeType
_GuessAttributeType(const TfToken pvRole, const TfToken pvName)
{
    using AT = pmc::AttributeType;
    if (pvRole == SdfValueRoleNames->Color) {
        return AT::COLOR;
    }
    if (pvRole == SdfValueRoleNames->Normal) {
        return AT::NORMAL;
    }
    if (pvRole == SdfValueRoleNames->TextureCoordinate) {
        return AT::TEX_COORD;
    }

    if (pvName == "uv" || pvName == "UV" || pvName == "st") {
        return AT::TEX_COORD;
    }
    if (pvName == UsdGeomTokens->normals) {
        return AT::NORMAL;
    }
    if (pvName == "displayColor") {
        return AT::COLOR;
    }

    // If pvName ends with "_uv"
    constexpr std::string_view suff {"_uv"};
    std::string_view ps = pvName.GetString();
    if (ps.length() >= suff.length() &&
        ps.compare(ps.length() - suff.length(), suff.length(), suff) == 0) {
        return AT::TEX_COORD;
    }

    return AT::USER_DEFINED_START;
}

//=============================================================================
// :: Encoder parameter selection

/// Pick an index coding strategy.
pmc::AttributeIndicesCodingStrategy
_GetIndicesStrategyForAttr(const pmc::AttributeMeshpartInfo& ampi)
{
    using AICS = pmc::AttributeIndicesCodingStrategy;

    if (!ampi.sparse) {
        switch (ampi.scope) {
            using AS = pmc::AttributeScope;
            case AS::CORNER: return AICS::ADAPTIVE;
            case AS::VERTEX: return AICS::VERTEX_BASED;
            case AS::FACE:   return AICS::CORNER_BASED;
            case AS::EDGE:      break; /* this isn't supported */
            case AS::DERIVED:   break; /* sparse only */
            case AS::EXTERNAL:  break; /* sparse only */
            case AS::UNDEFINED: break; /* this should be removed from the API */
        }
        return AICS::SKIP;
    }

    /* sparse attribtues */
    switch (ampi.scope) {
        case pmc::AttributeScope::VERTEX:
            // todo: re-enable vertex based index coding for creases when crease
            //       data is ordered correctly.
            if ([[maybe_unused]] const bool creasesAreOrderedCorrectly = 0)
                if (ampi.type == pmc::AttributeType::CREASE)
                    return AICS::EDGE_BASED;
            [[fallthrough]];

        default:
            return AICS::SPARSE_BASED;
    }
}

/// Pick the traversal strategy.
pmc::TraversalStrategy
_GetTraversalStrategyForAttr(const pmc::AttributeMeshpartInfo& ampi)
{
    using AT = pmc::AttributeType;
    using TS = pmc::TraversalStrategy;
    switch (ampi.type) {
        case AT::TEX_COORD: return TS::CONNECTIVITY_GUIDED;
        case AT::NORMAL:    return TS::GEOMETRY_DEFINED;
        default:            return TS::ADAPTIVE;
    }
}

/// Pick the traversal strategy.
pmc::PredictionStrategy
_GetPredictionStrategyForAttr(const pmc::AttributeMeshpartInfo& ampi)
{
    using AT = pmc::AttributeType;
    using AS = pmc::AttributeScope;
    using PS = pmc::PredictionStrategy;

    switch (ampi.type) {
        default:                 return PS::LINEAR;
        case AT::TEX_COORD:      return PS::TEX_COORD_GEOMETRY_GUIDED;
        case AT::NORMAL:
            switch (ampi.scope) {
                // XXX: For AS::CORNER and AS::VERTEX, info can be examined to
                // determine if PS::UNITARY_OCTAHEDRAL_NORMAL_VECTOR strategy
                // may be used.
                case AS::CORNER:    return PS::LINEAR;
                case AS::VERTEX:    return PS::LINEAR;
                default:            return PS::LINEAR;
            }
    }
}

//=============================================================================
// :: Quantization

/// Scale and round floating-point values according to coordSys.
struct _Quantizer {
    float _scale;
    std::vector<float> _offset;

    _Quantizer(float scale, int32_t* origin, size_t width);
    int32_t* operator()(int32_t* dst, const double* src, size_t width) const;
};

_Quantizer::_Quantizer(float scale, int32_t* origin, size_t width)
    : _scale(scale)
{
    auto invScale = 1.0 / _scale;
    for (size_t k = 0; k < width; k++)
        _offset.push_back(origin[k] * invScale);
}

int32_t*
_Quantizer::operator()(int32_t* dst, const double* src, size_t width) const
{
    for (size_t k = 0; k < width; k++)
        dst[k] = int(std::round((src[k] - _offset[k]) * _scale));
    return dst + width;
}

struct _QuantizerOctahedral {
    double _oneOcs;
    _QuantizerOctahedral(float scale) : _oneOcs(scale) {}
    int32_t* operator()(int32_t* dst, const double* src, size_t width) const;
};

int32_t*
_QuantizerOctahedral::operator()(int32_t* dst, const double* src, size_t width)
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
_ConvertBuffer(
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

    return _ToPmc((uint8_t*)backing.data(), buffer.componentsPerVector,
            buffer.vectorCount, pmc::DataType::Int32);
}

//=============================================================================

}    // namespace <anon>

//=============================================================================

/// Set up geometry data for PMC encoding.
void
UsdPmc_EncodeSession::_setupGeom()
{
    // Extract core UsdGeomMesh primitives with their standard specification
    const auto fvcs = _GetAs<VtArray<int>>(_ugm.GetFaceVertexCountsAttr());
    const auto vidxs = _GetAs<VtArray<int>>(_ugm.GetFaceVertexIndicesAttr());
    const auto vtxs = _GetAs<VtValue>(_ugm.GetPointsAttr());

    // Validate that essential geometry data is present
    if (fvcs.empty() || vidxs.empty() || vtxs.IsEmpty()) {
        throw std::runtime_error("missing geometry");
    }

    // Configure geometry meshpart information
    _gmp.info.frameOrderCount = 0;
    _gmp.info.meshpartId = 0;
    _gmp.info.faceType = _GetMeshFaceTypeFromFaceVertexCounts(fvcs);
    _gmp.info.faceCount = fvcs.size();
    _gmp.info.vertexCount = vtxs.GetArraySize();
    _gmp.info.indexCount = vidxs.size();

    // Convert USD data to PMC buffer format
    _gmp.buffers.positions = _ToPmc(vtxs);
    _gmp.buffers.faceDegrees = _ToPmc(fvcs);
    _gmp.buffers.indices = _ToPmc(vidxs);

    // Keep references to data alive
    _keepAlive.emplace_back(vtxs);
    _keepAlive.emplace_back(fvcs);
    _keepAlive.emplace_back(vidxs);

    // Calculate quantization parameters for vertex positions
    auto& usdname = UsdGeomTokens->points;
    _gmp.info << _MakeCoordSys(_gmp, _QparamsFromOptions(_options, usdname, SdfValueRoleNames->Point));

    // Track which attributes have been processed
    _processedAttributes.insert(_ugm.GetFaceVertexCountsAttr().GetName());
    _processedAttributes.insert(_ugm.GetFaceVertexIndicesAttr().GetName());
    _processedAttributes.insert(_ugm.GetPointsAttr().GetName());
}

pmc::AttributeMeshpart&
UsdPmc_EncodeSession::_setupAttr(VtValue vals, VtArray<int> idxs)
{
    auto& amp = _amps.emplace_back();
    amp.info.frameOrderCount = 0,
    amp.info.meshpartId = 0,
    amp.info.attributeId = _amps.size() - 1;
    amp.info.componentsPerVector = _GetExtentFromType(vals);
    amp.info.type = pmc::AttributeType::USER_DEFINED_START;
    // todo: fix api to remove one of these
    amp.info.outputVectorCount = amp.info.vectorCount = vals.GetArraySize(),
    amp.info.outputIndexCount = amp.info.indexCount = idxs.size(),
    amp.info.explicitIndices = !idxs.empty();

    if (!vals.IsEmpty()) {
        amp.buffers.values = _ToPmc(vals);
        _keepAlive.emplace_back(vals);
    }

    if (!idxs.empty()) {
        amp.buffers.indices = _ToPmc(idxs);
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
UsdPmc_EncodeSession::_setupPrimvar(const UsdGeomPrimvar& pv)
{
    // Ignore constant primitives (no point encoding them)
    if (pv.GetInterpolation() == UsdGeomTokens->constant) {
        return;
    }

    auto pvName = pv.GetPrimvarName();
    const auto pvRole = pv.GetTypeName().GetRole();
    const auto vals = _GetAs<VtValue>(pv);
    const auto idxs = _GetAs<VtArray<int>>(pv.GetIndicesAttr());
    if (!vals.IsArrayValued() || vals.IsHolding<VtArray<std::string>>() || vals.IsHolding<VtArray<TfToken>>()) {
        return;
    }

    auto& amp = _setupAttr(vals, idxs);
    amp.info.scope = _GetScopeFromUsd(pv.GetInterpolation());
    amp.info.type = _GuessAttributeType(pvRole, pvName);

    // flat arrays may have elementSize set manually, eg: jointIndices
    // todo: check these are treated as values ...
    if (auto width = pv.GetElementSize(); width > 1) {
        amp.info.componentsPerVector = width;
        amp.info.vectorCount /= width;
        amp.info.outputVectorCount /= width;
        amp.buffers.values.componentsPerVector = width;
        amp.buffers.values.vectorCount /= width;
        amp.buffers.values.stride *= width;
    }

    amp.info << _MakeCoordSys(amp, _QparamsFromOptions(_options, pvName, pvRole));

    // metadata
    amp.info.name = pv.GetName();
    amp.info.jsonCustomAui = JsonAuiForAttr(pv);
    _processedAttributes.insert(amp.info.name);
    if (pv.IsIndexed())
        _processedAttributes.insert(pv.GetIndicesAttr().GetName());
}

/// Build face group information from all subsets.
void
UsdPmc_EncodeSession::_setupGeomSubsets()
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
        auto len = _GetAs<VtValue>(set.GetIndicesAttr()).GetArraySize();
        vals.push_back(len);
        totalLen += len;
    }

    // Catenate all the subset indices into a single array
    VtArray<int> idxs(totalLen);
    auto idxsIt = idxs.begin();
    for (const auto& set : sets) {
        const auto setIdxs = _GetAs<VtArray<int>>(set.GetIndicesAttr());
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
        _processedSubsets.insert(set.GetPrim().GetName());
        os << sep << '\"' << set.GetPrim().GetName() << '\"';
        sep = ',';
    }
    os << "]}}";

    amp.info.jsonCustomAui = std::move(os).str();
}

/// Setup crease data for PMC encoding.
void
UsdPmc_EncodeSession::_setupCreases()
{
    const auto attrIdxs = _ugm.GetCreaseIndicesAttr();
    const auto attrLens = _ugm.GetCreaseLengthsAttr();
    const auto attrVals = _ugm.GetCreaseSharpnessesAttr();

    for (const auto& attr : {attrIdxs, attrLens, attrVals})
        if (!attr.HasAuthoredValue())
            return;

    const auto idxs = _GetAs<VtArray<int>>(attrIdxs);
    const auto lens = _GetAs<VtValue>(attrLens);
    const auto vals = _GetAs<VtValue>(attrVals);

    using pmc::IndicesInterpretation;
    auto& ampIdxs = _setupAttr(lens, idxs);
    const auto ampIdxsId = ampIdxs.info.attributeId;
    ampIdxs.info.type = pmc::AttributeType::CREASE;
    ampIdxs.info.scope = pmc::AttributeScope::VERTEX;
    ampIdxs.info.indicesInterpretation = IndicesInterpretation::SCOPE_INDEXING;
    ampIdxs.info.sparse = true;

    auto& usdname = UsdGeomTokens->creaseSharpnesses;
    auto& ampVals = _setupAttr(vals, {});
    ampVals.info.type = pmc::AttributeType::SHARPNESS;
    ampVals.info.scope = pmc::AttributeScope::DERIVED;
    ampVals.info.derivedScope.scopedAttributeId = ampIdxsId;
    ampVals.info.indicesInterpretation = IndicesInterpretation::VALUE_INDEXING;
    ampVals.info.sparse = true;
    ampVals.info.jsonCustomAui = JsonAuiForAttr(attrVals);
    ampVals.info << _MakeCoordSys(ampVals, _QparamsFromOptions(_options, usdname, SdfValueRoleNames->Vector));

    _processedAttributes.insert(attrIdxs.GetName());
    _processedAttributes.insert(attrLens.GetName());
    _processedAttributes.insert(attrVals.GetName());
}

/// Setup crease data for PMC encoding.
void
UsdPmc_EncodeSession::_setupCorners()
{
    const auto attrIdxs = _ugm.GetCornerIndicesAttr();
    const auto attrVals = _ugm.GetCornerSharpnessesAttr();

    for (const auto& attr : {attrIdxs, attrVals})
        if (!attr.HasAuthoredValue())
            return;

    const auto idxs = _GetAs<VtArray<int>>(attrIdxs);
    const auto vals = _GetAs<VtValue>(attrVals);

    using pmc::IndicesInterpretation;
    auto& ampIdxs = _setupAttr({}, idxs);
    const auto ampIdxsId = ampIdxs.info.attributeId;
    ampIdxs.info.type = pmc::AttributeType::CREASE;
    ampIdxs.info.scope = pmc::AttributeScope::VERTEX;
    ampIdxs.info.indicesInterpretation = IndicesInterpretation::SCOPE_INDEXING;
    ampIdxs.info.sparse = true;
    ampIdxs.info.name = UsdGeomTokens->cornerIndices;

    auto& usdname = UsdGeomTokens->cornerSharpnesses;
    auto& ampVals = _setupAttr(vals, {});
    ampVals.info.type = pmc::AttributeType::SHARPNESS;
    ampVals.info.scope = pmc::AttributeScope::DERIVED;
    ampVals.info.derivedScope.scopedAttributeId = ampIdxsId;
    ampVals.info.indicesInterpretation = IndicesInterpretation::VALUE_INDEXING;
    ampVals.info.sparse = true;
    ampVals.info.jsonCustomAui = JsonAuiForAttr(attrVals);
    ampVals.info << _MakeCoordSys(ampVals, _QparamsFromOptions(_options, usdname, SdfValueRoleNames->Vector));

    _processedAttributes.insert(attrIdxs.GetName());
    _processedAttributes.insert(attrVals.GetName());
}

/// Find all attributes for coding.
void
UsdPmc_EncodeSession::_setupAttrs()
{
    for (auto& pv : UsdGeomPrimvarsAPI(_ugm).GetPrimvarsWithAuthoredValues()) {
        _setupPrimvar(pv);
    }

    // If there are non-primvar normals, code them after the primvar version
    // NB: for rendering, primvars should have priority; we preserve all data
    if (const auto attr = _ugm.GetNormalsAttr(); attr.HasAuthoredValue()) {
        auto& usdname = UsdGeomTokens->normals;
        auto vals = _GetAs<VtValue>(attr);
        auto& amp = _setupAttr(vals, {});
        amp.info.type = pmc::AttributeType::NORMAL;
        amp.info.scope = _GetScopeFromUsd(_ugm.GetNormalsInterpolation());
        amp.info.jsonCustomAui = JsonAuiForAttr(attr);
        amp.info.name = attr.GetName();
        amp.info << _MakeCoordSys(amp, _QparamsFromOptions(_options, usdname, SdfValueRoleNames->Normal));
        _processedAttributes.insert(attr.GetName());
    }

    if (const auto attr = _ugm.GetHoleIndicesAttr(); attr.HasAuthoredValue()) {
        using pmc::IndicesInterpretation;
        auto& amp = _setupAttr({}, _GetAs<VtArray<int>>(attr));
        amp.info.type = pmc::AttributeType::HOLE;
        amp.info.scope = pmc::AttributeScope::FACE;
        amp.info.indicesInterpretation = IndicesInterpretation::SCOPE_INDEXING;
        amp.info.sparse = true;
        amp.info.jsonCustomAui = JsonAuiForAttr(attr);
        amp.info.name = attr.GetName();
        _processedAttributes.insert(attr.GetName());
    }

    if (const auto attr = _ugm.GetVelocitiesAttr(); attr.HasAuthoredValue()) {
        auto& usdname = UsdGeomTokens->velocities;
        auto vals = _GetAs<VtValue>(attr);
        auto& amp = _setupAttr(vals, {});
        amp.info.type = pmc::AttributeType::USER_DEFINED_START;
        amp.info.scope = pmc::AttributeScope::VERTEX;
        amp.info.jsonCustomAui = JsonAuiForAttr(attr);
        amp.info.name = attr.GetName();
        amp.info << _MakeCoordSys(amp, _QparamsFromOptions(_options, usdname, SdfValueRoleNames->Vector));
        _processedAttributes.insert(attr.GetName());
    }

    if (const auto attr = _ugm.GetAccelerationsAttr(); attr.HasAuthoredValue()) {
        auto& usdname = UsdGeomTokens->accelerations;
        auto vals = _GetAs<VtValue>(attr);
        auto& amp = _setupAttr(vals, {});
        amp.info.type = pmc::AttributeType::USER_DEFINED_START;
        amp.info.scope = pmc::AttributeScope::VERTEX;
        amp.info.jsonCustomAui = JsonAuiForAttr(attr);
        amp.info.name = attr.GetName();
        amp.info << _MakeCoordSys(amp, _QparamsFromOptions(_options, usdname, SdfValueRoleNames->Vector));
        _processedAttributes.insert(attr.GetName());
    }

    _setupGeomSubsets();
    _setupCreases();
    _setupCorners();
}

/// Configure the PMC encoder with geometry and attribute information.
void
UsdPmc_EncodeSession::_configurePmc()
{
    constexpr _Expect throwOnError {pmc::Error::OK};

    // Configure geometry meshpart
    throwOnError = _enc.configure(_gmp.info);

    // Configure all attribute meshparts
    for (const auto& amp : _amps) {
        throwOnError = _enc.configure(amp.info);
    }
}

/// Perform the actual PMC encoding operation.
std::vector<uint8_t>
UsdPmc_EncodeSession::_encode()
{
    // Configure geometry encoding parameters
    pmc::GeometryEncodingParameters geomOpts;

    // Configure attribute encoding parameters
    pmc::AttributeEncodingParameters attrOpts;

    // Estimate the total size needed for the output buffer
    size_t estSize = 0;
    estSize += _enc.estimateMaxEncodedSize(_gmp, geomOpts);
    for (const auto& amp : _amps) {
        attrOpts.indicesCodingStrategy = _GetIndicesStrategyForAttr(amp.info);
        attrOpts.traversalStrategy = _GetTraversalStrategyForAttr(amp.info);
        attrOpts.predictionStrategy = _GetPredictionStrategyForAttr(amp.info);
        estSize += _enc.estimateMaxEncodedSize(amp, attrOpts);
    }

    // Allocate output buffer and create PMC byte buffer
    std::vector<uint8_t> dst(estSize);
    pmc::ByteBuffer dstBuf {dst.size(), 0, dst.data()};

    // Temporary buffer for conversions
    std::vector<int32_t> tmp;

    // Encode geometry meshpart
    if (1) {
        _Quantizer q(float(_gmp.info.coordSys), _gmp.info.coordSysOrigin, 3);
        _gmp.buffers.positions = _ConvertBuffer(_gmp.buffers.positions, tmp, q);
    }

    // cross-reference attributes for encoder connectivity decisions
    for (auto& amp : _amps)
        _gmp.attMeshparts.push_back(&amp);

    constexpr _Expect throwOnError {pmc::Error::OK};
    throwOnError = _enc.encode(_gmp, dstBuf, geomOpts);

    // Encode all attribute meshparts
    for (auto& amp : _amps) {
        using PS = pmc::PredictionStrategy;
        attrOpts.indicesCodingStrategy = _GetIndicesStrategyForAttr(amp.info);
        attrOpts.traversalStrategy = _GetTraversalStrategyForAttr(amp.info);
        attrOpts.predictionStrategy = _GetPredictionStrategyForAttr(amp.info);

        if (amp.info.coordSys) {
            auto& cs = *amp.info.coordSys;
            if (attrOpts.predictionStrategy == PS::UNITARY_OCTAHEDRAL_NORMAL_VECTOR) {
                _QuantizerOctahedral q(float(cs.scale));
                amp.buffers.values = _ConvertBuffer(amp.buffers.values, tmp, q);
                amp.info.coordSysProjection = pmc::CoordSysProjection::OCTAHEDRAL;
            } else {
                _Quantizer q(float(cs.scale), cs.origin.data(), cs.origin.size());
                amp.buffers.values = _ConvertBuffer(amp.buffers.values, tmp, q);
            }
        }

        throwOnError = _enc.encode(amp, dstBuf, attrOpts);
    }

    // Resize to actual encoded size and return
    dst.resize(dstBuf.size);
    return dst;
}

std::vector<uint8_t>
UsdPmc_EncodeSession::encode()
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
