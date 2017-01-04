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
/// \file LayerStackRegistry.cpp

#include "pxr/usd/pcp/layerStackRegistry.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdf/layerUtils.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticData.h"
#include <boost/unordered_map.hpp>

#include <algorithm>
#include <mutex>
#include <utility>

using std::pair;
using std::make_pair;

class Pcp_LayerStackRegistryData {
public:
    Pcp_LayerStackRegistryData(
        const std::string& targetSchema_, bool isUsd) 
        : targetSchema(targetSchema_)
        , isUsd(isUsd)
    { }

    typedef SdfLayerHandleVector Layers;
    typedef PcpLayerStackPtrVector LayerStacks;
    typedef boost::unordered_map<PcpLayerStackIdentifier, PcpLayerStackPtr>
        IdentifierToLayerStack;
    typedef boost::unordered_map<SdfLayerHandle, LayerStacks>
        LayerToLayerStacks;
    typedef boost::unordered_map<PcpLayerStackPtr, Layers>
        LayerStackToLayers;

    typedef boost::unordered_map<std::string, LayerStacks>
        MutedLayerIdentifierToLayerStacks;
    typedef boost::unordered_map<PcpLayerStackPtr, std::set<std::string> >
        LayerStackToMutedLayerIdentifiers;

    IdentifierToLayerStack identifierToLayerStack;
    LayerToLayerStacks layerToLayerStacks;
    LayerStackToLayers layerStackToLayers;
    MutedLayerIdentifierToLayerStacks mutedLayerIdentifierToLayerStacks;
    LayerStackToMutedLayerIdentifiers layerStackToMutedLayerIdentifiers;

    const PcpLayerStackPtrVector empty;
    const std::string targetSchema;
    bool isUsd;
    Pcp_MutedLayers mutedLayers;

    std::mutex mutex;
};

Pcp_LayerStackRegistryRefPtr
Pcp_LayerStackRegistry::New(const std::string& targetSchema, bool isUsd)
{
    return TfCreateRefPtr(new Pcp_LayerStackRegistry(targetSchema, isUsd));
}

Pcp_LayerStackRegistry::Pcp_LayerStackRegistry(
    const std::string& targetSchema, bool isUsd)
    : _data(new Pcp_LayerStackRegistryData(targetSchema, isUsd))
{
    // Do nothing
}

Pcp_LayerStackRegistry::~Pcp_LayerStackRegistry()
{
    // Do nothing
}

void 
Pcp_LayerStackRegistry::MuteAndUnmuteLayers(
    const SdfLayerHandle& anchorLayer,
    std::vector<std::string>* layersToMute,
    std::vector<std::string>* layersToUnmute)
{
    _data->mutedLayers.MuteAndUnmuteLayers(
        anchorLayer, layersToMute, layersToUnmute);
}

const std::vector<std::string>& 
Pcp_LayerStackRegistry::GetMutedLayers() const
{
    return _data->mutedLayers.GetMutedLayers();
}

bool 
Pcp_LayerStackRegistry::IsLayerMuted(const SdfLayerHandle& anchorLayer,
                                     const std::string& layerIdentifier,
                                     std::string* canonicalSublayerId) const
{
    return _data->mutedLayers.IsLayerMuted(
        anchorLayer, layerIdentifier, canonicalSublayerId);
}

const PcpLayerStackPtrVector&
Pcp_LayerStackRegistry::FindAllUsingMutedLayer(const std::string& layerId) const
{
    std::lock_guard<std::mutex> lock(_data->mutex);
    const auto i = _data->mutedLayerIdentifierToLayerStacks.find(layerId);
    return i != _data->mutedLayerIdentifierToLayerStacks.end() ? 
        i->second : _data->empty;
}

