//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/basisCurves.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomBasisCurves,
        TfType::Bases< UsdGeomCurves > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("BasisCurves")
    // to find TfType<UsdGeomBasisCurves>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdGeomBasisCurves>("BasisCurves");
}

/* virtual */
UsdGeomBasisCurves::~UsdGeomBasisCurves()
{
}

/* static */
UsdGeomBasisCurves
UsdGeomBasisCurves::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomBasisCurves();
    }
    return UsdGeomBasisCurves(stage->GetPrimAtPath(path));
}

/* static */
UsdGeomBasisCurves
UsdGeomBasisCurves::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("BasisCurves");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomBasisCurves();
    }
    return UsdGeomBasisCurves(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdGeomBasisCurves::_GetSchemaKind() const
{
    return UsdGeomBasisCurves::schemaKind;
}

/* static */
const TfType &
UsdGeomBasisCurves::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomBasisCurves>();
    return tfType;
}

/* static */
bool 
UsdGeomBasisCurves::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomBasisCurves::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomBasisCurves::GetTypeAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->type);
}

UsdAttribute
UsdGeomBasisCurves::CreateTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->type,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomBasisCurves::GetBasisAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->basis);
}

UsdAttribute
UsdGeomBasisCurves::CreateBasisAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->basis,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomBasisCurves::GetWrapAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->wrap);
}

UsdAttribute
UsdGeomBasisCurves::CreateWrapAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->wrap,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

/*static*/
const TfTokenVector&
UsdGeomBasisCurves::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->type,
        UsdGeomTokens->basis,
        UsdGeomTokens->wrap,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomCurves::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

PXR_NAMESPACE_OPEN_SCOPE

static
size_t
_GetVStepForBasis(const TfToken &basis)
{
    if (basis == UsdGeomTokens->bezier) {
        return 3;
    } else if (basis == UsdGeomTokens->bspline) {
        return 1;
    } else if (basis == UsdGeomTokens->catmullRom) {
        return 1;
    }
    // Calling code should have already errored from unknown basis.
    return 0;
}

static
size_t
_ComputeVaryingDataSize(const UsdGeomBasisCurves &basisCurves, 
        const VtIntArray& curveVertexCounts,
        const UsdTimeCode &frame)
{
    TfToken curvesType, wrap;
    basisCurves.GetTypeAttr().Get(&curvesType, frame);
    basisCurves.GetWrapAttr().Get(&wrap, frame);

    VtIntArray curveVertexCts;
    basisCurves.GetCurveVertexCountsAttr().Get(&curveVertexCts, frame);

    size_t result = 0;
    // Code is deliberately verbose to clarify each case.
    if (curvesType == UsdGeomTokens->linear) {
        if (wrap == UsdGeomTokens->nonperiodic ||
            wrap == UsdGeomTokens->pinned) {

            for (const int &count : curveVertexCts) {
                result += count;
            }

        } else { // periodic
            for (const int &count : curveVertexCts) {
                result += count + 1;
            }
        }
        return result;
    } 
    
    TfToken basis;
    basisCurves.GetBasisAttr().Get(&basis, frame);
    size_t vstep = _GetVStepForBasis(basis);

    if (curvesType == UsdGeomTokens->cubic) {
        // While the minimum vertex count is 2 for pinned cubic curves and 4
        // otherwise, we treat pinned and non-periodic cubic curves identically
        // below to reflect the authored intent in that there shouldn't be any
        // difference in the primvar data authored (i.e., data for the
        // additional segment(s) for pinned curves doesn't need to be authored).
        // 
        // Also note that we treat curves with fewer vertices than the
        // minimum as a single segment, thus requiring 2 varying values.
        //
        if (wrap == UsdGeomTokens->nonperiodic ||
            wrap == UsdGeomTokens->pinned) {

            for (const int &count : curveVertexCts) {
                result +=  std::max<size_t>((count - 4)/vstep, 0) + 2;
            }

        } else { // periodic
            for (const int &count : curveVertexCts) {
                result += count/vstep;
            }
        }
    }
       
    return result;
}

static
size_t
_ComputeVertexDataSize(
        const VtIntArray& curveVertexCounts)
{
    size_t result = 0;
    for (const int &count : curveVertexCounts) {
        result += count;
    }
       
    return result;
}


#define RETURN_OR_APPEND_INFO(interpToken, val, expected) \
    if (val == expected) { return interpToken; } \
    else if (info) { info->push_back(std::make_pair(interpToken, expected)); }

TfToken
UsdGeomBasisCurves::ComputeInterpolationForSize(
        size_t n,
        const UsdTimeCode& timeCode,
        UsdGeomBasisCurves::ComputeInterpolationInfo* info) const
{
    if (info) {
        info->clear();
    }

    RETURN_OR_APPEND_INFO(UsdGeomTokens->constant, n, 1);

    VtIntArray curveVertexCounts;
    GetCurveVertexCountsAttr().Get(&curveVertexCounts, timeCode);

    size_t numUniform = curveVertexCounts.size();
    RETURN_OR_APPEND_INFO(UsdGeomTokens->uniform, n, numUniform);

    size_t numVarying = _ComputeVaryingDataSize(
        *this, curveVertexCounts, timeCode);
    RETURN_OR_APPEND_INFO(UsdGeomTokens->varying, n, numVarying);

    size_t numVertex = _ComputeVertexDataSize(curveVertexCounts);
    RETURN_OR_APPEND_INFO(UsdGeomTokens->vertex, n, numVertex);

    return TfToken();
}

