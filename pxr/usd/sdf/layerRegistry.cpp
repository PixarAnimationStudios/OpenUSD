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

using namespace boost::multi_index;
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

const Sdf_LayerRegistry::layer_identifier::result_type&
Sdf_LayerRegistry::layer_identifier::operator()(
    const SdfLayerHandle& layer) const
{
    static string emptyString;
    return layer ? layer->GetIdentifier() : emptyString;
}

Sdf_LayerRegistry::layer_repository_path::result_type
Sdf_LayerRegistry::layer_repository_path::operator()(
    const SdfLayerHandle& layer) const
{
    if (!layer) {
        return std::string();
    }

    const string repoPath = layer->GetRepositoryPath();
    if (!repoPath.empty()) {
        string layerPath, arguments;
        TF_VERIFY(Sdf_SplitIdentifier(
                layer->GetIdentifier(), &layerPath, &arguments));
        return Sdf_CreateIdentifier(repoPath, arguments);
    }

    return std::string();
}

Sdf_LayerRegistry::layer_real_path::result_type
Sdf_LayerRegistry::layer_real_path::operator()(
    const SdfLayerHandle& layer) const
{
    if (!layer) {
        return std::string();
    }

    if (layer->IsAnonymous()) {
        // The layer_real_path index requires a unique key. As anonymous do
        // not have a realPath, we use the (unique) identifier as the key.
        return layer->GetIdentifier();
    }

    const string realPath = layer->GetRealPath();
    if (!realPath.empty()) {
        string layerPath, arguments;
        TF_VERIFY(Sdf_SplitIdentifier(
                layer->GetIdentifier(), &layerPath, &arguments));
        return Sdf_CreateIdentifier(realPath, arguments);
    }

    return std::string();
}

Sdf_LayerRegistry::Sdf_LayerRegistry()
{
}

struct update_index_only {
    void operator()(const SdfLayerHandle&) { }
};

void
Sdf_LayerRegistry::InsertOrUpdate(
    const SdfLayerHandle& layer)
{
    TRACE_FUNCTION();

    if (!layer) {
        TF_CODING_ERROR("Expired layer handle");
        return;
    }

    TF_DEBUG(SDF_LAYER).Msg(
        "Sdf_LayerRegistry::InsertOrUpdate(%s)\n",
        Sdf_LayerDebugRepr(layer).c_str());

    // Attempt to insert the layer into the registry. This may fail because
    // the new layer violates constraints of one of the registry indices.
    std::pair<_Layers::iterator, bool> result = _layers.insert(layer);
    if (!result.second) {
        SdfLayerHandle existingLayer = *result.first;
        if (layer == existingLayer) {
            // We failed to insert the layer into the registry because this
            // layer object is already in the registry. All we need to do is
            // update the indices so it can be found.
            _layers.modify(result.first, update_index_only());
        } else {
            // We failed to insert the layer into the registry because there
            // is a realPath conflict. This can happen when the same layer is
            // crated twice in the same location in the same session.
            TF_CODING_ERROR("Cannot insert duplicate registry entry for "
                "%s layer %s over existing entry for %s layer %s",
                layer->GetFileFormat()->GetFormatId().GetText(),
                Sdf_LayerDebugRepr(layer).c_str(),
                existingLayer->GetFileFormat()->GetFormatId().GetText(),
                Sdf_LayerDebugRepr(existingLayer).c_str());
        }
    }
}

void
Sdf_LayerRegistry::Erase(
    const SdfLayerHandle& layer)
{
    bool erased = _layers.erase(layer);

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
        foundLayer = FindByIdentifier(inputLayerPath);
    } else {
        ArResolver& resolver = ArGetResolver();

        const string& layerPath = inputLayerPath;

        // If the layer path depends on context there may be multiple
        // layers with the same identifier but different resolved paths.
        // In this case we need to look up the layer by resolved path.
        string assetPath, args;
        Sdf_SplitIdentifier(inputLayerPath, &assetPath, &args);
        if (!resolver.IsContextDependentPath(assetPath)) {
            foundLayer = FindByIdentifier(layerPath);
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
Sdf_LayerRegistry::FindByIdentifier(
    const string& layerPath) const
{
    TRACE_FUNCTION();

    SdfLayerHandle foundLayer;

    const _LayersByIdentifier& byIdentifier = _layers.get<by_identifier>();
    _LayersByIdentifier::const_iterator identifierIt =
        byIdentifier.find(layerPath);
    if (identifierIt != byIdentifier.end())
        foundLayer = *identifierIt;

    TF_DEBUG(SDF_LAYER).Msg(
        "Sdf_LayerRegistry::FindByIdentifier('%s') => %s\n",
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

    const _LayersByRepositoryPath& byRepoPath = _layers.get<by_repository_path>();
    _LayersByRepositoryPath::const_iterator repoPathIt =
        byRepoPath.find(layerPath);
    if (repoPathIt != byRepoPath.end())
        foundLayer = *repoPathIt;

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

    const _LayersByRealPath& byRealPath = _layers.get<by_real_path>();
    _LayersByRealPath::const_iterator realPathIt =
        byRealPath.find(searchPath);
    if (realPathIt != byRealPath.end())
        foundLayer = *realPathIt;

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

    TF_FOR_ALL(i, _layers.get<by_identity>()) {
        SdfLayerHandle layer = *i;
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
