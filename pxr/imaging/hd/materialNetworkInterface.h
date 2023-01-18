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

    virtual TfTokenVector GetNodeNames() const  = 0;
    virtual TfToken GetNodeType(const TfToken &nodeName) const = 0;

    /// Node type info is a collection of data related to the node type, often
    /// used to determine the node type.
    ///
    /// For now, we only have getters for this, as we aren't really intending on
    /// mutating this in any filter.
    virtual TfTokenVector
    GetNodeTypeInfoKeys(const TfToken& nodeName) const = 0;
    virtual VtValue
    GetNodeTypeInfoValue(const TfToken& nodeName, const TfToken& key) const = 0;

    virtual TfTokenVector GetAuthoredNodeParameterNames(
        const TfToken &nodeName) const = 0;
    
    virtual VtValue GetNodeParameterValue(
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

    virtual void SetNodeParameterValue(
        const TfToken &nodeName,
        const TfToken &paramName,
        const VtValue &value) = 0;

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
