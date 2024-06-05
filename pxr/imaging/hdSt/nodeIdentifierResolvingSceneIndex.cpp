//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/imaging/hdSt/nodeIdentifierResolvingSceneIndex.h"

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

    (glslfx)
);

namespace {

const TfToken _sourceType = _tokens->glslfx;

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
    const TfToken &key)
{
    static const std::string prefix = _sourceType.GetString() + ":";
    const TfToken fullKey(prefix + key.GetString());

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
    const TfToken &nodeName)
{
    const SdfAssetPath shaderAsset =
        _GetNodeTypeInfoForSourceType<SdfAssetPath>(
            interface, nodeName, _tokens->sourceAsset);

    const NdrTokenMap metadata =
        _ToNdrTokenMap(
            _GetNodeTypeInfo<VtDictionary>(
                interface, nodeName, _tokens->sdrMetadata));
    const TfToken subIdentifier =
        _GetNodeTypeInfoForSourceType<TfToken>(
            interface, nodeName, _tokens->sourceAssetSubIdentifier);

    return
        SdrRegistry::GetInstance().GetShaderNodeFromAsset(
            shaderAsset, metadata, subIdentifier, _sourceType);
}

SdrShaderNodeConstPtr
_GetSdrShaderNodeFromSourceCode(
    const HdMaterialNetworkInterface * const interface,
    const TfToken &nodeName)
{
    const std::string sourceCode =
        _GetNodeTypeInfoForSourceType<std::string>(
            interface, nodeName, _tokens->sourceCode);

    if (sourceCode.empty()) {
        return nullptr;
    }
    const NdrTokenMap metadata =
        _ToNdrTokenMap(
            _GetNodeTypeInfo<VtDictionary>(
                interface, nodeName, _tokens->sdrMetadata));
    
    return
        SdrRegistry::GetInstance().GetShaderNodeFromSourceCode(
            sourceCode, _sourceType, metadata);
}    

SdrShaderNodeConstPtr
_GetSdrShaderNode(
    const HdMaterialNetworkInterface * const interface,
    const TfToken &nodeName)
{
    const TfToken implementationSource =
        _GetNodeTypeInfo<TfToken>(
            interface, nodeName, _tokens->implementationSource);

    if (implementationSource == _tokens->sourceAsset) {
        return _GetSdrShaderNodeFromSourceAsset(interface, nodeName);
    }
    if (implementationSource == _tokens->sourceCode) {
        return _GetSdrShaderNodeFromSourceCode(interface, nodeName);
    }
    return nullptr;
}

void
_SetNodeTypeFromSourceAssetInfo(
    const TfToken &nodeName,
    HdMaterialNetworkInterface * const interface)
{
    if (!interface->GetNodeType(nodeName).IsEmpty()) {
        return;
    }
     
    if (SdrShaderNodeConstPtr const sdrNode =
            _GetSdrShaderNode(interface, nodeName)) {
        interface->SetNodeType(nodeName, sdrNode->GetIdentifier());
    }
}

void
_SetNodeTypesFromSourceAssetInfo(HdMaterialNetworkInterface* const interface)
{
    for (const TfToken& nodeName : interface->GetNodeNames()) {
        _SetNodeTypeFromSourceAssetInfo(nodeName, interface);
    }
}

} // anonymous namespace

// static
HdSt_NodeIdentifierResolvingSceneIndexRefPtr
HdSt_NodeIdentifierResolvingSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex)
{
    return TfCreateRefPtr(
        new HdSt_NodeIdentifierResolvingSceneIndex(inputSceneIndex));
}

HdSt_NodeIdentifierResolvingSceneIndex::HdSt_NodeIdentifierResolvingSceneIndex(
    HdSceneIndexBaseRefPtr const &inputSceneIndex)
  : HdMaterialFilteringSceneIndexBase(inputSceneIndex)
{
}

HdSt_NodeIdentifierResolvingSceneIndex::
~HdSt_NodeIdentifierResolvingSceneIndex()
    = default;

HdSt_NodeIdentifierResolvingSceneIndex::FilteringFnc
HdSt_NodeIdentifierResolvingSceneIndex::_GetFilteringFunction() const
{
    return _SetNodeTypesFromSourceAssetInfo;
}

PXR_NAMESPACE_CLOSE_SCOPE
