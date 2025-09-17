//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file Sdf/LayerRegistry.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/layerRegistry.h"
#include "pxr/usd/sdf/assetPathResolver.h"
#include "pxr/usd/sdf/debugCodes.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/layer.h"
//#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticData.h"
#include <ostream>

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

// A simple layer repr, used for debug and error messages, that includes both
// the identifier and the real path.
static string
Sdf_LayerDebugRepr(
    const SdfLayerHandle& layer)
{
    return layer ?
        "SdfLayer('" +
            layer->GetIdentifier() + "', '" +
            layer->GetRealPath() + "')" :
        "None"
        ;
}

struct Sdf_RegistryAliases {
    std::string identifier;
    std::string repositoryPath;
    std::string realPath;
};

static Sdf_RegistryAliases
_AssetInfoToAliases(const Sdf_AssetInfo& assetInfo) {
    std::string identifierSansArguments, arguments;
    TF_VERIFY(Sdf_SplitIdentifier(
              assetInfo.identifier, &identifierSansArguments, &arguments));
    // The identifier cannot be empty. If it is, GetLayers() will not function
    // correctly.
    TF_VERIFY(!assetInfo.identifier.empty());
    return {
        assetInfo.identifier,
        assetInfo.assetInfo.repoPath.empty() ? "" :
            Sdf_CreateIdentifier(assetInfo.assetInfo.repoPath, arguments),
        assetInfo.resolvedPath.empty() ? "" :
            Sdf_CreateIdentifier(assetInfo.resolvedPath, arguments)
    };
}

Sdf_LayerRegistry::Sdf_LayerRegistry()
{
}

// Ideally, the lifetime of a layer should be synchronized with the registry.
// However--
//    a) Update operations can result in a "dangling layer" where a layer
//       is evicted from the registry even though a user still retains
//       a handle.
//    b) There's a known race in expiring layers where a handle is evicted
//       before the destructor completes.
// For those two reasons _TryToRemove must "try" to remove and not error
// or warn if an expected key is missing.
static bool
_TryToRemove(const std::string& key, const SdfLayerHandle& layer,
             std::unordered_map<std::string, SdfLayerHandle, TfHash>* map) {
    if (!key.empty()) {
        if (const auto it = map->find(key);
            it != std::end(*map) && it->second == layer) {
            map->erase(it);
            return true;
        }
    }
    return false;
}

static bool
_TryToRemove(
    const std::string& key, const SdfLayerHandle& layer,
    std::unordered_multimap<std::string, SdfLayerHandle, TfHash>* map) {
    const auto range = map->equal_range(key);
    if (const auto it = std::find_if(range.first, range.second,
                                [&layer](const auto& entry){
                                    return entry.second == layer;
                                }); it != range.second) {
        map->erase(it);
        return true;
    }
    return false;
}

void
Sdf_LayerRegistry::_Layers::Update(const SdfLayerHandle& layer,
                                   const Sdf_AssetInfo& oldInfo,
                                   const Sdf_AssetInfo& newInfo) {
    const auto oldAliases = _AssetInfoToAliases(oldInfo);
    auto newAliases = _AssetInfoToAliases(newInfo);
    if (oldAliases.realPath != newAliases.realPath) {
        if (_TryToRemove(oldAliases.realPath, layer, &_byRealPath)) {
            TF_DEBUG(SDF_LAYER).Msg("Removed realPath '%s' for update.\n",
                                    oldAliases.realPath.c_str());
        }
        if (!newAliases.realPath.empty()) {
            if (const auto insertion = _byRealPath.emplace(
                    newAliases.realPath, layer);
                insertion.second) {
                TF_DEBUG(SDF_LAYER).Msg("Updated realPath '%s'.\n",
                                        newAliases.realPath.c_str());
            }
            else {
                // It is uncommon but possible for two distinct handles to have
                // the same real path. If an update operation is going to
                // generate a dangling layer, then ensure the identifier and
                // repository path entries are removed as well by setting the
                // identifier and repository path to the empty string so that
                // their entries in the repository and identifier maps will be
                // removed.
                newAliases.repositoryPath = "";
                newAliases.identifier = "";
                TF_DEBUG(SDF_LAYER).Msg("Updated realPath '%s' would create "
                                        "collision. Dangling layer created "
                                        "instead.\n",
                                        newAliases.realPath.c_str());
            }
        }
    }
    if (oldAliases.repositoryPath != newAliases.repositoryPath) {
        if (_TryToRemove(oldAliases.repositoryPath, layer,
                         &_byRepositoryPath)) {
            TF_DEBUG(SDF_LAYER).Msg("Removed repositoryPath '%s' for "
                                    "update.\n",
                                    oldAliases.repositoryPath.c_str());
        }
        if (!newAliases.repositoryPath.empty()) {
            _byRepositoryPath.emplace(newAliases.repositoryPath, layer);
            TF_DEBUG(SDF_LAYER).Msg("Updated repositoryPath '%s'.\n",
                                    newAliases.repositoryPath.c_str());
        }
    }
    if (oldAliases.identifier != newAliases.identifier) {
        if (_TryToRemove(oldAliases.identifier, layer,
                         &_byIdentifier)) {
            TF_DEBUG(SDF_LAYER).Msg("Removed identifier '%s' for "
                                    "update.\n",
                                    oldAliases.identifier.c_str());
        }
        if (!newAliases.identifier.empty()) {
            _byIdentifier.emplace(newAliases.identifier, layer);
            TF_DEBUG(SDF_LAYER).Msg("Updated identifier '%s'.\n",
                                    newAliases.identifier.c_str());
        }
    }
}