PcpLayerStackRefPtr
Pcp_LayerStackRegistry::FindOrCreate(const PcpLayerStackIdentifier& identifier,
                                     PcpErrorVector *allErrors)
{
    // Can only create layer stacks for valid identifiers so if the identifier
    // is invalid we can't have an entry for it.
    if (!identifier) {
        TF_CODING_ERROR("Cannot build layer stack with null rootLayer");
        return TfNullPtr;
    }

    if (const PcpLayerStackPtr & layerStack = Find(identifier)) {
        return layerStack;
    } else {
        PcpLayerStackRefPtr refLayerStack =
            TfCreateRefPtr(new PcpLayerStack(
                identifier, _GetTargetSchema(), _GetMutedLayers(), _IsUsd()));

        // Take the lock and see if we get to install the layerstack.
        std::lock_guard<std::mutex> lock(_data->mutex);
        std::pair<Pcp_LayerStackRegistryData::IdentifierToLayerStack::iterator,
                  bool> iresult = _data->identifierToLayerStack.insert(
                      make_pair(identifier, refLayerStack));
        if (iresult.second) {
            // If so give it a link back to us so it can remove itself upon
            // destruction, and install its layers into our structures.
            refLayerStack->_registry = TfCreateWeakPtr(this);
            _SetLayers(get_pointer(refLayerStack));

            // Return errors from newly computed layer stacks.
            PcpErrorVector errors = refLayerStack->GetLocalErrors();
            allErrors->insert( allErrors->end(), errors.begin(), errors.end());
        }

        return iresult.first->second;
    }
}

PcpLayerStackPtr
Pcp_LayerStackRegistry::Find(const PcpLayerStackIdentifier& identifier) const
{
    std::lock_guard<std::mutex> lock(_data->mutex);
    Pcp_LayerStackRegistryData::IdentifierToLayerStack::const_iterator i =
        _data->identifierToLayerStack.find(identifier);
    return (i != _data->identifierToLayerStack.end())?
        i->second : PcpLayerStackPtr();
}

const PcpLayerStackPtrVector&
Pcp_LayerStackRegistry::FindAllUsingLayer(const SdfLayerHandle& layer) const
{
    std::lock_guard<std::mutex> lock(_data->mutex);
    Pcp_LayerStackRegistryData::LayerToLayerStacks::const_iterator i =
        _data->layerToLayerStacks.find(layer);
    return i != _data->layerToLayerStacks.end() ? i->second : _data->empty;
}

