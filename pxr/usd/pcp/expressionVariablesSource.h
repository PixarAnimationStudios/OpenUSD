//
// Copyright 2023 Pixar
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
