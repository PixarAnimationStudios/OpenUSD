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
#include "pxr/usd/usdLux/shapingAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxShapingAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (ShapingAPI)
);

/* virtual */
UsdLuxShapingAPI::~UsdLuxShapingAPI()
{
}

/* static */
UsdLuxShapingAPI
UsdLuxShapingAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxShapingAPI();
    }
    return UsdLuxShapingAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaType UsdLuxShapingAPI::_GetSchemaType() const {
    return UsdLuxShapingAPI::schemaType;
}

/* static */
UsdLuxShapingAPI
UsdLuxShapingAPI::Apply(const UsdPrim &prim)
{
    return UsdAPISchemaBase::_ApplyAPISchema<UsdLuxShapingAPI>(
            prim, _schemaTokens->ShapingAPI);
}

/* static */
const TfType &
UsdLuxShapingAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxShapingAPI>();
    return tfType;
}

/* static */
bool 
UsdLuxShapingAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxShapingAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLuxShapingAPI::GetShapingFocusAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->shapingFocus);
}

UsdAttribute
UsdLuxShapingAPI::CreateShapingFocusAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->shapingFocus,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxShapingAPI::GetShapingFocusTintAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->shapingFocusTint);
}

UsdAttribute
UsdLuxShapingAPI::CreateShapingFocusTintAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->shapingFocusTint,
                       SdfValueTypeNames->Color3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxShapingAPI::GetShapingConeAngleAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->shapingConeAngle);
}

UsdAttribute
UsdLuxShapingAPI::CreateShapingConeAngleAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->shapingConeAngle,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxShapingAPI::GetShapingConeSoftnessAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->shapingConeSoftness);
}

UsdAttribute
UsdLuxShapingAPI::CreateShapingConeSoftnessAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->shapingConeSoftness,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxShapingAPI::GetShapingIesFileAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->shapingIesFile);
}

UsdAttribute
UsdLuxShapingAPI::CreateShapingIesFileAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->shapingIesFile,
                       SdfValueTypeNames->Asset,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxShapingAPI::GetShapingIesAngleScaleAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->shapingIesAngleScale);
}

UsdAttribute
UsdLuxShapingAPI::CreateShapingIesAngleScaleAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->shapingIesAngleScale,
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
UsdLuxShapingAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLuxTokens->shapingFocus,
        UsdLuxTokens->shapingFocusTint,
        UsdLuxTokens->shapingConeAngle,
        UsdLuxTokens->shapingConeSoftness,
        UsdLuxTokens->shapingIesFile,
        UsdLuxTokens->shapingIesAngleScale,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdAPISchemaBase::GetSchemaAttributeNames(true),
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
