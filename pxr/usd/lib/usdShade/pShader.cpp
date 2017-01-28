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
#include "pxr/pxr.h"
#include "pxr/usd/usdShade/pShader.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdShadePShader,
        TfType::Bases< UsdTyped > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("PShader")
    // to find TfType<UsdShadePShader>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdShadePShader>("PShader");
}

/* virtual */
UsdShadePShader::~UsdShadePShader()
{
}

/* static */
UsdShadePShader
UsdShadePShader::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdShadePShader();
    }
    return UsdShadePShader(stage->GetPrimAtPath(path));
}

/* static */
UsdShadePShader
UsdShadePShader::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("PShader");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdShadePShader();
    }
    return UsdShadePShader(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* static */
const TfType &
UsdShadePShader::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdShadePShader>();
    return tfType;
}

/* static */
bool 
UsdShadePShader::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdShadePShader::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdShadePShader::GetSloPathAttr() const
{
    return GetPrim().GetAttribute(UsdShadeTokens->sloPath);
}

UsdAttribute
UsdShadePShader::CreateSloPathAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdShadeTokens->sloPath,
                       SdfValueTypeNames->Asset,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdShadePShader::GetShaderProtocolAttr() const
{
    return GetPrim().GetAttribute(UsdShadeTokens->shaderProtocol);
}

UsdAttribute
UsdShadePShader::CreateShaderProtocolAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdShadeTokens->shaderProtocol,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdShadePShader::GetShaderTypeAttr() const
{
    return GetPrim().GetAttribute(UsdShadeTokens->shaderType);
}

UsdAttribute
UsdShadePShader::CreateShaderTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdShadeTokens->shaderType,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdShadePShader::GetDisplayColorAttr() const
{
    return GetPrim().GetAttribute(UsdShadeTokens->displayColor);
}

UsdAttribute
UsdShadePShader::CreateDisplayColorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdShadeTokens->displayColor,
                       SdfValueTypeNames->Color3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdShadePShader::GetDisplayOpacityAttr() const
{
    return GetPrim().GetAttribute(UsdShadeTokens->displayOpacity);
}

UsdAttribute
UsdShadePShader::CreateDisplayOpacityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdShadeTokens->displayOpacity,
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
UsdShadePShader::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdShadeTokens->sloPath,
        UsdShadeTokens->shaderProtocol,
        UsdShadeTokens->shaderType,
        UsdShadeTokens->displayColor,
        UsdShadeTokens->displayOpacity,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdTyped::GetSchemaAttributeNames(true),
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
// Just remember to wrap code in the pxr namespace macros:
// PXR_NAMESPACE_OPEN_SCOPE, PXR_NAMESPACE_CLOSE_SCOPE.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--