std::pair<SdfLayerHandle, bool>
Sdf_LayerRegistry::_Layers::Insert(const SdfLayerHandle& layer,
                                   const Sdf_AssetInfo& assetInfo) {
    const auto aliases = _AssetInfoToAliases(assetInfo);
    if (const auto it = _byRealPath.find(aliases.realPath);
        it != std::end(_byRealPath)) {
        return std::make_pair(it->second, false);
    }
    if (!aliases.realPath.empty()) {
        TF_VERIFY(_byRealPath.emplace(aliases.realPath, layer).second);
        TF_DEBUG(SDF_LAYER).Msg("Inserted realPath '%s' into registry\n",
                                aliases.realPath.c_str());
    }
    if (!aliases.repositoryPath.empty()) {
        _byRepositoryPath.emplace(aliases.repositoryPath, layer);
        TF_DEBUG(SDF_LAYER).Msg("Inserted repositoryPath '%s' into registry\n",
                                aliases.repositoryPath.c_str());
    }
    if (!aliases.identifier.empty()) {
        _byIdentifier.emplace(aliases.identifier, layer);
        TF_DEBUG(SDF_LAYER).Msg("Inserted identifier '%s' into registry\n",
                                aliases.identifier.c_str());
    }
    return std::make_pair(layer, true);
}

bool
Sdf_LayerRegistry::_Layers::Erase(const SdfLayerHandle& layer,
                                  const Sdf_AssetInfo& assetInfo) {
    const auto aliases = _AssetInfoToAliases(assetInfo);
    // Track whether any entries were actually erased. In general,
    // It's the responsibility of the layer destructor to erase
    // the entry from the registry, but trying to acquire an expiring
    // layer may cause this eviction to happen early. This isn't an
    // error, but we do want to track successful erases for TF_DEBUG
    // info. Additionally, Update in rare circumstances could create
    // a real path collisions due to asset resolver context updates
    // and may lead to an early eviction of a layer from the registry.
    bool erased = false;
    if (_TryToRemove(aliases.realPath, layer, &_byRealPath)) {
        erased = true;
        TF_DEBUG(SDF_LAYER).Msg("Erased realPath '%s' from registry.\n",
                                aliases.realPath.c_str());
    }
    if (_TryToRemove(aliases.repositoryPath, layer,
                     &_byRepositoryPath)) {
        erased = true;
        TF_DEBUG(SDF_LAYER).Msg(
            "Erased repositoryPath '%s' from registry.\n",
            aliases.repositoryPath.c_str());
    }
    if (_TryToRemove(aliases.identifier, layer,
                     &_byIdentifier)) {
        erased = true;
        TF_DEBUG(SDF_LAYER).Msg(
            "Erased identifier '%s' from registry.\n",
            aliases.repositoryPath.c_str());
    }
    return erased;
}

