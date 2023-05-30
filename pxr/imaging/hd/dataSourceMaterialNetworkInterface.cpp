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
#include "pxr/imaging/hd/retainedDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

HdContainerDataSourceHandle
HdDataSourceMaterialNetworkInterface::_GetNode(
    const TfToken &nodeName) const
{
    if (_deletedNodes.find(nodeName) != _deletedNodes.end()) {
        return nullptr;
    }

    if (nodeName == _lastAccessedNodeName) {
        return _lastAccessedNode;
    }

    if (!_nodesContainer) {
        _nodesContainer = HdMaterialNetworkSchema(_networkContainer).GetNodes();
    }

    _lastAccessedNodeName = nodeName;
    _lastAccessedNode = nullptr;
    _lastAccessedNodeParameters = nullptr;
    _lastAccessedNodeConnections = nullptr;

    if (!_nodesContainer) {
        return nullptr;
    }

    _lastAccessedNode =
        HdContainerDataSource::Cast(_nodesContainer->Get(nodeName));

    return _lastAccessedNode;
}

HdContainerDataSourceHandle
HdDataSourceMaterialNetworkInterface::_GetNodeParameters(
    const TfToken &nodeName) const
{
    if (HdContainerDataSourceHandle node = _GetNode(nodeName)) {
        if (_lastAccessedNodeParameters) {
            return _lastAccessedNodeParameters;
        }

        _lastAccessedNodeParameters =
            HdMaterialNodeSchema(node).GetParameters();
        
        return _lastAccessedNodeParameters;
    }

    return nullptr;
}

HdContainerDataSourceHandle
HdDataSourceMaterialNetworkInterface::_GetNodeConnections(
    const TfToken &nodeName) const
{
    if (HdContainerDataSourceHandle node = _GetNode(nodeName)) {
        if (_lastAccessedNodeConnections) {
            return _lastAccessedNodeConnections;
        }

        _lastAccessedNodeConnections =
            HdMaterialNodeSchema(node).GetInputConnections();
        
        return _lastAccessedNodeConnections;
    }

    return nullptr;
}

void
HdDataSourceMaterialNetworkInterface::_SetOverride(
    const HdDataSourceLocator &loc,
    const HdDataSourceBaseHandle &ds)
{
    _containerEditor.Set(loc, ds);
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
    if (!_nodesContainer) {
        _nodesContainer = HdMaterialNetworkSchema(_networkContainer).GetNodes();
    }

    if (!_nodesContainer) {
        return {};
    }

    TfTokenVector result = _nodesContainer->GetNames();

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

    HdMaterialNodeSchema node(_GetNode(nodeName));
    if (node) {
        if (HdTokenDataSourceHandle idDs = node.GetNodeIdentifier()) {
            return idDs->GetTypedValue(0.0f);
        }
    }

    return TfToken();
}

HdContainerDataSourceHandle
HdDataSourceMaterialNetworkInterface::_GetNodeTypeInfo(const TfToken& nodeName) const
{
    HdContainerDataSourceHandle const node = _GetNode(nodeName);
    if (!node) {
        return nullptr;
    }
    return HdContainerDataSource::Cast(
        node->Get(HdMaterialNodeSchemaTokens->nodeTypeInfo));
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
    TfTokenVector result;
    if (HdContainerDataSourceHandle params = _GetNodeParameters(nodeName)) {
        result = params->GetNames();
    }

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
        if (HdSampledDataSourceHandle sds =
                HdSampledDataSource::Cast(it->second)) {
            return sds->GetValue(0.0f);
        } else {
            // overridden with nullptr data source means deletion
            return VtValue();
        }
    }

    if (HdContainerDataSourceHandle params = _GetNodeParameters(nodeName)) {
        if (HdSampledDataSourceHandle param =
                HdSampledDataSource::Cast(params->Get(paramName))) {
            return param->GetValue(0.0f);
        }
    }

    return VtValue();
}

TfTokenVector
HdDataSourceMaterialNetworkInterface::GetNodeInputConnectionNames(
    const TfToken &nodeName) const
{
    TfTokenVector result;

    if (HdContainerDataSourceHandle inputs = _GetNodeConnections(nodeName)) {
        result = inputs->GetNames();
    }

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

    if (!connectionVectorDs) {
        if (HdContainerDataSourceHandle inputs = _GetNodeConnections(nodeName)) {
            connectionVectorDs =
                HdVectorDataSource::Cast(inputs->Get(inputName));
        }
    }

    if (!connectionVectorDs) {
        return {};
    }

    size_t e = connectionVectorDs->GetNumElements();
    InputConnectionVector result;
    result.reserve(e);
    for (size_t i = 0; i < e; ++i) {
        HdMaterialConnectionSchema c(HdContainerDataSource::Cast(
            connectionVectorDs->GetElement(i)));
        if (c) {
            HdTokenDataSourceHandle nodeNameDs = c.GetUpstreamNodePath();
            HdTokenDataSourceHandle outputNameDs = c.GetUpstreamNodeOutputName();
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

    _containerEditor.Set(locator, nullptr);
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

    HdDataSourceBaseHandle ds = HdRetainedSampledDataSource::New(value);
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
    TfTokenVector result;

    HdContainerDataSourceHandle terminals =
        HdMaterialNetworkSchema(_networkContainer).GetTerminals();

    if (terminals) {
        result =  terminals->GetNames();
    }

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

    if (!container) {
        HdContainerDataSourceHandle terminals =
            HdMaterialNetworkSchema(_networkContainer).GetTerminals();
        if (terminals) {
            container =
                HdContainerDataSource::Cast(terminals->Get(terminalName));
        }
    }

    HdMaterialConnectionSchema connectionSchema(container);
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
        return _networkContainer;
    }

    return _containerEditor.Finish();
}

PXR_NAMESPACE_CLOSE_SCOPE