size_t
UsdGeomBasisCurves::ComputeUniformDataSize(
        const UsdTimeCode& timeCode) const
{
    VtIntArray curveVertexCounts;
    GetCurveVertexCountsAttr().Get(&curveVertexCounts, timeCode);
    return curveVertexCounts.size();
}

size_t
UsdGeomBasisCurves::ComputeVaryingDataSize(
        const UsdTimeCode& timeCode) const
{
    VtIntArray curveVertexCounts;
    GetCurveVertexCountsAttr().Get(&curveVertexCounts, timeCode);
    return _ComputeVaryingDataSize(*this, curveVertexCounts, timeCode);
}

size_t
UsdGeomBasisCurves::ComputeVertexDataSize(
        const UsdTimeCode& timeCode) const
{
    VtIntArray curveVertexCounts;
    GetCurveVertexCountsAttr().Get(&curveVertexCounts, timeCode);
    return _ComputeVertexDataSize(curveVertexCounts);
}

VtIntArray
UsdGeomBasisCurves::ComputeSegmentCounts(
        const UsdTimeCode& timeCode) const
{
    VtIntArray curveVertexCounts;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
    if (!GetCurveVertexCountsAttr().Get(&curveVertexCounts, timeCode)) {
        TF_WARN("CurveVertexCounts could not be read from prim, " 
            "cannot compute segment counts.");
        return VtIntArray();
    }

    TfToken type;
    if (!GetTypeAttr().Get(&type, timeCode)) {
        TF_WARN("Curve type could not be read from prim, " 
            "cannot compute segment counts.");
        return VtIntArray();
    }

    TfToken wrap;
    if (!GetWrapAttr().Get(&wrap, timeCode)) {
        TF_WARN("Curve wrap could not be read from prim, " 
            "cannot compute segment counts.");
        return VtIntArray();
    }

    TfToken basis;
    if (!GetBasisAttr().Get(&basis, timeCode)) {
        TF_WARN("Curve basis could not be read from prim, " 
            "cannot compute segment counts.");
        return VtIntArray();
    }

    VtIntArray segmentCounts(curveVertexCounts.size());
    bool isValid = false;

    if (type == UsdGeomTokens->linear) {
        if (wrap == UsdGeomTokens->periodic) {
            // Linear and periodic
            segmentCounts = curveVertexCounts;
            isValid = true;
        } else if (wrap == UsdGeomTokens->nonperiodic || 
                wrap == UsdGeomTokens->pinned) {
            // Linear and nonperiodic/pinned
            std::transform(curveVertexCounts.cbegin(), curveVertexCounts.cend(), 
                segmentCounts.begin(), [](int n) { return n - 1; });
            isValid = true;
        }
    } else if (type == UsdGeomTokens->cubic) {
        if (basis == UsdGeomTokens->bezier) {
            constexpr int vstep = 3;
            if (wrap == UsdGeomTokens->periodic) {
                // Cubic, bezier, periodic
                std::transform(curveVertexCounts.cbegin(), curveVertexCounts.cend(), 
                    segmentCounts.begin(), [vstep](int n) { return n / vstep; });
                isValid = true;
            } else if (wrap == UsdGeomTokens->nonperiodic || 
                    wrap == UsdGeomTokens->pinned) {
                // Cubic, bezier, nonperiodic/pinned
                std::transform(curveVertexCounts.cbegin(), curveVertexCounts.cend(), 
                    segmentCounts.begin(), [vstep](int n) { return (n - 4) / vstep + 1; });
                isValid = true;
            }
        } else if (basis == UsdGeomTokens->bspline ||
                basis == UsdGeomTokens->catmullRom) {
            if (wrap == UsdGeomTokens->periodic) {
                // Cubic, bspline/catmullRom, periodic
                segmentCounts = curveVertexCounts;
                isValid = true;
            } else if (wrap == UsdGeomTokens->nonperiodic) {
                // Cubic, bspline/catmullRom, nonperiodic
                std::transform(curveVertexCounts.cbegin(), curveVertexCounts.cend(), 
                    segmentCounts.begin(), [](int n) { return n - 3; });
                isValid = true;
            } else if (wrap == UsdGeomTokens->pinned) {
                // Cubic, bspline/catmullRom, pinned
                std::transform(curveVertexCounts.cbegin(), curveVertexCounts.cend(), 
                    segmentCounts.begin(), [](int n) { return n - 1; });
                isValid = true;
            }
        }
    }

    if (!isValid) {
        TF_WARN("Invalid type, wrap, or basis.");
        return VtIntArray();
    }

    return segmentCounts;
}

PXR_NAMESPACE_CLOSE_SCOPE
