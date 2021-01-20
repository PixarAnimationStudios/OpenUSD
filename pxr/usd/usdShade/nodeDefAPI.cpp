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
#include "pxr/usd/usdShade/nodeDefAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdShadeNodeDefAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (NodeDefAPI)
);

/* virtual */
UsdShadeNodeDefAPI::~UsdShadeNodeDefAPI()
{
}

/* static */
UsdShadeNodeDefAPI
UsdShadeNodeDefAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdShadeNodeDefAPI();
    }
    return UsdShadeNodeDefAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdShadeNodeDefAPI::_GetSchemaKind() const {
    return UsdShadeNodeDefAPI::schemaKind;
}

/* virtual */
UsdSchemaKind UsdShadeNodeDefAPI::_GetSchemaType() const {
    return UsdShadeNodeDefAPI::schemaType;
}

/* static */
UsdShadeNodeDefAPI
UsdShadeNodeDefAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdShadeNodeDefAPI>()) {
        return UsdShadeNodeDefAPI(prim);
    }
    return UsdShadeNodeDefAPI();
}

/* static */
const TfType &
UsdShadeNodeDefAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdShadeNodeDefAPI>();
    return tfType;
}

/* static */
bool 
UsdShadeNodeDefAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdShadeNodeDefAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdShadeNodeDefAPI::GetImplementationSourceAttr() const
{
    return GetPrim().GetAttribute(UsdShadeTokens->infoImplementationSource);
}

UsdAttribute
UsdShadeNodeDefAPI::CreateImplementationSourceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdShadeTokens->infoImplementationSource,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdShadeNodeDefAPI::GetIdAttr() const
{
    return GetPrim().GetAttribute(UsdShadeTokens->infoId);
}

UsdAttribute
UsdShadeNodeDefAPI::CreateIdAttr(VtValue const &defaultValue, bool writeSparsely) const
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
UsdShadeNodeDefAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdShadeTokens->infoImplementationSource,
        UsdShadeTokens->infoId,
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

#include "pxr/usd/sdr/registry.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (info)
    ((infoSourceAsset, "info:sourceAsset"))
    ((infoSubIdentifier, "info:sourceAsset:subIdentifier"))
    ((infoSourceCode, "info:sourceCode"))
);

TfToken 
UsdShadeNodeDefAPI::GetImplementationSource() const
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
UsdShadeNodeDefAPI::SetShaderId(const TfToken &id) const
{
    return CreateImplementationSourceAttr(VtValue(UsdShadeTokens->id), 
                                          /*writeSparsely*/ true) &&
           GetIdAttr().Set(id);
}

bool 
UsdShadeNodeDefAPI::GetShaderId(TfToken *id) const
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
UsdShadeNodeDefAPI::SetSourceAsset(
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
UsdShadeNodeDefAPI::GetSourceAsset(
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
UsdShadeNodeDefAPI::SetSourceAssetSubIdentifier(
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
UsdShadeNodeDefAPI::GetSourceAssetSubIdentifier(
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
UsdShadeNodeDefAPI::SetSourceCode(
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
UsdShadeNodeDefAPI::GetSourceCode(
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

static NdrTokenMap
_GetSdrMetadata(UsdPrim const& prim)
{
    NdrTokenMap result;

    VtDictionary sdrMetadata;
    if (prim.GetMetadata(UsdShadeTokens->sdrMetadata, &sdrMetadata)){
        for (const auto &it : sdrMetadata) {
            result[TfToken(it.first)] = TfStringify(it.second);
        }
    }

    return result;
}

SdrShaderNodeConstPtr 
UsdShadeNodeDefAPI::GetShaderNodeForSourceType(const TfToken &sourceType) const
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
                sourceAsset, _GetSdrMetadata(GetPrim()), subIdentifier, sourceType);
        }
    } else if (implSource == UsdShadeTokens->sourceCode) {
        std::string sourceCode;
        if (GetSourceCode(&sourceCode, sourceType)) {
            return SdrRegistry::GetInstance().GetShaderNodeFromSourceCode(
                sourceCode, sourceType, _GetSdrMetadata(GetPrim()));
        }
    }

    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