std::vector<PcpLayerStackPtr>
Pcp_LayerStackRegistry::GetAllLayerStacks() const
{
    std::lock_guard<std::mutex> lock(_data->mutex);
    std::vector<PcpLayerStackPtr> result;
    result.reserve(_data->identifierToLayerStack.size());
    TF_FOR_ALL(i, _data->identifierToLayerStack) {
        TF_VERIFY(i->second, "Unexpected dead layer stack %s",
                  TfStringify(i->first).c_str());
        result.push_back(i->second);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////
// Private helper methods.

void
Pcp_LayerStackRegistry::_Remove(const PcpLayerStackIdentifier& identifier,
                                const PcpLayerStack *layerStack)
{
    Pcp_LayerStackRegistryData::IdentifierToLayerStack::const_iterator i =
        _data->identifierToLayerStack.find(identifier);
    if (TF_VERIFY(i != _data->identifierToLayerStack.end()) &&
        TF_VERIFY(i->second.operator->() == layerStack)) {
        _data->identifierToLayerStack.erase(identifier);
    }
}

void
Pcp_LayerStackRegistry::_SetLayers(const PcpLayerStack* layerStack)
{
    PcpLayerStackPtr layerStackPtr = TfCreateNonConstWeakPtr(layerStack);

    // Get the layers for the layer stack.
    Pcp_LayerStackRegistryData::Layers& layers =
        _data->layerStackToLayers[layerStackPtr];

    // Remove layer stack from the table entry for each layer in
    // the layer stack.
    for (const auto& layer : layers) {
        Pcp_LayerStackRegistryData::LayerStacks& layerStacks =
            _data->layerToLayerStacks[layer];
        layerStacks.erase(std::find(layerStacks.begin(),
                                    layerStacks.end(), layerStackPtr));
    }

    // Save the layers for the layer stack.
    const SdfLayerRefPtrVector& newLayers = layerStack->GetLayers();
    if (newLayers.empty()) {
        // Don't leave empty entries hanging around.
        _data->layerStackToLayers.erase(layerStackPtr);
    } else {
        layers.assign(newLayers.begin(), newLayers.end());
    }

    // Add the layer stack for each layer in the layer stack.
    for (const auto& layer : newLayers) {
        _data->layerToLayerStacks[layer].push_back(layerStackPtr);
    }

    // Also store mappings from layer stack <-> muted layers in
    // the layer stack.
    std::set<std::string>* mutedLayerIdentifiers = TfMapLookupPtr(
        _data->layerStackToMutedLayerIdentifiers, layerStackPtr);

    if (mutedLayerIdentifiers) {
        for (const auto& layerId : *mutedLayerIdentifiers) {
            Pcp_LayerStackRegistryData::LayerStacks& layerStacks =
                _data->mutedLayerIdentifierToLayerStacks[layerId];
            layerStacks.erase(std::find(layerStacks.begin(),
                                        layerStacks.end(), layerStackPtr));
        }
    }

    const std::set<std::string>& newMutedLayers = layerStack->GetMutedLayers();
    if (newMutedLayers.empty()) {
        if (mutedLayerIdentifiers) {
            // Don't leave empty entries hanging around.
            _data->layerStackToMutedLayerIdentifiers.erase(layerStackPtr);
        }
    }
    else {
        if (!mutedLayerIdentifiers) {
            mutedLayerIdentifiers = 
                &_data->layerStackToMutedLayerIdentifiers[layerStackPtr];
        }
        *mutedLayerIdentifiers = std::set<std::string>(
            newMutedLayers.begin(), newMutedLayers.end());
    }

    for (const auto& mutedLayer : newMutedLayers) {
        _data->mutedLayerIdentifierToLayerStacks[mutedLayer]
            .push_back(layerStackPtr);
    }
}

const std::string&
Pcp_LayerStackRegistry::_GetTargetSchema() const
{
    return _data->targetSchema;
}

bool
Pcp_LayerStackRegistry::_IsUsd() const
{
    return _data->isUsd;
}

const Pcp_MutedLayers&
Pcp_LayerStackRegistry::_GetMutedLayers() const
{
    return _data->mutedLayers;
}

// ------------------------------------------------------------

namespace
{
std::string 
_GetCanonicalLayerId(const SdfLayerHandle& anchorLayer, 
                     const std::string& layerId)
{
    const std::string computedLayerId = 
        SdfComputeAssetPathRelativeToLayer(anchorLayer, layerId);
    if (computedLayerId.empty()) {
        return layerId;
    }

    if (SdfLayer::IsAnonymousLayerIdentifier(computedLayerId)) {
        return computedLayerId;
    }

    ArResolver& resolver = ArGetResolver();

    std::string canonicalPath = computedLayerId;
    if (resolver.IsSearchPath(canonicalPath)) {
        std::string resolvedSearchPath = resolver.Resolve(canonicalPath);
        if (!resolvedSearchPath.empty()) {
            canonicalPath.swap(resolvedSearchPath);
        }
    }

    canonicalPath = resolver.ComputeRepositoryPath(canonicalPath);
    return canonicalPath.empty() ? computedLayerId : canonicalPath;
}
}

const std::vector<std::string>& 
Pcp_MutedLayers::GetMutedLayers() const
{
    return _layers;
}

void 
Pcp_MutedLayers::MuteAndUnmuteLayers(const SdfLayerHandle& anchorLayer,
                                     std::vector<std::string>* layersToMute,
                                     std::vector<std::string>* layersToUnmute)
{
    std::vector<std::string> mutedLayers, unmutedLayers;

    for (const auto& layerToMute : *layersToMute) {
        const std::string canonicalId = 
            _GetCanonicalLayerId(anchorLayer, layerToMute);

        const auto layerIt = std::lower_bound(
            _layers.begin(), _layers.end(), canonicalId);
        if (layerIt == _layers.end() || *layerIt != canonicalId) {
            _layers.insert(layerIt, canonicalId);
            mutedLayers.push_back(canonicalId);
        }
    }

    for (const auto& layerToUnmute : *layersToUnmute) {
        const std::string canonicalId = 
            _GetCanonicalLayerId(anchorLayer, layerToUnmute);

        const auto layerIt = std::lower_bound(
            _layers.begin(), _layers.end(), canonicalId);
        if (layerIt != _layers.end() && *layerIt == canonicalId) {
            _layers.erase(layerIt);
            unmutedLayers.push_back(canonicalId);
        }
    }

    layersToMute->swap(mutedLayers);
    layersToUnmute->swap(unmutedLayers);
}

bool 
Pcp_MutedLayers::IsLayerMuted(const SdfLayerHandle& anchorLayer,
                              const std::string& layerId,
                              std::string* canonicalLayerId) const
{
    if (_layers.empty()) {
        return false;
    }

    std::string canonicalId = _GetCanonicalLayerId(anchorLayer, layerId);
    if (std::binary_search(_layers.begin(), _layers.end(), canonicalId)) {
        if (canonicalLayerId) {
            canonicalLayerId->swap(canonicalId);
        }
        return true;
    }
    return false;
}
