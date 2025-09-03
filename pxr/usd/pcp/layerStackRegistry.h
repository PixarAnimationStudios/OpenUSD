//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PCP_LAYER_STACK_REGISTRY_H
#define PXR_USD_PCP_LAYER_STACK_REGISTRY_H

/// \file pcp/layerStackRegistry.h

#include "pxr/pxr.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/functionRef.h"
#include "pxr/base/tf/refBase.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(PcpLayerStack);
TF_DECLARE_REF_PTRS(Pcp_LayerStackRegistry);

class PcpLayerStackIdentifier;
class Pcp_LayerStackRegistryData;
class Pcp_MutedLayers;

/// \class Pcp_LayerStackRegistry
///
/// A registry of layer stacks.
///
class Pcp_LayerStackRegistry : public TfRefBase, public TfWeakBase {
public:
    /// Create a new Pcp_LayerStackRegistry.
    static Pcp_LayerStackRegistryRefPtr New(
        const PcpLayerStackIdentifier& rootLayerStackIdentifier,
        const std::string& fileFormatTarget = std::string(),
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

    /// Return true if this registry contains \p layerStack, false otherwise.
    bool Contains(const PcpLayerStackPtr &layerStack) const;

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

    /// Runs \p fn on all layer stacks known to this registry.
    void 
    ForEachLayerStack(const TfFunctionRef<void(const PcpLayerStackPtr&)>& fn);

private:
    /// Private constructor -- see New().
    Pcp_LayerStackRegistry(const PcpLayerStackIdentifier& rootLayerStackId,
                           const std::string& fileFormatTarget,
                           bool isUsd);
    ~Pcp_LayerStackRegistry();

    // Find that doesn't lock.
    PcpLayerStackPtr _Find(const PcpLayerStackIdentifier&) const;

    // Remove the layer stack with the given identifier from the registry.
    void _SetLayersAndRemove(const PcpLayerStackIdentifier&,
                             const PcpLayerStack *);

    // Update the layer-stack-by-layer maps by setting the layers for the
    // given layer stack.
    void _SetLayers(const PcpLayerStack*);

    // Returns the identifier of the root layer stack associated with
    // this registry.
    const PcpLayerStackIdentifier& _GetRootLayerStackIdentifier() const;

    // Returns the file format target for layer stacks managed by this
    // registry.
    const std::string& _GetFileFormatTarget() const;

    // Returns whether or not we are in USD mode for avoiding
    // extra calls such as Pcp_ComputeRelocationForLayerStack()
    bool _IsUsd() const;

    // Returns the muted layer collection so that layer stack
    // computation can easily query whether a layer is muted.
    const Pcp_MutedLayers& _GetMutedLayers() const;

    // PcpLayerStack can access private _GetFileFormatTarget(), 
    // _SetLayersAndRemove(), and _SetLayers().
    friend class PcpLayerStack;

private:
    std::unique_ptr<Pcp_LayerStackRegistryData> _data;
};

/// \class Pcp_MutedLayers
///
/// Helper for maintaining and querying a collection of muted layers.
///
class Pcp_MutedLayers
{
public:
    explicit Pcp_MutedLayers(const std::string& fileFormatTarget);

    const std::vector<std::string>& GetMutedLayers() const;
    void MuteAndUnmuteLayers(const SdfLayerHandle& anchorLayer,
                             std::vector<std::string>* layersToMute,
                             std::vector<std::string>* layersToUnmute);
    bool IsLayerMuted(const SdfLayerHandle& anchorLayer,
                      const std::string& layerIdentifier,
                      std::string* canonicalLayerIdentifier = nullptr) const;

private:
    std::string 
    _GetCanonicalLayerId(const SdfLayerHandle& anchorLayer, 
                         const std::string& layerId) const;

    std::string _fileFormatTarget;
    std::vector<std::string> _layers;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_LAYER_STACK_REGISTRY_H
