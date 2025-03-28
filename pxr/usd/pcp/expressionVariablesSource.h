//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PCP_EXPRESSION_VARIABLES_SOURCE_H
#define PXR_USD_PCP_EXPRESSION_VARIABLES_SOURCE_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class PcpCache;
class PcpLayerStackIdentifier;

/// \class PcpExpressionVariablesSource
///
/// Represents the layer stack associated with a set of expression variables.
/// This is typically a simple PcpLayerStackIdentifier.
class PcpExpressionVariablesSource
{
public:
    /// Create a PcpExpressionVariableSource representing the root layer stack
    /// of a prim index.
    PCP_API
    PcpExpressionVariablesSource();

    /// Create a PcpExpressionVariableSource representing the layer stack
    /// identified by \p layerStackIdentifier. If \p layerStackIdentifier
    /// is equal to \p rootLayerStackIdentifier, this is the same as
    /// the default constructor.
    PCP_API
    PcpExpressionVariablesSource(
        const PcpLayerStackIdentifier& layerStackIdentifier,
        const PcpLayerStackIdentifier& rootLayerStackIdentifier);

    PCP_API
    ~PcpExpressionVariablesSource();

    /// \name Comparison Operators
    /// @{
    PCP_API
    bool operator==(const PcpExpressionVariablesSource& rhs) const;

    PCP_API 
    bool operator!=(const PcpExpressionVariablesSource& rhs) const;

    PCP_API 
    bool operator<(const PcpExpressionVariablesSource& rhs) const;
    /// @}

    /// Return hash value for this object.
    PCP_API
    size_t GetHash() const;

    /// Return true if this object represents a prim index's root
    /// layer stack, false otherwise. If this function returns true,
    /// GetLayerStackIdentifier will return nullptr.
    bool IsRootLayerStack() const
    {
        return !static_cast<bool>(_identifier);
    }

    /// Return the identifier of the layer stack represented by this
    /// object if it is not the root layer stack. Return nullptr if
    /// this object represents the root layer stack (i.e., IsRootLayerStack
    /// returns true).
    const PcpLayerStackIdentifier* GetLayerStackIdentifier() const
    {
        return _identifier ? _identifier.get() : nullptr;
    }

    /// Convenience function to return the identifier of the layer
    /// stack represented by this object. If this object represents
    /// the root layer stack, return \p rootLayerStackIdentifier,
    /// otherwise return *GetLayerStackIdentifier().
    PCP_API
    const PcpLayerStackIdentifier& ResolveLayerStackIdentifier(
        const PcpLayerStackIdentifier& rootLayerStackIdentifier) const;

    /// \overload
    /// Equivalent to ResolveLayerStackIdentifier(cache.GetLayerStackIdentifier())
    PCP_API
    const PcpLayerStackIdentifier& ResolveLayerStackIdentifier(
        const PcpCache& cache) const;

    // Avoid possibly returning a const-reference to a temporary.
    const PcpLayerStackIdentifier& ResolveLayerStackIdentifier(
        PcpLayerStackIdentifier&&) const = delete;
    const PcpLayerStackIdentifier& ResolveLayerStackIdentifier(
        PcpCache&&) const = delete;

private:
    // The identifier of the layer stack providing the associated
    // expression variables. A null value indicates the root layer stack.
    std::shared_ptr<PcpLayerStackIdentifier> _identifier;
};

template <typename HashState>
void
TfHashAppend(HashState& h, const PcpExpressionVariablesSource& x)
{
    h.Append(x.GetHash());
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
