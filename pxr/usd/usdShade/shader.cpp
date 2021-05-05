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

PXR_NAMESPACE_OPEN_SCOPE

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
    if (!stage) {
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
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdShadeShader();
    }
    return UsdShadeShader(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdShadeShader::_GetSchemaKind() const {
    return UsdShadeShader::schemaKind;
}

/* virtual */
UsdSchemaKind UsdShadeShader::_GetSchemaType() const {
    return UsdShadeShader::schemaType;
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

/*static*/
const TfTokenVector&
UsdShadeShader::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdTyped::GetSchemaAttributeNames(true);

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

#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/connectableAPIBehavior.h"
#include "pxr/usd/usdShade/nodeDefAPI.h"
#include "pxr/usd/usdShade/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(UsdShadeConnectableAPI)
{
    // UsdShadeShader prims are connectable, with default behavior rules.
    UsdShadeRegisterConnectableAPIBehavior<UsdShadeShader>();
}

UsdShadeShader::UsdShadeShader(const UsdShadeConnectableAPI &connectable)
    : UsdShadeShader(connectable.GetPrim())
{
}

UsdShadeConnectableAPI 
UsdShadeShader::ConnectableAPI() const
{
    return UsdShadeConnectableAPI(GetPrim());
}

UsdShadeOutput
UsdShadeShader::CreateOutput(const TfToken& name,
                             const SdfValueTypeName& typeName)
{
    return UsdShadeConnectableAPI(GetPrim()).CreateOutput(name, typeName);
}

UsdShadeOutput
UsdShadeShader::GetOutput(const TfToken &name) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetOutput(name);
}

std::vector<UsdShadeOutput>
UsdShadeShader::GetOutputs(bool onlyAuthored) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetOutputs(onlyAuthored);
}

UsdShadeInput
UsdShadeShader::CreateInput(const TfToken& name,
                            const SdfValueTypeName& typeName)
{
    return UsdShadeConnectableAPI(GetPrim()).CreateInput(name, typeName);
}

UsdShadeInput
UsdShadeShader::GetInput(const TfToken &name) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetInput(name);
}

std::vector<UsdShadeInput>
UsdShadeShader::GetInputs(bool onlyAuthored) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetInputs(onlyAuthored);
}

UsdAttribute
UsdShadeShader::GetImplementationSourceAttr() const
{
    return UsdShadeNodeDefAPI(GetPrim()).GetImplementationSourceAttr();
}

UsdAttribute
UsdShadeShader::CreateImplementationSourceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdShadeNodeDefAPI(GetPrim()).CreateImplementationSourceAttr(defaultValue, writeSparsely);
}

UsdAttribute
UsdShadeShader::GetIdAttr() const
{
    return UsdShadeNodeDefAPI(GetPrim()).GetIdAttr();
}

UsdAttribute
UsdShadeShader::CreateIdAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdShadeNodeDefAPI(GetPrim()).CreateIdAttr(defaultValue, writeSparsely);
}

TfToken 
UsdShadeShader::GetImplementationSource() const
{
    return UsdShadeNodeDefAPI(GetPrim()).GetImplementationSource();
}
    
bool 
UsdShadeShader::SetShaderId(const TfToken &id) const
{
    return UsdShadeNodeDefAPI(GetPrim()).SetShaderId(id);
}

bool 
UsdShadeShader::GetShaderId(TfToken *id) const
{
    return UsdShadeNodeDefAPI(GetPrim()).GetShaderId(id);
}

bool 
UsdShadeShader::SetSourceAsset(
    const SdfAssetPath &sourceAsset,
    const TfToken &sourceType) const
{
    return UsdShadeNodeDefAPI(GetPrim()).SetSourceAsset(sourceAsset, sourceType);
}

bool 
UsdShadeShader::GetSourceAsset(
    SdfAssetPath *sourceAsset,
    const TfToken &sourceType) const
{
    return UsdShadeNodeDefAPI(GetPrim()).GetSourceAsset(sourceAsset, sourceType);
}

bool
UsdShadeShader::SetSourceAssetSubIdentifier(
    const TfToken &subIdentifier,
    const TfToken &sourceType) const
{
    return UsdShadeNodeDefAPI(GetPrim())
        .SetSourceAssetSubIdentifier(subIdentifier, sourceType);
}

bool
UsdShadeShader::GetSourceAssetSubIdentifier(
    TfToken *subIdentifier,
    const TfToken &sourceType) const
{
    return UsdShadeNodeDefAPI(GetPrim())
        .GetSourceAssetSubIdentifier(subIdentifier, sourceType);
}

bool 
UsdShadeShader::SetSourceCode(
    const std::string &sourceCode, 
    const TfToken &sourceType) const
{
    return UsdShadeNodeDefAPI(GetPrim())
        .SetSourceCode(sourceCode, sourceType);
}

bool 
UsdShadeShader::GetSourceCode(
    std::string *sourceCode,
    const TfToken &sourceType) const
{
    return UsdShadeNodeDefAPI(GetPrim())
        .GetSourceCode(sourceCode, sourceType);
}

SdrShaderNodeConstPtr 
UsdShadeShader::GetShaderNodeForSourceType(const TfToken &sourceType) const
{
    // XXX(Performance): This is in the critical path for rendering and may be invoked many times.
    // We may find that the overhead of creating a new schema object for each call to be
    // significant, in which case we may want to revisit this.
    return UsdShadeNodeDefAPI(GetPrim())
        .GetShaderNodeForSourceType(sourceType);
}

NdrTokenMap
UsdShadeShader::GetSdrMetadata() const
{
    NdrTokenMap result;

    VtDictionary sdrMetadata;
    if (GetPrim().GetMetadata(UsdShadeTokens->sdrMetadata, &sdrMetadata)){
        for (const auto &it : sdrMetadata) {
            result[TfToken(it.first)] = TfStringify(it.second);
        }
    }

    return result;
}

std::string 
UsdShadeShader::GetSdrMetadataByKey(const TfToken &key) const
{
    VtValue val;
    GetPrim().GetMetadataByDictKey(UsdShadeTokens->sdrMetadata, key, &val);
    return TfStringify(val);
}
    
void 
UsdShadeShader::SetSdrMetadata(const NdrTokenMap &sdrMetadata) const
{
    for (auto &i: sdrMetadata) {
        SetSdrMetadataByKey(i.first, i.second);
    }
}

void 
UsdShadeShader::SetSdrMetadataByKey(
    const TfToken &key, 
    const std::string &value) const
{
    GetPrim().SetMetadataByDictKey(UsdShadeTokens->sdrMetadata, key, value);
}

bool 
UsdShadeShader::HasSdrMetadata() const
{
    return GetPrim().HasMetadata(UsdShadeTokens->sdrMetadata);
}

bool 
UsdShadeShader::HasSdrMetadataByKey(const TfToken &key) const
{
    return GetPrim().HasMetadataDictKey(UsdShadeTokens->sdrMetadata, key);
}

void 
UsdShadeShader::ClearSdrMetadata() const
{
    GetPrim().ClearMetadata(UsdShadeTokens->sdrMetadata);
}

void
UsdShadeShader::ClearSdrMetadataByKey(const TfToken &key) const
{
    GetPrim().ClearMetadataByDictKey(UsdShadeTokens->sdrMetadata, key);
}

PXR_NAMESPACE_CLOSE_SCOPE
