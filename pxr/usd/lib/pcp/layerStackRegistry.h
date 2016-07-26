//
// Copyright 2016 Pixar
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
/// \file pcp/layerStackRegistry.h

#ifndef PCP_LAYER_STACK_REGISTRY_H
#define PCP_LAYER_STACK_REGISTRY_H

#include "pxr/usd/pcp/errors.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refBase.h"
#include <boost/scoped_ptr.hpp>

TF_DECLARE_WEAK_AND_REF_PTRS(PcpLayerStack);
TF_DECLARE_REF_PTRS(Pcp_LayerStackRegistry);

class PcpLayerStackIdentifier;
class Pcp_LayerStackRegistryData;
class Pcp_MutedLayers;

/// \class Pcp_LayerStackRegistry
/// \brief A registry of layer stacks
///
class Pcp_LayerStackRegistry : public TfRefBase, public TfWeakBase {
public:
    /// Create a new Pcp_LayerStackRegistry.
    static Pcp_LayerStackRegistryRefPtr New(
        const std::string& targetSchema = std::string(),
        bool isUsd=false);

    /// Adds layers specified in \p layersToMute and removes layers
    /// specified in \p layersToUnmute from the registry's set of muted
    /// layers.  Any relative paths will be anchored to the given 
    /// \p anchorLayer. On completion, \p layersToMute and \p layersToUnmute 
    /// will be filled with the canonical identifiers for layers that were 
    /// actually added or removed.
    void MuteAndUnmuteLayers(const SdfLayerHandle& anchorLayer,
                             std::vector<std::string>* layersToMute,
                             std::vector<std::string>* layersToUnmute);

    /// Returns the list of canonical identifiers for muted layers
    /// in this cache.
    const std::vector<std::string>& GetMutedLayers() const;

    /// Returns true if the layer identified by \p layerIdentifier is muted,
    /// false otherwise.  If \p layerIdentifier is relative, \p anchorLayer
    /// used to anchor the layer.  If this function returns true,
    /// \p canonicalLayerIdentifier will be populated with the canonical
    /// identifier for the muted layer.
    bool IsLayerMuted(const SdfLayerHandle& anchorLayer,
                      const std::string& layerIdentifier,
                      std::string* canonicalLayerIdentifier = nullptr) const;

    /// Returns the layer stack for \p identifier if it exists, otherwise
    /// creates a new layer stack for \p identifier.  This returns \c NULL
    /// if \p identifier is invalid (i.e. its root layer is \c NULL).
    PcpLayerStackRefPtr FindOrCreate(const PcpLayerStackIdentifier& identifier,
                                     PcpErrorVector *allErrors);

    /// Returns the layer stack for \p identifier if it exists, otherwise
    /// returns \c NULL.
    PcpLayerStackPtr Find(const PcpLayerStackIdentifier&) const;

    /// Returns every layer stack that includes \p layer.
    const PcpLayerStackPtrVector&
    FindAllUsingLayer(const SdfLayerHandle& layer) const;

    /// Returns every layer stack that uses the muted layer identified
    /// \p layerId, which is assumed to be a canonical muted layer
    /// identifier.
    const PcpLayerStackPtrVector&
    FindAllUsingMutedLayer(const std::string& layerId) const;

    /// Returns every layer stack known to this registry.
    std::vector<PcpLayerStackPtr> GetAllLayerStacks() const;

private:
    /// Private constructor -- see New().
    Pcp_LayerStackRegistry(const std::string& targetSchema,
                           bool isUsd);
    ~Pcp_LayerStackRegistry();

    // Remove the layer stack with the given identifier from the registry.
    void _Remove(const PcpLayerStackIdentifier&,
                 const PcpLayerStack *);

    // Update the layer-stack-by-layer maps by setting the layers for the
    // given layer stack.
    void _SetLayers(const PcpLayerStack*);

    // Returns the target schema for layer stacks managed by this
    // registry.
    const std::string& _GetTargetSchema() const;

    // Returns whether or not we are in USD mode for avoiding
    // extra calls such as Pcp_ComputeRelocationForLayerStack()
    bool _IsUsd() const;

    // Returns the muted layer collection so that layer stack
    // computation can easily query whether a layer is muted.
    const Pcp_MutedLayers& _GetMutedLayers() const;

    // PcpLayerStack can access private _GetTargetSchema(), 
    // _Remove(), and _SetLayers().
    friend class PcpLayerStack;

private:
    boost::scoped_ptr<Pcp_LayerStackRegistryData> _data;
};

/// \class Pcp_MutedLayers
/// Helper for maintaining and querying a collection of muted layers.
class Pcp_MutedLayers
{
public:
    const std::vector<std::string>& GetMutedLayers() const;
    void MuteAndUnmuteLayers(const SdfLayerHandle& anchorLayer,
                             std::vector<std::string>* layersToMute,
                             std::vector<std::string>* layersToUnmute);
    bool IsLayerMuted(const SdfLayerHandle& anchorLayer,
                      const std::string& layerIdentifier,
                      std::string* canonicalLayerIdentifier = nullptr) const;

private:
    std::vector<std::string> _layers;
};

#endif
