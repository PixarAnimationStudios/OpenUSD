//
// Copyright 2021 Pixar
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
#include "pxr/imaging/hd/dataSourceMaterialNetworkInterface.h"
#include "pxr/imaging/hd/materialConnectionSchema.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/materialNodeParameterSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    // See note in GetModelAssetName()
    (model)
    (assetName)
    );

std::string
HdDataSourceMaterialNetworkInterface::GetModelAssetName() const
{
    // XXX If the model schema moves from UsdImaging back to HdModelSchema
    // in the future, we could use that here.
    if (_primContainer) {
        if (HdContainerDataSourceHandle modelDs =
            HdContainerDataSource::Cast(_primContainer->Get(_tokens->model))) {
            if (HdStringDataSourceHandle assetNameDs =
                HdStringDataSource::Cast(modelDs->Get(_tokens->assetName))) {
                return assetNameDs->GetTypedValue(0.0f);
            }
        }
    }
    return std::string();
}

HdMaterialNodeSchema
HdDataSourceMaterialNetworkInterface::_ResetIfNecessaryAndGetNode(
    const TfToken &nodeName) const
{
    if (_deletedNodes.find(nodeName) != _deletedNodes.end()) {
        return HdMaterialNodeSchema(nullptr);
    }

    if (nodeName == _lastAccessedNodeName) {
        return _lastAccessedNodeSchema;
    }

    if (!_nodesSchema) {
        _nodesSchema = _networkSchema.GetNodes();
    }

    _lastAccessedNodeName = nodeName;
    _lastAccessedNodeSchema = _nodesSchema.Get(nodeName);
    _lastAccessedNodeParametersSchema =
        HdMaterialNodeParameterContainerSchema(nullptr);
    _lastAccessedNodeConnectionsSchema = 
        HdMaterialConnectionVectorContainerSchema(nullptr);

    return _lastAccessedNodeSchema;
}

HdMaterialNodeParameterContainerSchema
HdDataSourceMaterialNetworkInterface::_GetNodeParameters(
    const TfToken &nodeName) const
{
    HdMaterialNodeSchema node = _ResetIfNecessaryAndGetNode(nodeName);
    if (_lastAccessedNodeParametersSchema) {
        return _lastAccessedNodeParametersSchema;
    }

    _lastAccessedNodeParametersSchema = node.GetParameters();

    return _lastAccessedNodeParametersSchema;
}

HdMaterialConnectionVectorContainerSchema
HdDataSourceMaterialNetworkInterface::_GetNodeConnections(
    const TfToken &nodeName) const
{
    HdMaterialNodeSchema node = _ResetIfNecessaryAndGetNode(nodeName);
    if (_lastAccessedNodeConnectionsSchema) {
        return _lastAccessedNodeConnectionsSchema;
    }

    _lastAccessedNodeConnectionsSchema = node.GetInputConnections();

    return _lastAccessedNodeConnectionsSchema;
}

void
HdDataSourceMaterialNetworkInterface::_SetOverride(
    const HdDataSourceLocator &loc,
    const HdDataSourceBaseHandle &ds)
{
    _networkEditor.Set(loc, ds);
    _existingOverrides[loc] = ds;

    static const HdDataSourceLocator nodesLocator(
        HdMaterialNetworkSchemaTokens->nodes);
    
    static const HdDataSourceLocator terminalsLocator(
        HdMaterialNetworkSchemaTokens->terminals);

    if (loc.Intersects(nodesLocator) && loc.GetElementCount() > 1) {
        _overriddenNodes.insert(loc.GetElement(1));
        _deletedNodes.erase(loc.GetElement(1));
    } else if (loc.Intersects(terminalsLocator)) {
        _terminalsOverridden = true;
    }
}

TfTokenVector
HdDataSourceMaterialNetworkInterface::GetNodeNames() const
{
    if (!_nodesSchema) {
        _nodesSchema = _networkSchema.GetNodes();
    }

    TfTokenVector result = _nodesSchema.GetNames();

    if (!_deletedNodes.empty()) {
        std::unordered_set<TfToken, TfHash> nameSet;
        nameSet.insert(result.begin(), result.end());
        for (const TfToken &deletedName : _deletedNodes) {
            nameSet.erase(deletedName);
        }

        result.clear();
        result.insert(result.end(), nameSet.begin(), nameSet.end());
    }

    return result;
}

