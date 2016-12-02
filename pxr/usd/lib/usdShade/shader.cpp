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
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdShadeShader,
        TfType::Bases< UsdTyped > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("Shader")
    // to find TfType<UsdShadeShader>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdShadeShader>("Shader");
}

/* virtual */
UsdShadeShader::~UsdShadeShader()
{
}

/* static */
UsdShadeShader
UsdShadeShader::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (not stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdShadeShader();
    }
    return UsdShadeShader(stage->GetPrimAtPath(path));
}

/* static */
UsdShadeShader
UsdShadeShader::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("Shader");
    if (not stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdShadeShader();
    }
    return UsdShadeShader(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* static */
const TfType &
UsdShadeShader::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdShadeShader>();
    return tfType;
}

/* static */
bool 
UsdShadeShader::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdShadeShader::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdShadeShader::GetIdAttr() const
{
    return GetPrim().GetAttribute(UsdShadeTokens->infoId);
}

UsdAttribute
UsdShadeShader::CreateIdAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdShadeTokens->infoId,
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
UsdShadeShader::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdShadeTokens->infoId,
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

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

UsdShadeParameter
UsdShadeShader::CreateParameter(const TfToken& name,
    const SdfValueTypeName& typeName)
{
    return UsdShadeParameter(
            GetPrim(), 
            name, 
            typeName);
}

UsdShadeParameter
UsdShadeShader::GetParameter(const TfToken &name) const
{
    return UsdShadeParameter(GetPrim().GetAttribute(name));
}

std::vector<UsdShadeParameter>
UsdShadeShader::GetParameters() const
{
    std::vector<UsdShadeParameter> ret;

    std::vector<UsdAttribute> attrs = GetPrim().GetAttributes();
    TF_FOR_ALL(attrIter, attrs) { 
        const UsdAttribute& attr = *attrIter;
        if (attr.GetNamespace().IsEmpty()) {
            ret.push_back(UsdShadeParameter(attr));
        }
    }
    return ret;
}

