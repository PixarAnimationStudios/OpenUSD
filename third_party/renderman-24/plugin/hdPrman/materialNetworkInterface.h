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
#ifndef EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATERIAL_NETWORK_INTERFACE_H
#define EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATERIAL_NETWORK_INTERFACE_H

#include "pxr/base/vt/value.h"
#include "pxr/base/tf/smallVector.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrmanMaterialNetworkInterface
///
/// Abstract interface for querying and mutating a material network.
///
/// This is useful for implementing matfilt functions which can be reused
/// by future scene index implementations.
class HdPrmanMaterialNetworkInterface
{
public:
    virtual ~HdPrmanMaterialNetworkInterface() = default;

    virtual TfTokenVector GetNodeNames() const  = 0;
    virtual TfToken GetNodeType(const TfToken &nodeName) const = 0;

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
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATERIAL_NETWORK_INTERFACE_H