TfToken
HdDataSourceMaterialNetworkInterface::GetNodeType(
    const TfToken &nodeName) const
{
    HdDataSourceLocator locator(
        HdMaterialNetworkSchemaTokens->nodes,
        nodeName,
        HdMaterialNodeSchemaTokens->nodeIdentifier);

    const auto it = _existingOverrides.find(locator);
    if (it != _existingOverrides.end()) {
        if (HdTypedSampledDataSource<TfToken>::Handle ds = 
                HdTypedSampledDataSource<TfToken>::Cast(it->second)) {
            return ds->GetTypedValue(0.0f);
        } else {
            // if not a token, it's deleted.
            return TfToken();
        }
    }

    if (HdTokenDataSourceHandle idDs =
                _ResetIfNecessaryAndGetNode(nodeName).GetNodeIdentifier()) {
        return idDs->GetTypedValue(0.0f);
    }

    return TfToken();
}

HdContainerDataSourceHandle
HdDataSourceMaterialNetworkInterface::_GetNodeTypeInfo(const TfToken& nodeName) const
{
    return _ResetIfNecessaryAndGetNode(nodeName).GetNodeTypeInfo();
}

TfTokenVector
HdDataSourceMaterialNetworkInterface::GetNodeTypeInfoKeys(
    const TfToken& nodeName) const
{
    HdContainerDataSourceHandle const nodeTypeInfo = _GetNodeTypeInfo(nodeName);
    if (!nodeTypeInfo) {
        return {};
    }
    return nodeTypeInfo->GetNames();
}

VtValue
HdDataSourceMaterialNetworkInterface::GetNodeTypeInfoValue(
    const TfToken& nodeName, const TfToken& key) const
{
    HdContainerDataSourceHandle const nodeTypeInfo = _GetNodeTypeInfo(nodeName);
    if (!nodeTypeInfo) {
        return {};
    }
    HdSampledDataSourceHandle const ds =
        HdSampledDataSource::Cast(nodeTypeInfo->Get(key));
    if (!ds) {
        return {};
    }
    return ds->GetValue(0.0f);
}

TfTokenVector
HdDataSourceMaterialNetworkInterface::GetAuthoredNodeParameterNames(
    const TfToken &nodeName) const
{
    TfTokenVector result = _GetNodeParameters(nodeName).GetNames();

    if (_overriddenNodes.find(nodeName) != _overriddenNodes.end()) {
        HdDataSourceLocator paramsLocator(
            HdMaterialNetworkSchemaTokens->nodes,
            nodeName,
            HdMaterialNodeSchemaTokens->parameters);

        std::unordered_set<TfToken, TfHash> nameSet;
        nameSet.insert(result.begin(), result.end());
        for (const auto &locDsPair : _existingOverrides) {
            // anything with this prefix will have at least 4 elements
            if (locDsPair.first.HasPrefix(paramsLocator)) {
                if (locDsPair.second) {
                    nameSet.insert(locDsPair.first.GetElement(3));
                } else {
                    nameSet.erase(locDsPair.first.GetElement(3));
                }
            }
        }

        result.clear();
        result.insert(result.end(), nameSet.begin(), nameSet.end());
    }

    return result;
}

VtValue
HdDataSourceMaterialNetworkInterface::GetNodeParameterValue(
    const TfToken &nodeName,
    const TfToken &paramName) const
{
    // check overrides for existing value
    HdDataSourceLocator locator(
        HdMaterialNetworkSchemaTokens->nodes,
        nodeName,
        HdMaterialNodeSchemaTokens->parameters,
        paramName);

    const auto it = _existingOverrides.find(locator);
    if (it != _existingOverrides.end()) {
        HdContainerDataSourceHandle param =
            HdContainerDataSource::Cast(it->second);
        HdMaterialNodeParameterSchema paramSchema(param);
        if (paramSchema) {
            HdSampledDataSourceHandle paramValueDS = paramSchema.GetValue();
            if (paramValueDS) {
                return paramValueDS->GetValue(0);
            }
        }
        // overridden with nullptr data source means deletion
        return VtValue();
    }

    if (HdSampledDataSourceHandle paramValueDS =
            _GetNodeParameters(nodeName).Get(paramName).GetValue()) {
        return paramValueDS->GetValue(0);
    }

    return VtValue();
}

