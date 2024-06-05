//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_LAYER_REGISTRY_H
#define PXR_USD_SDF_LAYER_REGISTRY_H

/// \file sdf/layerRegistry.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/assetPathResolver.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/base/tf/hash.h"

#include <string>
#include <unordered_map>
#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);

/// \class Sdf_LayerRegistry
///
/// A class that provides functionality to look up layers by asset path that
/// are tracked by the registry. Currently, when a new SdfLayer is created, it
/// is inserted into the layer registry. This allows SdfLayer::Find/FindOrOpen
/// to locate loaded layers.
///
class Sdf_LayerRegistry
{
    Sdf_LayerRegistry(const Sdf_LayerRegistry&) = delete;
    Sdf_LayerRegistry& operator=(const Sdf_LayerRegistry&) = delete;
public:
    /// Constructor.
    Sdf_LayerRegistry();

    /// Inserts layer into the registry
    void Insert(const SdfLayerHandle& layer, const Sdf_AssetInfo& assetInfo);

    /// Updates an existing registry entry if an entry is found for the same
    /// layer.
    void Update(const SdfLayerHandle& layer, const Sdf_AssetInfo& oldInfo,
                const Sdf_AssetInfo& newInfo);

    /// Erases the layer from the registry, if found.
    void Erase(const SdfLayerHandle& layer, const Sdf_AssetInfo& assetInfo);

    /// Returns a layer from the registry, searching first by identifier using
    /// FindByIdentifier, then by real path using FindByRealPath. If the layer
    /// cannot be found, a null layer handle is returned. If the \p layerPath
    /// is relative, it is made absolute by anchoring to the current working
    /// directory.
    SdfLayerHandle Find(const std::string &layerPath,
                        const std::string &resolvedPath=std::string()) const;

    /// Returns all valid layers held in the registry as a set.
    SdfLayerHandleSet GetLayers() const;

private:
    // Returns a layer from the registry, consulting the by_identifier index
    // with the \p layerPath as provided.
    SdfLayerHandle _FindByIdentifier(const std::string& layerPath) const;

    // Returns a layer from the registry, consulting the by_repository_path
    // index with the \p layerPath as provided.
    SdfLayerHandle _FindByRepositoryPath(const std::string& layerPath) const;
    
    // Returns a layer from the registry, consulting the by_real_path index.  If
    // \p layerPath is an absolute file system path, the index is searched using
    // the input path. Otherwise, \p layerPath is resolved and the resulting
    // path is used to search the index.
    SdfLayerHandle _FindByRealPath(
        const std::string& layerPath,
        const std::string& resolvedPath=std::string()) const;

    // A wrapper around a set of unordered_maps that maps layers
    // bidirectionally to their various string representations (realPath,
    // identifier, and repositoryPath)
    class _Layers final {
    public:
        _Layers() = default;
        using LayersByRealPath =
            std::unordered_map<std::string, SdfLayerHandle, TfHash>;
        using LayersByIdentifier =
            std::unordered_multimap<std::string, SdfLayerHandle, TfHash>;
        using LayersByRepositoryPath =
            std::unordered_multimap<std::string, SdfLayerHandle, TfHash>;

        const LayersByRealPath& ByRealPath() const { return _byRealPath; }
        const LayersByIdentifier& ByIdentifier() const {
            return _byIdentifier;
        }
        const LayersByRepositoryPath& ByRepositoryPath() const {
            return _byRepositoryPath;
        }

        // Insert the layer.
        // If the insertion is successful, return (the inserted layer, true)
        // If insertion is unsuccessful, return the layer that's occupying one
        // of the layer's entries and false.
        std::pair<SdfLayerHandle, bool> Insert(const SdfLayerHandle& layer,
                                               const Sdf_AssetInfo& assetInfo);

        // Update all the aliases (realPath, identifier, repositoryPath)
        // for this layer. The layer should already be stored in the container.
        // If a layer already occupies the newAssetInfo realPath slot, the
        // Update operation results in the layer being evicted from the
        // registry, leaving a "dangling layer" outside of registry in user
        // space. This is undesirable but matches legacy behavior.
        void Update(const SdfLayerHandle& layer,
                    const Sdf_AssetInfo& oldAssetInfo,
                    const Sdf_AssetInfo& newAssetInfo);

        // Remove this layer (and its aliases)
        bool Erase(const SdfLayerHandle& layer,
                   const Sdf_AssetInfo& assetInfo);

    private:
        LayersByRealPath _byRealPath;
        LayersByIdentifier _byIdentifier;
        LayersByRepositoryPath _byRepositoryPath;
    };

    _Layers _layers;
};

std::ostream&
operator<<(std::ostream& ostr, const Sdf_LayerRegistry& registry);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_LAYER_REGISTRY_H
