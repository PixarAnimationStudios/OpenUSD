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
#ifndef EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_HD_MATERIAL_NETWORK_2_INTERFACE_H
#define EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_HD_MATERIAL_NETWORK_2_INTERFACE_H

#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/materialNetworkInterface.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrmanHdMaterialNetwork2Interface
///
/// Implements HdMaterialNetworkInterface interface backed by an
/// HdMaterialNetwork2 -- which is useful for implementing matfilt functions
/// without being tied to the legacy data model
class HdPrmanHdMaterialNetwork2Interface
    : public HdMaterialNetworkInterface
{
public:

    HdPrmanHdMaterialNetwork2Interface(HdMaterialNetwork2 *materialNetwork)
    : _materialNetwork(materialNetwork)
    , _lastAccessedNode(nullptr)
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

    void DeleteTerminal(const TfToken &terminalName) override;

    void SetTerminalConnection(
        const TfToken &terminalName,
        const InputConnection &connection) override;

private:
    HdMaterialNetwork2 *_materialNetwork;
    mutable TfToken _lastAccessedNodeName;
    mutable HdMaterialNode2 *_lastAccessedNode;

    HdMaterialNode2 *_GetNode(const TfToken &nodeName) const;
    HdMaterialNode2 *_GetOrCreateNode(const TfToken &nodeName) const;
    
};



PXR_NAMESPACE_CLOSE_SCOPE

#endif //EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_HD_MATERIAL_NETWORK_2_INTERFACE_H
