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
#ifndef EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATFILT_RESOLVE_VSTRUCTS_H
#define EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATFILT_RESOLVE_VSTRUCTS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/materialNetworkInterface.h"
#include "pxr/usd/ndr/declare.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

/// Expands "virtual struct" connections for RenderMan.
/// If requested, conditional actions are evaluated.
void MatfiltResolveVstructs(
        HdMaterialNetworkInterface *networkInterface,
        bool enableConditions = true);

class MatfiltVstructConditionalEvaluatorImpl;

/// \class MatfiltVstructConditionalEvaluator
///
/// Parses and evaluates a single expression of "virtual struct conditional
/// grammar". This is used internally by MatfiltResolveVstructs but is
/// available to facilitate unit testing.
class MatfiltVstructConditionalEvaluator
{
public:
    typedef std::shared_ptr<MatfiltVstructConditionalEvaluator> Ptr;

    ~MatfiltVstructConditionalEvaluator();

    static Ptr Parse(const std::string &inputExpr);

    /// Runs the conditional actions specified by the parsed inputExpr.
    /// 
    /// Because this evaluates the conditional actions (connect, ignore,
    /// set constant, copy upstream parameter value), this is sent the context
    /// of the current connected nodes as well as the mutable network to
    /// directly change.
    /// 
    void Evaluate(
            const TfToken &nodeId,
            const TfToken &nodeInputId,
            const TfToken &upstreamNodeId,
            const TfToken &upstreamNodeOutput,
            const NdrTokenVec &shaderTypePriority,
            HdMaterialNetworkInterface *networkInterface) const;
private:
    MatfiltVstructConditionalEvaluator() = default;

    MatfiltVstructConditionalEvaluatorImpl *_impl = nullptr;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
