//
// Copyright 2019 Pixar
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
#ifndef EXT_RMANPKG_23_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATFILT_FILTER_CHAIN_H
#define EXT_RMANPKG_23_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATFILT_FILTER_CHAIN_H

#include "pxr/pxr.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/ndr/declare.h"
#include <map>

PXR_NAMESPACE_OPEN_SCOPE

/// \struct MatfiltConnection
///
/// Describes a single connection to an upsream node and output port 
struct MatfiltConnection {
    SdfPath upstreamNode;
    TfToken upstreamOutputName;

    bool operator==(const MatfiltConnection & rhs) const {
        return upstreamNode == rhs.upstreamNode
                && upstreamOutputName == rhs.upstreamOutputName;
    }
};

/// \struct MatfiltNode
///
/// Describes an instance of a node within a network
/// A node contains a (shader) type identifier, parameter values, and 
/// connections to upstream nodes. A single input (mapped by TfToken) may have
/// multiple upstream connections to describe connected array elements.
struct MatfiltNode {
    TfToken nodeTypeId;
    std::map<TfToken, VtValue> parameters;
    std::map<TfToken, std::vector<MatfiltConnection>> inputConnections;

    bool operator==(const MatfiltNode & rhs) const {
        return nodeTypeId == rhs.nodeTypeId
                && parameters == rhs.parameters
                && inputConnections == rhs.inputConnections;
    }
};

/// \struct MatfiltNetwork
/// 
/// Container of nodes and top-level terminal connections. This is the mutable
/// representation of a shading network sent to filtering functions by a
/// MatfiltFilterChain.
struct MatfiltNetwork {
    std::map<SdfPath, MatfiltNode> nodes;
    std::map<TfToken, MatfiltConnection> terminals;

    bool operator==(const MatfiltNetwork & rhs) const {
        return nodes == rhs.nodes && terminals == rhs.terminals;
    }
};

/// A function which manipulates a shading network for a given context.
typedef void (*MatfiltFilterFnc)
    (const SdfPath & networkId,
     MatfiltNetwork & network,
     const std::map<TfToken, VtValue> & contextValues,
     const NdrTokenVec & shaderTypePriority,
     std::vector<std::string> * outputErrorMessages);

/// A sequence of material filter functions.
typedef std::vector<MatfiltFilterFnc> MatfiltFilterChain;

/// Executes the sequence of material filtering functions.
///
/// \p networkId is an identifier representing the entire network. It is
/// useful as a parent scope for any newly-created nodes in the filtered
/// network.
///
/// \p network is a reference to a mutable network on which the filtering
/// functions operate on in sequence.
///
/// \p contextValues is a map of named values which is useful either as
/// configuration input to the filtering functions. One example might be
/// to provide values to a filtering function which does subsitutions on
/// string values like $MODEL.
///
/// \p shaderTypePriority provides context to a filtering function which
/// may make use of ndr or sdr to query information about the shader of a
/// given node in the network. It is typically host/renderer-dependent.
///
/// \p outputErrorMessages is an optional vector to which filter functions
/// may write error messages.
void MatfiltExecFilterChain(
    MatfiltFilterChain const& filterChain,
    const SdfPath & networkId,
    MatfiltNetwork & network,
    const std::map<TfToken, VtValue> & contextValues,
    const NdrTokenVec & shaderTypePriority,
    std::vector<std::string> * outputErrorMessages = nullptr);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