void
Sdf_LayerRegistry::Insert(
    const SdfLayerHandle& layer, const Sdf_AssetInfo& assetInfo)
{
    TRACE_FUNCTION();

    if (!layer) {
        TF_CODING_ERROR("Expired layer handle");
        return;
    }

    TF_DEBUG(SDF_LAYER).Msg(
        "Sdf_LayerRegistry::Insert(%s)\n",
        Sdf_LayerDebugRepr(layer).c_str());

    // Attempt to insert the layer into the registry.
    std::pair<SdfLayerHandle, bool> result = _layers.Insert(layer, assetInfo);
    if (!result.second) {
        // We failed to insert the layer into the registry because there
        // is a realPath conflict. This can happen when the same layer is
        // created twice in the same location in the same session.
        TF_CODING_ERROR("Cannot insert duplicate registry entry for "
            "%s layer %s over existing entry for %s layer %s",
            layer->GetFileFormat()->GetFormatId().GetText(),
            Sdf_LayerDebugRepr(layer).c_str(),
            result.first->GetFileFormat()->GetFormatId().GetText(),
            Sdf_LayerDebugRepr(result.first).c_str());
    }
}

void
Sdf_LayerRegistry::Update(
    const SdfLayerHandle& layer,
    const Sdf_AssetInfo& oldInfo,
    const Sdf_AssetInfo& newInfo)
{
    TRACE_FUNCTION();

    if (!layer) {
        TF_CODING_ERROR("Expired layer handle");
        return;
    }

    TF_DEBUG(SDF_LAYER).Msg(
        "Sdf_LayerRegistry::Update(%s)\n",
        Sdf_LayerDebugRepr(layer).c_str());

    _layers.Update(layer, oldInfo, newInfo);
}

void
Sdf_LayerRegistry::Erase(
    const SdfLayerHandle& layer,
    const Sdf_AssetInfo& assetInfo)
{
    bool erased = _layers.Erase(layer, assetInfo);

    TF_DEBUG(SDF_LAYER).Msg(
        "Sdf_LayerRegistry::Erase(%s) => %s\n",
        Sdf_LayerDebugRepr(layer).c_str(),
        erased ? "Success" : "Failed");
}

SdfLayerHandle
Sdf_LayerRegistry::Find(
    const string &inputLayerPath,
    const string &resolvedPath) const
{
    TRACE_FUNCTION();

    SdfLayerHandle foundLayer;

    if (Sdf_IsAnonLayerIdentifier(inputLayerPath)) {
        foundLayer = _FindByIdentifier(inputLayerPath);
    } else {
        ArResolver& resolver = ArGetResolver();

        const string& layerPath = inputLayerPath;

        // If the layer path depends on context there may be multiple
        // layers with the same identifier but different resolved paths.
        // In this case we need to look up the layer by resolved path.
        string assetPath, args;
        Sdf_SplitIdentifier(inputLayerPath, &assetPath, &args);
        if (!resolver.IsContextDependentPath(assetPath)) {
            foundLayer = _FindByIdentifier(layerPath);
        }

        // If the layer path is in repository form and we haven't yet
        // found the layer via the identifier, attempt to look up the
        // layer by repository path.
        const bool isRepositoryPath = resolver.IsRepositoryPath(assetPath);
        if (!foundLayer && isRepositoryPath) {
            foundLayer = _FindByRepositoryPath(layerPath);
        }

        // If the layer has not yet been found, this may be some other
        // form of path that requires path resolution and lookup in the
        // real path index in order to locate.
        if (!foundLayer) {
            foundLayer = _FindByRealPath(layerPath, resolvedPath);
        }
    }

    TF_DEBUG(SDF_LAYER).Msg(
        "Sdf_LayerRegistry::Find('%s') => %s\n",
        inputLayerPath.c_str(),
        Sdf_LayerDebugRepr(foundLayer).c_str());

    return foundLayer;
}

SdfLayerHandle
Sdf_LayerRegistry::_FindByIdentifier(
    const string& layerPath) const
{
    TRACE_FUNCTION();

    SdfLayerHandle foundLayer;
    const auto& byIdentifier = _layers.ByIdentifier();
    const auto identifierIt = byIdentifier.find(layerPath);
    if (identifierIt != byIdentifier.end()) {
        foundLayer = identifierIt->second;
    }

    TF_DEBUG(SDF_LAYER).Msg(
        "Sdf_LayerRegistry::_FindByIdentifier('%s') => %s\n",
        layerPath.c_str(),
        foundLayer ? "Found" : "Not Found");

    return foundLayer;
}