HdMaterialNetworkInterface::NodeParamData
HdDataSourceMaterialNetworkInterface::GetNodeParameterData(
    const TfToken &nodeName,
    const TfToken &paramName) const
{
    // check overrides for existing value
    HdDataSourceLocator locator(
        HdMaterialNetworkSchemaTokens->nodes,
        nodeName,
        HdMaterialNodeSchemaTokens->parameters,
        paramName);

    HdMaterialNetworkInterface::NodeParamData paramData;
    const auto it = _existingOverrides.find(locator);
    if (it != _existingOverrides.end()) {
        HdContainerDataSourceHandle param = 
            HdContainerDataSource::Cast(it->second);
        HdMaterialNodeParameterSchema pSchema(param);
        if (pSchema) {
            HdSampledDataSourceHandle paramValueDS = pSchema.GetValue();
            if (paramValueDS) {
                paramData.value = paramValueDS->GetValue(0);
            }
            HdTokenDataSourceHandle colorSpaceDS = pSchema.GetColorSpace();
            if (colorSpaceDS) {
                paramData.colorSpace = colorSpaceDS->GetTypedValue(0);
            }
            return paramData;
        }
        // overridden with nullptr data source means deletion
        return paramData;
    }

    if (HdMaterialNodeParameterSchema pSchema =
        _GetNodeParameters(nodeName).Get(paramName)) {
        // Value
        if (HdSampledDataSourceHandle paramValueDS = pSchema.GetValue()) {
            paramData.value = paramValueDS->GetValue(0);
        }
            // ColorSpace
        if (HdTokenDataSourceHandle colorSpaceDS = pSchema.GetColorSpace()) {
            paramData.colorSpace = colorSpaceDS->GetTypedValue(0);
        }
    }

    return paramData;
}

TfTokenVector
HdDataSourceMaterialNetworkInterface::GetNodeInputConnectionNames(
    const TfToken &nodeName) const
{
    TfTokenVector result = _GetNodeConnections(nodeName).GetNames();

    if (_overriddenNodes.find(nodeName) != _overriddenNodes.end()) {
        HdDataSourceLocator inputsLocator(
            HdMaterialNetworkSchemaTokens->nodes,
            nodeName,
            HdMaterialNodeSchemaTokens->inputConnections);

        std::unordered_set<TfToken, TfHash> nameSet;
        nameSet.insert(result.begin(), result.end());
        for (const auto &locDsPair : _existingOverrides) {
            // anything with this prefix will have at least 4 elements
            if (locDsPair.first.HasPrefix(inputsLocator)) {
                if (locDsPair.second) {
                    nameSet.insert(locDsPair.first.GetElement(3));
                } else {
                    nameSet.erase(locDsPair.first.GetElement(3));
                }
            }
        }

        result.clear();
        result.insert(result.end(), nameSet.begin(), nameSet.end());
    }

    return result;
}

HdMaterialNetworkInterface::InputConnectionVector
HdDataSourceMaterialNetworkInterface::GetNodeInputConnection(
    const TfToken &nodeName,
    const TfToken &inputName) const
{
    // check overrides for existing value
    HdDataSourceLocator locator(
        HdMaterialNetworkSchemaTokens->nodes,
        nodeName,
        HdMaterialNodeSchemaTokens->inputConnections,
        inputName);

    HdVectorDataSourceHandle connectionVectorDs;
    const auto it = _existingOverrides.find(locator);
    if (it != _existingOverrides.end()) {
        if (HdVectorDataSourceHandle vds =
                HdVectorDataSource::Cast(it->second)) {
            connectionVectorDs = vds;
        } else {
            // overridden with nullptr means deletion
            return {};
        }
    }

    HdMaterialConnectionVectorSchema vectorSchema =
        connectionVectorDs
        ? HdMaterialConnectionVectorSchema(connectionVectorDs)
        : _GetNodeConnections(nodeName).Get(inputName);

    const size_t n = vectorSchema.GetNumElements();
    InputConnectionVector result;
    result.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        if (HdMaterialConnectionSchema schema = vectorSchema.GetElement(i)) {
            HdTokenDataSourceHandle nodeNameDs =
                schema.GetUpstreamNodePath();
            HdTokenDataSourceHandle outputNameDs =
                schema.GetUpstreamNodeOutputName();
            if (nodeNameDs && outputNameDs) {
                result.push_back({
                    nodeNameDs->GetTypedValue(0.0f),
                    outputNameDs->GetTypedValue(0.0f)});
            }
        }
    }

    return result;
}

