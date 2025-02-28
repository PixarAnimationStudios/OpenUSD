//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATFILT_RESOLVE_VSTRUCTS_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATFILT_RESOLVE_VSTRUCTS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/materialNetworkInterface.h"

#if PXR_VERSION >= 2505
#include "pxr/usd/sdr/declare.h"
#else
#include "pxr/usd/ndr/declare.h"
#endif

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

#if PXR_VERSION < 2505
using SdrTokenVec = NdrTokenVec;
#endif

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
            const SdrTokenVec &shaderTypePriority,
            HdMaterialNetworkInterface *networkInterface) const;
private:
    MatfiltVstructConditionalEvaluator() = default;

    MatfiltVstructConditionalEvaluatorImpl *_impl = nullptr;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATFILT_RESOLVE_VSTRUCTS_H
