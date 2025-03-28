//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_MATERIAL_NETWORK_INTERFACE_H
#define PXR_IMAGING_HD_MATERIAL_NETWORK_INTERFACE_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/tf/smallVector.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdMaterialNetworkInterface
///
/// Abstract interface for querying and mutating a material network.
///
/// This is useful for implementing matfilt functions which can be reused
/// by future scene index implementations.
///
/// NOTE: Subclasses make no guarantee of thread-safety even for the const
///       accessors as they might make use of internal caching for optimization.
///       Should you want to read from a material from multiple threads, create
///       a thread-specific interface instance. The non-const methods should
///       never be considered thread-safe from multiple interface instances
///       backed from the same concrete data.
class HdMaterialNetworkInterface
{
public:
    virtual ~HdMaterialNetworkInterface() = default;

    virtual SdfPath GetMaterialPrimPath() const = 0;

    /// Material config is a collection of data related to the entire material,
    /// e.g. material definition version, etc.
    ///
    /// Similarly to GetNodeTypeInfoXXX() below, only getters are provided, as
    /// we don't intend to mutate this config data.
    virtual TfTokenVector GetMaterialConfigKeys() const = 0;
    virtual VtValue GetMaterialConfigValue(const TfToken& key) const = 0;

    /// Returns the nearest enclosing model asset name, as described by
    /// the model schema, or empty string if none is available.
    virtual std::string GetModelAssetName() const = 0;

    virtual TfTokenVector GetNodeNames() const  = 0;
    virtual TfToken GetNodeType(const TfToken &nodeName) const = 0;

    /// Node type info is a collection of data related to the node type, often
    /// used to determine the node type.
    ///
    virtual TfTokenVector
    GetNodeTypeInfoKeys(const TfToken& nodeName) const = 0;
    virtual VtValue
    GetNodeTypeInfoValue(const TfToken& nodeName, const TfToken& key) const = 0;

    virtual TfTokenVector GetAuthoredNodeParameterNames(
        const TfToken &nodeName) const = 0;
    
    virtual VtValue GetNodeParameterValue(
        const TfToken &nodeName,
        const TfToken &paramName) const = 0;
    
    struct NodeParamData
    {
        VtValue value;
        TfToken colorSpace;
        TfToken typeName;
    };

    virtual NodeParamData GetNodeParameterData(
        const TfToken &nodeName,
        const TfToken &paramName) const = 0;

    virtual TfTokenVector GetNodeInputConnectionNames(
        const TfToken &nodeName) const = 0;

    struct InputConnection
    {
        TfToken upstreamNodeName;
        TfToken upstreamOutputName;
    };
    using InputConnectionVector = TfSmallVector<InputConnection, 4>;

    virtual InputConnectionVector GetNodeInputConnection(
        const TfToken &nodeName,
        const TfToken &inputName) const = 0;

    virtual void DeleteNode(const TfToken &nodeName) = 0;

    virtual void SetNodeType(
        const TfToken &nodeName,
        const TfToken &nodeType) = 0;

    virtual void SetNodeTypeInfoValue(
        const TfToken &nodeName,
        const TfToken &key,
        const VtValue &value
    ) = 0;

    virtual void SetNodeParameterValue(
        const TfToken &nodeName,
        const TfToken &paramName,
        const VtValue &value) = 0;
    
    virtual void SetNodeParameterData(
        const TfToken &nodeName,
        const TfToken &paramName,
        const NodeParamData &paramData) = 0;

    virtual void DeleteNodeParameter(
        const TfToken &nodeName,
        const TfToken &paramName) = 0;

    virtual void SetNodeInputConnection(
        const TfToken &nodeName,
        const TfToken &inputName,
        const InputConnectionVector &connections) = 0;

    virtual void DeleteNodeInputConnection(
        const TfToken &nodeName,
        const TfToken &inputName) = 0;

    /// ------------------------------------------------------------------------
    /// Terminal query & mutation
    virtual TfTokenVector GetTerminalNames() const = 0;

    using InputConnectionResult = std::pair<bool, InputConnection>;
    virtual InputConnectionResult GetTerminalConnection(
        const TfToken &terminalName) const = 0;

    virtual void DeleteTerminal(
        const TfToken &terminalName) = 0;

    virtual void SetTerminalConnection(
        const TfToken &terminalName,
        const InputConnection &connection) = 0;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_MATERIAL_NETWORK_INTERFACE_H
