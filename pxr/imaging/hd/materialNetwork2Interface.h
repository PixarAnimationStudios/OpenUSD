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
#ifndef HD_MATERIAL_NETWORK_2_INTERFACE_H
#define HD_MATERIAL_NETWORK_2_INTERFACE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/materialNetworkInterface.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdMaterialNetwork2Interface
///
/// Implements HdMaterialNetworkInterface interface backed by an
/// HdMaterialNetwork2 -- which is useful for implementing material
/// filtering functions without being tied to the legacy data model
class HdMaterialNetwork2Interface
    : public HdMaterialNetworkInterface
{
public:

    HdMaterialNetwork2Interface(
        const SdfPath &materialPrimPath,
        HdMaterialNetwork2 *materialNetwork)
    : _materialPrimPath(materialPrimPath)
    , _materialNetwork(materialNetwork)
    , _lastAccessedNode(nullptr)
    {}

    HD_API
    SdfPath GetMaterialPrimPath() const override {
        return _materialPrimPath;
    }

    HD_API
    std::string GetModelAssetName() const override {
        return std::string();
    }

    HD_API
    TfTokenVector GetNodeNames() const override;

    HD_API
    TfToken GetNodeType(const TfToken &nodeName) const override;

    HD_API
    TfTokenVector GetNodeTypeInfoKeys(const TfToken& nodeName) const override;

    HD_API
    VtValue GetNodeTypeInfoValue(
        const TfToken& nodeName, const TfToken& key) const override;

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
    void DeleteTerminal(const TfToken &terminalName) override;

    HD_API
    void SetTerminalConnection(
        const TfToken &terminalName,
        const InputConnection &connection) override;

private:
    SdfPath _materialPrimPath;
    HdMaterialNetwork2 *_materialNetwork;
    mutable TfToken _lastAccessedNodeName;
    mutable HdMaterialNode2 *_lastAccessedNode;

    HdMaterialNode2 *_GetNode(const TfToken &nodeName) const;
    HdMaterialNode2 *_GetOrCreateNode(const TfToken &nodeName) const;
    
};



PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_MATERIAL_NETWORK_2_INTERFACE_H
