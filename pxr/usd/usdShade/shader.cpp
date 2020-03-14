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
UsdSchemaType UsdShadeShader::_GetSchemaType() const {
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

UsdAttribute
UsdShadeShader::GetImplementationSourceAttr() const
{
    return GetPrim().GetAttribute(UsdShadeTokens->infoImplementationSource);
}

UsdAttribute
UsdShadeShader::CreateImplementationSourceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdShadeTokens->infoImplementationSource,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
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
        UsdShadeTokens->infoImplementationSource,
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

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/usdShade/connectableAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (info)
    ((infoSourceAsset, "info:sourceAsset"))
    ((infoSubIdentifier, "info:sourceAsset:subIdentifier"))
    ((infoSourceCode, "info:sourceCode"))
);

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
UsdShadeShader::GetOutputs() const
{
    return UsdShadeConnectableAPI(GetPrim()).GetOutputs();
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
UsdShadeShader::GetInputs() const
{
    return UsdShadeConnectableAPI(GetPrim()).GetInputs();
}

TfToken 
UsdShadeShader::GetImplementationSource() const
{
    TfToken implSource;
    GetImplementationSourceAttr().Get(&implSource);

    if (implSource == UsdShadeTokens->id ||
        implSource == UsdShadeTokens->sourceAsset ||
        implSource == UsdShadeTokens->sourceCode) {
        return implSource;
    } else {
        TF_WARN("Found invalid info:implementationSource value '%s' on shader "
                "at path <%s>. Falling back to 'id'.", implSource.GetText(),
                GetPath().GetText());
        return UsdShadeTokens->id;
    }
}
    
bool 
UsdShadeShader::SetShaderId(const TfToken &id) const
{
    return CreateImplementationSourceAttr(VtValue(UsdShadeTokens->id), 
                                          /*writeSparsely*/ true) &&
           GetIdAttr().Set(id);
}

bool 
UsdShadeShader::GetShaderId(TfToken *id) const
{
    TfToken implSource = GetImplementationSource();
    if (implSource == UsdShadeTokens->id) {
        return GetIdAttr().Get(id);
    }
    return false;
}

static 
TfToken
_GetSourceAssetAttrName(const TfToken &sourceType) 
{
    if (sourceType == UsdShadeTokens->universalSourceType) {
        return _tokens->infoSourceAsset;
    }
    return TfToken(SdfPath::JoinIdentifier(TfTokenVector{
                                    _tokens->info, 
                                    sourceType,
                                    UsdShadeTokens->sourceAsset}));
}

bool 
UsdShadeShader::SetSourceAsset(
    const SdfAssetPath &sourceAsset,
    const TfToken &sourceType) const
{
    TfToken sourceAssetAttrName = _GetSourceAssetAttrName(sourceType);
    return CreateImplementationSourceAttr(VtValue(UsdShadeTokens->sourceAsset)) 
        && UsdSchemaBase::_CreateAttr(sourceAssetAttrName,
                                      SdfValueTypeNames->Asset,
                                      /* custom = */ false,
                                      SdfVariabilityUniform,
                                      VtValue(sourceAsset),
                                      /* writeSparsely */ false);
}

bool 
UsdShadeShader::GetSourceAsset(
    SdfAssetPath *sourceAsset,
    const TfToken &sourceType) const
{
    TfToken implSource = GetImplementationSource();
    if (implSource != UsdShadeTokens->sourceAsset) {
        return false;
    }

    TfToken sourceAssetAttrName = _GetSourceAssetAttrName(sourceType);
    UsdAttribute sourceAssetAttr = GetPrim().GetAttribute(sourceAssetAttrName);
    if (sourceAssetAttr) {
        return sourceAssetAttr.Get(sourceAsset);
    }

    if (sourceType != UsdShadeTokens->universalSourceType) {
        UsdAttribute univSourceAssetAttr = GetPrim().GetAttribute(
                _GetSourceAssetAttrName(UsdShadeTokens->universalSourceType));
        if (univSourceAssetAttr) {
            return univSourceAssetAttr.Get(sourceAsset);
        }
    }

    return false;
}

static
TfToken
_GetSourceAssetSubIdentifierAttrName(const TfToken &sourceType)
{
    if (sourceType == UsdShadeTokens->universalSourceType) {
        return _tokens->infoSubIdentifier;
    }
    return TfToken(SdfPath::JoinIdentifier(TfTokenVector{
                                    _tokens->info,
                                    sourceType,
                                    UsdShadeTokens->sourceAsset,
                                    UsdShadeTokens->subIdentifier}));
}

bool
UsdShadeShader::SetSourceAssetSubIdentifier(
    const TfToken &subIdentifier,
    const TfToken &sourceType) const
{
    TfToken subIdentifierAttrName =
        _GetSourceAssetSubIdentifierAttrName(sourceType);
    return CreateImplementationSourceAttr(VtValue(UsdShadeTokens->sourceAsset))
        && UsdSchemaBase::_CreateAttr(subIdentifierAttrName,
                                      SdfValueTypeNames->Token,
                                      /* custom = */ false,
                                      SdfVariabilityUniform,
                                      VtValue(subIdentifier),
                                      /* writeSparsely */ false);
}

bool
UsdShadeShader::GetSourceAssetSubIdentifier(
    TfToken *subIdentifier,
    const TfToken &sourceType) const
{
    TfToken implSource = GetImplementationSource();
    if (implSource != UsdShadeTokens->sourceAsset) {
        return false;
    }

    TfToken subIdentifierAttrName =
        _GetSourceAssetSubIdentifierAttrName(sourceType);
    UsdAttribute subIdentifierAttr = GetPrim().GetAttribute(
        subIdentifierAttrName);
    if (subIdentifierAttr) {
        return subIdentifierAttr.Get(subIdentifier);
    }

    if (sourceType != UsdShadeTokens->universalSourceType) {
        UsdAttribute univSubIdentifierAttr = GetPrim().GetAttribute(
            _GetSourceAssetSubIdentifierAttrName(
                UsdShadeTokens->universalSourceType));
        if (univSubIdentifierAttr) {
            return univSubIdentifierAttr.Get(subIdentifier);
        }
    }

    return false;
}

static 
TfToken
_GetSourceCodeAttrName(const TfToken &sourceType) 
{
    if (sourceType == UsdShadeTokens->universalSourceType) {
        return _tokens->infoSourceCode;
    }
    return TfToken(SdfPath::JoinIdentifier(TfTokenVector{
                                    _tokens->info, 
                                    sourceType,
                                    UsdShadeTokens->sourceCode}));
}

bool 
UsdShadeShader::SetSourceCode(
    const std::string &sourceCode, 
    const TfToken &sourceType) const
{
    TfToken sourceCodeAttrName = _GetSourceCodeAttrName(sourceType);
    return CreateImplementationSourceAttr(VtValue(UsdShadeTokens->sourceCode)) 
        && UsdSchemaBase::_CreateAttr(sourceCodeAttrName,
                                      SdfValueTypeNames->String,
                                      /* custom = */ false,
                                      SdfVariabilityUniform,
                                      VtValue(sourceCode),
                                      /* writeSparsely */ false);
}

bool 
UsdShadeShader::GetSourceCode(
    std::string *sourceCode,
    const TfToken &sourceType) const
{
    TfToken implSource = GetImplementationSource();
    if (implSource != UsdShadeTokens->sourceCode) {
        return false;
    }

    TfToken sourceCodeAttrName = _GetSourceCodeAttrName(sourceType);
    UsdAttribute sourceCodeAttr = GetPrim().GetAttribute(sourceCodeAttrName);
    if (sourceCodeAttr) {
        return sourceCodeAttr.Get(sourceCode);
    }

    if (sourceType != UsdShadeTokens->universalSourceType) {
        UsdAttribute univSourceCodeAttr = GetPrim().GetAttribute(
                _GetSourceCodeAttrName(UsdShadeTokens->universalSourceType));
        if (univSourceCodeAttr) {
            return univSourceCodeAttr.Get(sourceCode);
        }
    }

    return false;
}

SdrShaderNodeConstPtr 
UsdShadeShader::GetShaderNodeForSourceType(const TfToken &sourceType) const
{
    TfToken implSource = GetImplementationSource();
    if (implSource == UsdShadeTokens->id) {
        TfToken shaderId;
        if (GetShaderId(&shaderId)) {
            return SdrRegistry::GetInstance().GetShaderNodeByIdentifierAndType(
                    shaderId, sourceType);
        }
    } else if (implSource == UsdShadeTokens->sourceAsset) {
        SdfAssetPath sourceAsset;
        if (GetSourceAsset(&sourceAsset, sourceType)) {
            TfToken subIdentifier;
            GetSourceAssetSubIdentifier(&subIdentifier, sourceType);
            return SdrRegistry::GetInstance().GetShaderNodeFromAsset(
                sourceAsset, GetSdrMetadata(), subIdentifier, sourceType);
        }
    } else if (implSource == UsdShadeTokens->sourceCode) {
        std::string sourceCode;
        if (GetSourceCode(&sourceCode, sourceType)) {
            return SdrRegistry::GetInstance().GetShaderNodeFromSourceCode(
                sourceCode, sourceType, GetSdrMetadata());
        }
    }

    return nullptr;
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
