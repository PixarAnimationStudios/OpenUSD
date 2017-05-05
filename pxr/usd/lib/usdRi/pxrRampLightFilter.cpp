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
#include "pxr/usd/usdRi/pxrRampLightFilter.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdRiPxrRampLightFilter,
        TfType::Bases< UsdLuxLightFilter > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("PxrRampLightFilter")
    // to find TfType<UsdRiPxrRampLightFilter>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdRiPxrRampLightFilter>("PxrRampLightFilter");
}

/* virtual */
UsdRiPxrRampLightFilter::~UsdRiPxrRampLightFilter()
{
}

/* static */
UsdRiPxrRampLightFilter
UsdRiPxrRampLightFilter::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiPxrRampLightFilter();
    }
    return UsdRiPxrRampLightFilter(stage->GetPrimAtPath(path));
}

/* static */
UsdRiPxrRampLightFilter
UsdRiPxrRampLightFilter::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("PxrRampLightFilter");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiPxrRampLightFilter();
    }
    return UsdRiPxrRampLightFilter(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* static */
const TfType &
UsdRiPxrRampLightFilter::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdRiPxrRampLightFilter>();
    return tfType;
}

/* static */
bool 
UsdRiPxrRampLightFilter::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdRiPxrRampLightFilter::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdRiPxrRampLightFilter::GetRampModeAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->rampMode);
}

UsdAttribute
UsdRiPxrRampLightFilter::CreateRampModeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->rampMode,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRampLightFilter::GetFalloffRampBeginDistanceAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->falloffRampBeginDistance);
}

UsdAttribute
UsdRiPxrRampLightFilter::CreateFalloffRampBeginDistanceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->falloffRampBeginDistance,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiPxrRampLightFilter::GetFalloffRampEndDistanceAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->falloffRampEndDistance);
}

UsdAttribute
UsdRiPxrRampLightFilter::CreateFalloffRampEndDistanceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->falloffRampEndDistance,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
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
UsdRiPxrRampLightFilter::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdRiTokens->rampMode,
        UsdRiTokens->falloffRampBeginDistance,
        UsdRiTokens->falloffRampEndDistance,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdLuxLightFilter::GetSchemaAttributeNames(true),
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
