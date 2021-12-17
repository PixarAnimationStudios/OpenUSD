
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
#ifndef EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_DATA_SOURCE_MATERIAL_NETWORK_INTERFACE_H
#define EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_DATA_SOURCE_MATERIAL_NETWORK_INTERFACE_H

#include "pxr/imaging/hd/containerDataSourceEditor.h"
#include "pxr/imaging/hd/materialNetworkInterface.h"
#include <unordered_map>
#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrmanDataSourceMaterialNetworkInterface
///
/// Implements HdMaterialNetworkInterface for reading from and overriding
/// values within data sources. Overrides are managed internally via an
/// HdContainerDataSourceEditor. Calling Finish returns the resulting 
/// container data resource representing an individual material network. If
/// nothing is overriden, the input data source is returned.
class HdPrmanDataSourceMaterialNetworkInterface
    : public HdMaterialNetworkInterface
{
public:

    HdPrmanDataSourceMaterialNetworkInterface(
        const HdContainerDataSourceHandle &networkContainer)
    : _networkContainer(networkContainer)
    , _containerEditor(networkContainer)
    {}

    TfTokenVector GetNodeNames() const override;
    TfToken GetNodeType(const TfToken &nodeName) const override;

    TfTokenVector GetAuthoredNodeParameterNames(
        const TfToken &nodeName) const override;
    
    VtValue GetNodeParameterValue(
        const TfToken &nodeName,
        const TfToken &paramName) const override;

    TfTokenVector GetNodeInputConnectionNames(
        const TfToken &nodeName) const override;

    InputConnectionVector GetNodeInputConnection(
        const TfToken &nodeName,
        const TfToken &inputName) const override;

    void DeleteNode(const TfToken &nodeName) override;

    void SetNodeType(
        const TfToken &nodeName,
        const TfToken &nodeType) override;

    void SetNodeParameterValue(
        const TfToken &nodeName,
        const TfToken &paramName,
        const VtValue &value) override;

    void DeleteNodeParameter(
        const TfToken &nodeName,
        const TfToken &paramName) override;

    void SetNodeInputConnection(
        const TfToken &nodeName,
        const TfToken &inputName,
        const InputConnectionVector &connections) override;

    void DeleteNodeInputConnection(
        const TfToken &nodeName,
        const TfToken &inputName) override;

    TfTokenVector GetTerminalNames() const override;

    InputConnectionResult GetTerminalConnection(
        const TfToken &terminalName) const override;

    void DeleteTerminal(
        const TfToken &terminalName) override;

    void SetTerminalConnection(
        const TfToken &terminalName,
        const InputConnection &connection) override;

    HdContainerDataSourceHandle Finish();

private:
    using _OverrideMap =
        std::unordered_map<HdDataSourceLocator, HdDataSourceBaseHandle,
            TfHash>;

    using _TokenSet = std::unordered_set<TfToken, TfHash>;

    void _SetOverride(
        const HdDataSourceLocator &loc,
        const HdDataSourceBaseHandle &ds);

    HdContainerDataSourceHandle _networkContainer;
    HdContainerDataSourceEditor _containerEditor;
    _OverrideMap _existingOverrides;
    _TokenSet _overriddenNodes;
    _TokenSet _deletedNodes;
    bool _terminalsOverridden = false;

    // cache some common child containers to avoid repeated access
    HdContainerDataSourceHandle _GetNode(
        const TfToken &nodeName) const;
    HdContainerDataSourceHandle _GetNodeParameters(
        const TfToken &nodeName) const;
    HdContainerDataSourceHandle _GetNodeConnections(
        const TfToken &nodeName) const;

    mutable HdContainerDataSourceHandle _nodesContainer;
    mutable TfToken _lastAccessedNodeName;
    mutable HdContainerDataSourceHandle _lastAccessedNode;
    mutable HdContainerDataSourceHandle _lastAccessedNodeParameters;
    mutable HdContainerDataSourceHandle _lastAccessedNodeConnections;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_DATA_SOURCE_MATERIAL_NETWORK_INTERFACE_H