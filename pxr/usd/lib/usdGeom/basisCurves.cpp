//
// Copyright 2016 Pixar
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
    // http://renderman.pixar.com/resources/current/rps/geometricPrimitives.html#ribasis
    // BasisCurves do not yet support hermite or power bases.
    if (basis == UsdGeomTokens->bezier) {
        return 3;
    } else if (basis == UsdGeomTokens->bspline) {
        return 1;
    } else if (basis == UsdGeomTokens->catmullRom) {
        return 1;
    } else if (basis == UsdGeomTokens->hermite) {
        return 2;
    } else if (basis == UsdGeomTokens->power) {
        return 4;
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
    bool isNonPeriodic = wrap != UsdGeomTokens->periodic;

    VtIntArray curveVertexCts;
    basisCurves.GetCurveVertexCountsAttr().Get(&curveVertexCts, frame);

    // http://renderman.pixar.com/resources/current/rps/appnote.19.html
    size_t result = 0;
    // Code is deliberately verbose to clarify each case.
    if (curvesType == UsdGeomTokens->linear) {
        if (isNonPeriodic) {
            for (const auto& curveVertexCt : curveVertexCts) {
                result += curveVertexCt;
            }
        } else {
            for (const auto& curveVertexCt : curveVertexCts) {
                result += curveVertexCt + 1;
            }
        }
        return result;
    } 
    
    TfToken basis;
    basisCurves.GetBasisAttr().Get(&basis, frame);
    size_t vstep = _GetVStepForBasis(basis);

    if (curvesType == UsdGeomTokens->cubic) {
        if (isNonPeriodic) {
            for (const auto& curveVertexCt : curveVertexCts) {
                result += (curveVertexCt - 4)/vstep + 2;
            }
        } else {
            for (const auto& curveVertexCt : curveVertexCts) {
                result += curveVertexCt/vstep;
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
    // http://renderman.pixar.com/resources/current/rps/appnote.19.html
    size_t result = 0;
    for (const auto& curveVertexCount : curveVertexCounts) {
        result += curveVertexCount;
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

    size_t numVarying = _ComputeVaryingDataSize(*this, curveVertexCounts, timeCode);
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

PXR_NAMESPACE_CLOSE_SCOPE