SdfLayerHandle
Sdf_LayerRegistry::_FindByRepositoryPath(
    const string& layerPath) const
{
    TRACE_FUNCTION();

    SdfLayerHandle foundLayer;

    if (layerPath.empty())
        return foundLayer;

    const auto& byRepoPath = _layers.ByRepositoryPath();
    const auto repoPathIt = byRepoPath.find(layerPath);
    if (repoPathIt != byRepoPath.end()) {
        foundLayer = repoPathIt->second;
    }

    TF_DEBUG(SDF_LAYER).Msg(
        "Sdf_LayerRegistry::_FindByRepositoryPath('%s') => %s\n",
        layerPath.c_str(),
        foundLayer ? "Found" : "Not Found");

    return foundLayer;
}

SdfLayerHandle
Sdf_LayerRegistry::_FindByRealPath(
    const string& layerPath,
    const string& resolvedPath) const
{
    TRACE_FUNCTION();

    SdfLayerHandle foundLayer;

    if (layerPath.empty())
        return foundLayer;

    string searchPath, arguments;
    if (!Sdf_SplitIdentifier(layerPath, &searchPath, &arguments))
        return foundLayer;

    // Ignore errors reported by Sdf_ComputeFilePath. These errors mean we
    // weren't able to compute a real path from the given layerPath. However,
    // that shouldn't be an error for this function, it just means there was
    // nothing to find at that layerPath.
    {
        TfErrorMark m;
        searchPath = !resolvedPath.empty() ?
            resolvedPath : Sdf_ComputeFilePath(searchPath);

        if (!m.IsClean()) {
            std::vector<std::string> errors;
            for (const TfError& e : m) {
                errors.push_back(e.GetCommentary());
            }

            TF_DEBUG(SDF_LAYER).Msg(
                "Sdf_LayerRegistry::_FindByRealPath('%s'): "
                "Failed to compute real path: %s\n",
                layerPath.c_str(), TfStringJoin(errors, ", ").c_str());

            m.Clear();
        }
    }
    searchPath = Sdf_CreateIdentifier(searchPath, arguments);

    const auto& byRealPath = _layers.ByRealPath();
    const auto realPathIt = byRealPath.find(searchPath);
    if (realPathIt != byRealPath.end()) {
        foundLayer = realPathIt->second;
    }

    TF_DEBUG(SDF_LAYER).Msg(
        "Sdf_LayerRegistry::_FindByRealPath('%s') => %s\n",
        searchPath.c_str(),
        foundLayer ? "Found" : "Not Found");

    return foundLayer;
}

SdfLayerHandleSet
Sdf_LayerRegistry::GetLayers() const
{
    SdfLayerHandleSet layers;

    for (const auto& entry : _layers.ByIdentifier()) {
        SdfLayerHandle layer = entry.second;
        if (TF_VERIFY(layer, "Found expired layer in registry")) {
            layers.insert(layer);
        }
    }

    return layers;
}

std::ostream&
operator<<(std::ostream& ostr, const Sdf_LayerRegistry& registry)
{
    SdfLayerHandleSet layers = registry.GetLayers();
    TF_FOR_ALL(i, layers) {
        if (SdfLayerHandle layer = *i) {
            ostr << TfStringPrintf(
                "%p[ref=%zu]:\n"
                "    format           = %s\n"
                "    identifier       = '%s'\n"
                "    repositoryPath   = '%s'\n"
                "    realPath         = '%s'\n"
                "    version          = '%s'\n"
                "    assetInfo        = \n'%s'\n"
                "    muted            = %s\n"
                "    anonymous        = %s\n"
                "\n"
                , layer.GetUniqueIdentifier()
                , layer->GetCurrentCount()
                , layer->GetFileFormat()->GetFormatId().GetText()
                , layer->GetIdentifier().c_str()
                , layer->GetRepositoryPath().c_str()
                , layer->GetRealPath().c_str()
                , layer->GetVersion().c_str()
                , TfStringify(layer->GetAssetInfo()).c_str()
                , (layer->IsMuted()          ? "True" : "False")
                , (layer->IsAnonymous()      ? "True" : "False")
                );
        }
    }

    return ostr;
}

PXR_NAMESPACE_CLOSE_SCOPE