void
HdDataSourceMaterialNetworkInterface::DeleteNode(const TfToken &nodeName)
{
    HdDataSourceLocator locator(
        HdMaterialNetworkSchemaTokens->nodes,
        nodeName);

    _networkEditor.Set(locator, nullptr);
    _deletedNodes.insert(nodeName);
}

void
HdDataSourceMaterialNetworkInterface::SetNodeType(
    const TfToken &nodeName,
    const TfToken &nodeType)
{
    HdDataSourceLocator locator(
        HdMaterialNetworkSchemaTokens->nodes,
        nodeName,
        HdMaterialNodeSchemaTokens->nodeIdentifier);

    HdDataSourceBaseHandle ds =
        HdRetainedTypedSampledDataSource<TfToken>::New(nodeType);

    _SetOverride(locator, ds);
}

void
HdDataSourceMaterialNetworkInterface::SetNodeParameterValue(
    const TfToken &nodeName,
    const TfToken &paramName,
    const VtValue &value)
{
    HdDataSourceLocator locator(
        HdMaterialNetworkSchemaTokens->nodes,
        nodeName,
        HdMaterialNodeSchemaTokens->parameters,
        paramName);

    HdDataSourceBaseHandle ds = 
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(HdRetainedTypedSampledDataSource<VtValue>::New(value))
            .Build();
    _SetOverride(locator, ds);
}

void
HdDataSourceMaterialNetworkInterface::SetNodeParameterData(
    const TfToken &nodeName,
    const TfToken &paramName,
    const NodeParamData &paramData)
{
    HdDataSourceLocator locator(
        HdMaterialNetworkSchemaTokens->nodes,
        nodeName,
        HdMaterialNodeSchemaTokens->parameters,
        paramName);

    HdDataSourceBaseHandle ds = 
        HdMaterialNodeParameterSchema::Builder()
            .SetValue(
                HdRetainedTypedSampledDataSource<VtValue>::New(paramData.value))
            .SetColorSpace(
                paramData.colorSpace.IsEmpty()
                    ? nullptr /* colorSpace */
                    : HdRetainedTypedSampledDataSource<TfToken>::New(
                        paramData.colorSpace))
            .Build();
    _SetOverride(locator, ds);
}

void
HdDataSourceMaterialNetworkInterface::DeleteNodeParameter(
    const TfToken &nodeName,
    const TfToken &paramName)
{
    HdDataSourceLocator locator(
        HdMaterialNetworkSchemaTokens->nodes,
        nodeName,
        HdMaterialNodeSchemaTokens->parameters,
        paramName);

    _SetOverride(locator, nullptr);
}

void
HdDataSourceMaterialNetworkInterface::SetNodeInputConnection(
    const TfToken &nodeName,
    const TfToken &inputName,
    const InputConnectionVector &connections)
{
    HdDataSourceLocator locator(
        HdMaterialNetworkSchemaTokens->nodes,
        nodeName,
        HdMaterialNodeSchemaTokens->inputConnections,
        inputName);

    std::vector<HdDataSourceBaseHandle> groups;
    groups.reserve(connections.size());

    using TDS = HdRetainedTypedSampledDataSource<TfToken>;

    for (const InputConnection &c : connections) {
        groups.push_back(HdMaterialConnectionSchema::Builder()
            .SetUpstreamNodePath(TDS::New(c.upstreamNodeName))
            .SetUpstreamNodeOutputName(TDS::New(c.upstreamOutputName))
            .Build());
    }

    HdDataSourceBaseHandle ds =
        HdRetainedSmallVectorDataSource::New(groups.size(), groups.data());
    _SetOverride(locator, ds);
}

