//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PCP_EXPRESSION_VARIABLES_H
#define PXR_USD_PCP_EXPRESSION_VARIABLES_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"
#include "pxr/usd/pcp/expressionVariablesSource.h"

#include "pxr/base/vt/dictionary.h"

#include <utility>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

/// \class PcpExpressionVariables
///
/// Object containing composed expression variables associated with a given
/// layer stack, identified by a PcpExpressionVariablesSource.
class PcpExpressionVariables
{
public:
    /// Compute the composed expression variables for \p sourceLayerStackId,
    /// recursively computing and composing the overrides specified by
    /// its expressionVariableOverridesSource. If \p overrideExpressionVars is
    /// provided, it will be used as the overrides instead of performing
    /// the recursive computation.
    PCP_API
    static PcpExpressionVariables
    Compute(
        const PcpLayerStackIdentifier& sourceLayerStackId,
        const PcpLayerStackIdentifier& rootLayerStackId,
        const PcpExpressionVariables* overrideExpressionVars = nullptr);

    /// Create a new object with no expression variables and the source set
    /// to the root layer stack.
    PcpExpressionVariables() = default;

    /// Creates a new  object for \p source with the given
    /// \p expressionVariables.
    PcpExpressionVariables(
        const PcpExpressionVariablesSource& source,
        const VtDictionary& expressionVariables)
        : PcpExpressionVariables(
            PcpExpressionVariablesSource(source),
            VtDictionary(expressionVariables))
    { }

    /// Creates a new object for \p source with the given
    /// \p expressionVariables.
    PcpExpressionVariables(
        PcpExpressionVariablesSource&& source,
        VtDictionary&& expressionVariables)
        : _source(std::move(source))
        , _expressionVariables(std::move(expressionVariables))
    { }

    /// \name Comparison Operators
    /// @{
    bool operator==(const PcpExpressionVariables& rhs) const
    { 
        return (this == &rhs) || 
        (std::tie(_source, _expressionVariables) == 
         std::tie(rhs._source, rhs._expressionVariables));
    }

    bool operator!=(const PcpExpressionVariables& rhs) const
    { 
        return !(*this == rhs);
    }
    /// @}

    /// Return the source of the composed expression variables.
    const PcpExpressionVariablesSource& GetSource() const
    { return _source; }

    /// Returns the composed expression variables dictionary.
    const VtDictionary& GetVariables() const
    { return _expressionVariables; }

    /// Set the composed expression variables to \p variables.
    void SetVariables(const VtDictionary& variables)
    { _expressionVariables = variables; }

private:
    PcpExpressionVariablesSource _source;
    VtDictionary _expressionVariables;
};

// ------------------------------------------------------------

/// \class PcpExpressionVariableCachingComposer
///
/// Helper object for computing PcpExpressionVariable objects. This gives the
/// same results as PcpExpressionVariables::Compute, but caches the results
/// of the recursive override computations so they can be reused by 
/// subsequent computations.
class PcpExpressionVariableCachingComposer
{
public:
    PCP_API
    explicit PcpExpressionVariableCachingComposer(
        const PcpLayerStackIdentifier& rootLayerStackIdentifier);

    /// Compute the composed expression variables for the layer stack with
    /// the given \p id. This will recursively compute the overriding
    /// expression variables specified in \p id.
    PCP_API
    const PcpExpressionVariables&
    ComputeExpressionVariables(const PcpLayerStackIdentifier& id);

private:
    PcpLayerStackIdentifier _rootLayerStackId;

    using _IdentifierToExpressionVarsMap = std::unordered_map<
        PcpLayerStackIdentifier, PcpExpressionVariables, TfHash>;
    _IdentifierToExpressionVarsMap _identifierToExpressionVars;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
