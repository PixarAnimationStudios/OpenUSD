//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef HD_DATA_SOURCE_MATERIAL_NETWORK_INTERFACE_H
#define HD_DATA_SOURCE_MATERIAL_NETWORK_INTERFACE_H

#include "pxr/usd/sdf/path.h"
#include "pxr/imaging/hd/containerDataSourceEditor.h"
#include "pxr/imaging/hd/materialNetworkInterface.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/schemaTypeDefs.h"
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
        const HdContainerDataSourceHandle &networkContainer,
        const HdContainerDataSourceHandle &primContainer)
    : _materialPrimPath(materialPrimPath)
    , _networkSchema(networkContainer)
    , _networkEditor(networkContainer)
    , _primContainer(primContainer)
    , _nodesSchema(nullptr)
    , _lastAccessedNodeSchema(nullptr)
    , _lastAccessedNodeParametersSchema(nullptr)
    , _lastAccessedNodeConnectionsSchema(nullptr)
    {}

    HD_API
    SdfPath GetMaterialPrimPath() const override {
        return _materialPrimPath;
    }

    HD_API
    TfTokenVector GetMaterialConfigKeys() const override;
    HD_API
    VtValue GetMaterialConfigValue(const TfToken& key) const override;

    HD_API
    std::string GetModelAssetName() const override;

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
    HdMaterialNetworkInterface::NodeParamData GetNodeParameterData(
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

    /// Set the nodeType for the shader node with the given \p nodeName. 
    /// If \p nodeType is empty then the nodeType attribute will be removed
    /// from the node's dataSource.
    HD_API
    void SetNodeType(
        const TfToken &nodeName,
        const TfToken &nodeType) override;

    HD_API
    void SetNodeTypeInfoValue(
        const TfToken &nodeName,
        const TfToken &key,
        const VtValue &value) override;

    HD_API
    void SetNodeParameterValue(
        const TfToken &nodeName,
        const TfToken &paramName,
        const VtValue &value) override;

    HD_API
    void SetNodeParameterData(
        const TfToken &nodeName,
        const TfToken &paramName,
        const NodeParamData &paramData) override;

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

    /// Return the nodeTypeInfo dataSource, if it exists, for the specified 
    /// \p nodeName. Does NOT take into account any overrides of the nodeType 
    /// data that may have been authored by SetNodeTypeInfoValue().
    HdContainerDataSourceHandle _GetOriginalNodeTypeInfo(
            const TfToken& nodeName) const;

    /// Return the nodeTypeInfo dataSource for the supplied \p nodeName,  
    /// taking into account any overrides that may have been authored.
    HdContainerDataSourceHandle _GetNodeTypeInfo(
            const TfToken& nodeName) const;

    using _OverrideMap =
        std::unordered_map<HdDataSourceLocator, HdDataSourceBaseHandle,
            TfHash>;

    using _HdContainerDataSourceEditorSharedPtr = 
        std::shared_ptr<class HdContainerDataSourceEditor>;

    using _NodeTypeInfoMap = 
        std::unordered_map<TfToken, _HdContainerDataSourceEditorSharedPtr, 
            TfToken::HashFunctor>;

    using _TokenSet = std::unordered_set<TfToken, TfHash>;

    void _SetOverride(
        const HdDataSourceLocator &loc,
        const HdDataSourceBaseHandle &ds);

    SdfPath _materialPrimPath;
    mutable HdMaterialNetworkSchema _networkSchema;
    HdContainerDataSourceEditor _networkEditor;
    _NodeTypeInfoMap _nodeTypeInfoOverrides;
    HdContainerDataSourceHandle _primContainer;
    _OverrideMap _existingOverrides;
    _TokenSet _overriddenNodes;
    _TokenSet _deletedNodes;
    bool _terminalsOverridden = false;

    // cache some common child containers to avoid repeated access
    HdMaterialNodeSchema _ResetIfNecessaryAndGetNode(
        const TfToken &nodeName) const;
    HdMaterialNodeParameterContainerSchema _GetNodeParameters(
        const TfToken &nodeName) const;
    HdMaterialConnectionVectorContainerSchema _GetNodeConnections(
        const TfToken &nodeName) const;

    mutable HdMaterialNodeContainerSchema _nodesSchema;


    mutable TfToken _lastAccessedNodeName;


    mutable HdMaterialNodeSchema _lastAccessedNodeSchema;
    mutable HdMaterialNodeParameterContainerSchema _lastAccessedNodeParametersSchema;
    mutable HdMaterialConnectionVectorContainerSchema _lastAccessedNodeConnectionsSchema;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_DATA_SOURCE_MATERIAL_NETWORK_INTERFACE_H
