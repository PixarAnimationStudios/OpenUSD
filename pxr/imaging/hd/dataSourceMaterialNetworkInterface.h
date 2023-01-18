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
#ifndef HD_DATA_SOURCE_MATERIAL_NETWORK_INTERFACE_H
#define HD_DATA_SOURCE_MATERIAL_NETWORK_INTERFACE_H

#include "pxr/usd/sdf/path.h"
#include "pxr/imaging/hd/containerDataSourceEditor.h"
#include "pxr/imaging/hd/materialNetworkInterface.h"
#include <unordered_map>
#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdDataSourceMaterialNetworkInterface
///
/// Implements HdMaterialNetworkInterface for reading from and overriding
/// values within data sources. Overrides are managed internally via an
/// HdContainerDataSourceEditor. Calling Finish returns the resulting 
/// container data resource representing an individual material network. If
/// nothing is overriden, the input data source is returned.
class HdDataSourceMaterialNetworkInterface
    : public HdMaterialNetworkInterface
{
public:

    HdDataSourceMaterialNetworkInterface(
        const SdfPath &materialPrimPath,
        const HdContainerDataSourceHandle &networkContainer)
    : _materialPrimPath(materialPrimPath)
    , _networkContainer(networkContainer)
    , _containerEditor(networkContainer)
    {}

    SdfPath GetMaterialPrimPath() const override {
        return _materialPrimPath;
    }

    HD_API
    TfTokenVector GetNodeNames() const override;

    HD_API
    TfToken GetNodeType(const TfToken &nodeName) const override;

    HD_API
    TfTokenVector GetNodeTypeInfoKeys(const TfToken& nodeName) const override;
    HD_API
    VtValue GetNodeTypeInfoValue(
        const TfToken& nodeName, const TfToken& value) const override;

    HD_API
    TfTokenVector GetAuthoredNodeParameterNames(
        const TfToken &nodeName) const override;
    
    HD_API
    VtValue GetNodeParameterValue(
        const TfToken &nodeName,
        const TfToken &paramName) const override;

    HD_API
    TfTokenVector GetNodeInputConnectionNames(
        const TfToken &nodeName) const override;

    HD_API
    InputConnectionVector GetNodeInputConnection(
        const TfToken &nodeName,
        const TfToken &inputName) const override;

    HD_API
    void DeleteNode(const TfToken &nodeName) override;

    HD_API
    void SetNodeType(
        const TfToken &nodeName,
        const TfToken &nodeType) override;

    HD_API
    void SetNodeParameterValue(
        const TfToken &nodeName,
        const TfToken &paramName,
        const VtValue &value) override;

    HD_API
    void DeleteNodeParameter(
        const TfToken &nodeName,
        const TfToken &paramName) override;

    HD_API
    void SetNodeInputConnection(
        const TfToken &nodeName,
        const TfToken &inputName,
        const InputConnectionVector &connections) override;

    HD_API
    void DeleteNodeInputConnection(
        const TfToken &nodeName,
        const TfToken &inputName) override;

    HD_API
    TfTokenVector GetTerminalNames() const override;

    HD_API
    InputConnectionResult GetTerminalConnection(
        const TfToken &terminalName) const override;

    HD_API
    void DeleteTerminal(
        const TfToken &terminalName) override;

    HD_API
    void SetTerminalConnection(
        const TfToken &terminalName,
        const InputConnection &connection) override;

    HD_API
    HdContainerDataSourceHandle Finish();

private:
    HdContainerDataSourceHandle _GetNodeTypeInfo(
            const TfToken& nodeName) const;

    using _OverrideMap =
        std::unordered_map<HdDataSourceLocator, HdDataSourceBaseHandle,
            TfHash>;

    using _TokenSet = std::unordered_set<TfToken, TfHash>;

    void _SetOverride(
        const HdDataSourceLocator &loc,
        const HdDataSourceBaseHandle &ds);

    SdfPath _materialPrimPath;
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

#endif // HD_DATA_SOURCE_MATERIAL_NETWORK_INTERFACE_H
