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
#include "pxr/usd/usdLux/cylinderLight.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxCylinderLight,
        TfType::Bases< UsdLuxLight > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("CylinderLight")
    // to find TfType<UsdLuxCylinderLight>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdLuxCylinderLight>("CylinderLight");
}

/* virtual */
UsdLuxCylinderLight::~UsdLuxCylinderLight()
{
}

/* static */
UsdLuxCylinderLight
UsdLuxCylinderLight::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxCylinderLight();
    }
    return UsdLuxCylinderLight(stage->GetPrimAtPath(path));
}

/* static */
UsdLuxCylinderLight
UsdLuxCylinderLight::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("CylinderLight");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxCylinderLight();
    }
    return UsdLuxCylinderLight(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaType UsdLuxCylinderLight::_GetSchemaType() const {
    return UsdLuxCylinderLight::schemaType;
}

/* static */
const TfType &
UsdLuxCylinderLight::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxCylinderLight>();
    return tfType;
}

/* static */
bool 
UsdLuxCylinderLight::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxCylinderLight::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLuxCylinderLight::GetLengthAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->length);
}

UsdAttribute
UsdLuxCylinderLight::CreateLengthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->length,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxCylinderLight::GetRadiusAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->radius);
}

UsdAttribute
UsdLuxCylinderLight::CreateRadiusAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->radius,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxCylinderLight::GetTreatAsLineAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->treatAsLine);
}

UsdAttribute
UsdLuxCylinderLight::CreateTreatAsLineAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->treatAsLine,
                       SdfValueTypeNames->Bool,
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
UsdLuxCylinderLight::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLuxTokens->length,
        UsdLuxTokens->radius,
        UsdLuxTokens->treatAsLine,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdLuxLight::GetSchemaAttributeNames(true),
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
