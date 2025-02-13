//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/imaging/hdsi/nodeIdentifierResolvingSceneIndex.h"

#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/sdr/shaderNode.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/vt/dictionary.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (implementationSource)
    (sourceCode)
    (sourceAsset)
    ((sourceAssetSubIdentifier, "sourceAsset:subIdentifier"))
    (sdrMetadata)
);

namespace {

template<typename T>
T
_GetNodeTypeInfo(
    const HdMaterialNetworkInterface * const interface,
    const TfToken &nodeName,
    const TfToken &key)
{
    return
        interface
            ->GetNodeTypeInfoValue(nodeName, key)
            .GetWithDefault<T>();
}    

template<typename T>
T
_GetNodeTypeInfoForSourceType(
    const HdMaterialNetworkInterface * const interface,
    const TfToken &nodeName,
    const TfToken &sourceType,
    const TfToken &key)
{
    const TfToken fullKey(sourceType.GetString() + ":" + key.GetString());
    return _GetNodeTypeInfo<T>(interface, nodeName, fullKey);
}

NdrTokenMap
_ToNdrTokenMap(const VtDictionary &d)
{
    NdrTokenMap result;
    for (const auto &it : d) {
        result[TfToken(it.first)] = TfStringify(it.second);
    }
    return result;
}

SdrShaderNodeConstPtr
_GetSdrShaderNodeFromSourceAsset(
    const HdMaterialNetworkInterface * const interface,
    const TfToken &nodeName,
    const TfToken &sourceType)
{
    const SdfAssetPath shaderAsset =
        _GetNodeTypeInfoForSourceType<SdfAssetPath>(
            interface, nodeName, sourceType, _tokens->sourceAsset);

    const NdrTokenMap metadata =
        _ToNdrTokenMap(
            _GetNodeTypeInfo<VtDictionary>(
                interface, nodeName, _tokens->sdrMetadata));
    const TfToken subIdentifier =
        _GetNodeTypeInfoForSourceType<TfToken>(
            interface, nodeName, sourceType, _tokens->sourceAssetSubIdentifier);
    return
        SdrRegistry::GetInstance().GetShaderNodeFromAsset(
            shaderAsset, metadata, subIdentifier, sourceType);
}

SdrShaderNodeConstPtr
_GetSdrShaderNodeFromSourceCode(
    const HdMaterialNetworkInterface * const interface,
    const TfToken &nodeName,
    const TfToken &sourceType)
{
    const std::string sourceCode =
        _GetNodeTypeInfoForSourceType<std::string>(
            interface, nodeName, sourceType, _tokens->sourceCode);

    if (sourceCode.empty()) {
        return nullptr;
    }
    const NdrTokenMap metadata =
        _ToNdrTokenMap(
            _GetNodeTypeInfo<VtDictionary>(
                interface, nodeName, _tokens->sdrMetadata));
    
    return
        SdrRegistry::GetInstance().GetShaderNodeFromSourceCode(
            sourceCode, sourceType, metadata);
}    

SdrShaderNodeConstPtr
_GetSdrShaderNode(
    const HdMaterialNetworkInterface * const interface,
    const TfToken &nodeName,
    const TfToken &sourceType)
{
    const TfToken implementationSource =
        _GetNodeTypeInfo<TfToken>(
            interface, nodeName, _tokens->implementationSource);

    if (implementationSource == _tokens->sourceAsset) {
        return _GetSdrShaderNodeFromSourceAsset(interface, nodeName, sourceType);
    }
    if (implementationSource == _tokens->sourceCode) {
        return _GetSdrShaderNodeFromSourceCode(interface, nodeName, sourceType);
    }
    return nullptr;
}

void
_SetNodeTypeFromSourceAssetInfo(
    HdMaterialNetworkInterface * const interface,
    const TfToken &nodeName,
    const TfToken &sourceType)
{
    if (!interface->GetNodeType(nodeName).IsEmpty()) {
        return;
    }
     
    if (SdrShaderNodeConstPtr const sdrNode =
            _GetSdrShaderNode(interface, nodeName, sourceType)) {
        interface->SetNodeType(nodeName, sdrNode->GetIdentifier());
    }
}

void
_SetNodeTypesFromSourceAssetInfo(const TfToken& sourceType, 
                                 HdMaterialNetworkInterface* const interface)
{
    for (const TfToken& nodeName : interface->GetNodeNames()) {
        _SetNodeTypeFromSourceAssetInfo(interface, nodeName, sourceType);
    }
}

} // anonymous namespace

// static
HdSiNodeIdentifierResolvingSceneIndexRefPtr
HdSiNodeIdentifierResolvingSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    const TfToken &sourceType)
{
    return TfCreateRefPtr(
        new HdSiNodeIdentifierResolvingSceneIndex(inputSceneIndex, sourceType));
}

HdSiNodeIdentifierResolvingSceneIndex::HdSiNodeIdentifierResolvingSceneIndex(
    HdSceneIndexBaseRefPtr const &inputSceneIndex, const TfToken &sourceType)
  : HdMaterialFilteringSceneIndexBase(inputSceneIndex), _sourceType(sourceType)
{
    SetDisplayName(TfStringPrintf("HdSiNodeIdentifierResolvingSceneIndex (%s)", 
                                  sourceType.GetText()));
}

HdSiNodeIdentifierResolvingSceneIndex::
~HdSiNodeIdentifierResolvingSceneIndex()
    = default;

HdSiNodeIdentifierResolvingSceneIndex::FilteringFnc
HdSiNodeIdentifierResolvingSceneIndex::_GetFilteringFunction() const
{
    return std::bind(_SetNodeTypesFromSourceAssetInfo, 
                     _sourceType, 
                     std::placeholders::_1);
}

PXR_NAMESPACE_CLOSE_SCOPE