void
HdDataSourceMaterialNetworkInterface::DeleteNodeInputConnection(
    const TfToken &nodeName,
    const TfToken &inputName)
{
    HdDataSourceLocator locator(
        HdMaterialNetworkSchemaTokens->nodes,
        nodeName,
        HdMaterialNodeSchemaTokens->inputConnections,
        inputName);

    _SetOverride(locator, nullptr);
}

TfTokenVector
HdDataSourceMaterialNetworkInterface::GetTerminalNames() const
{
    TfTokenVector result = _networkSchema.GetTerminals().GetNames();

    if (_terminalsOverridden) {
        static const HdDataSourceLocator terminalsLocator(
            HdMaterialNetworkSchemaTokens->terminals);

        std::unordered_set<TfToken, TfHash> nameSet;
        nameSet.insert(result.begin(), result.end());
        for (const auto &locDsPair : _existingOverrides) {
            // anything with this prefix will have at least 2 elements
            if (locDsPair.first.HasPrefix(terminalsLocator)) {
                if (locDsPair.second) {
                    nameSet.insert(locDsPair.first.GetElement(1));
                } else {
                    nameSet.erase(locDsPair.first.GetElement(1));
                }
            }
        }

        result.clear();
        result.insert(result.end(), nameSet.begin(), nameSet.end());
    }

    return result;
}

HdMaterialNetworkInterface::InputConnectionResult
HdDataSourceMaterialNetworkInterface::GetTerminalConnection(
    const TfToken &terminalName) const
{
    HdDataSourceLocator locator(
        HdMaterialNetworkSchemaTokens->terminals,
        terminalName);

    HdContainerDataSourceHandle container = nullptr;

    const auto it = _existingOverrides.find(locator);
    if (it != _existingOverrides.end()) {
        container = HdContainerDataSource::Cast(it->second);

        // If it's set but still isn't a container then it's been deleted
        if (!container) {
            return {false, {TfToken(), TfToken()}};
        }
    }

    HdMaterialConnectionSchema connectionSchema =
        container
        ? HdMaterialConnectionSchema(container)
        : _networkSchema.GetTerminals().Get(terminalName);

    if (connectionSchema) {
        InputConnectionResult result = {true, {TfToken(), TfToken()}};

        if (HdTypedSampledDataSource<TfToken>::Handle ds =
                connectionSchema.GetUpstreamNodePath()) {
            result.second.upstreamNodeName = ds->GetTypedValue(0.0f);
        } else {
            result.first = false;
        }

        // output name is optional for a terminal
        if (HdTypedSampledDataSource<TfToken>::Handle ds =
                connectionSchema.GetUpstreamNodeOutputName()) {
            result.second.upstreamOutputName = ds->GetTypedValue(0.0f);
        }

        return result;
    }

    return {false, {TfToken(), TfToken()}};
}

void
HdDataSourceMaterialNetworkInterface::DeleteTerminal(
    const TfToken &terminalName)
{
    HdDataSourceLocator locator(
        HdMaterialNetworkSchemaTokens->terminals,
        terminalName);

    _SetOverride(locator, nullptr);
}

void
HdDataSourceMaterialNetworkInterface::SetTerminalConnection(
    const TfToken &terminalName,
    const InputConnection &connection)
{
    HdDataSourceLocator locator(
    HdMaterialNetworkSchemaTokens->terminals,
        terminalName);

    HdContainerDataSourceHandle ds =
        HdMaterialConnectionSchema::Builder()
            .SetUpstreamNodePath(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    connection.upstreamNodeName))
            .SetUpstreamNodeOutputName(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    connection.upstreamOutputName))
            .Build();

    _SetOverride(locator, ds);
}

HdContainerDataSourceHandle
HdDataSourceMaterialNetworkInterface::Finish()
{
    if (_existingOverrides.empty()) {
        return _networkSchema.GetContainer();
    }

    return _networkEditor.Finish();
}

PXR_NAMESPACE_CLOSE_SCOPE
