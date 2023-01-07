//
// Copyright 2021 Pixar
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
#include "primDataSourceOverlayCache.h"

#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/denseHashSet.h"
#include "pxr/base/work/utils.h"

PXR_NAMESPACE_OPEN_SCOPE

HdPrimDataSourceOverlayCache::~HdPrimDataSourceOverlayCache() = default;

HdSceneIndexPrim
HdPrimDataSourceOverlayCache::GetPrim(const SdfPath &primPath) const
{
    const auto it = _cache.find(primPath);
    if (it != _cache.end()) {
        return it->second;
    }

    return { TfToken(), nullptr };
}

void
HdPrimDataSourceOverlayCache::HandlePrimsAdded(
    const HdSceneIndexObserver::AddedPrimEntries &entries,
    const HdSceneIndexBaseRefPtr &source)
{
    TRACE_FUNCTION();

    for (const auto &entry : entries) {
        HdContainerDataSourceHandle parentOverlay = nullptr;
        HdContainerDataSourceHandle inputDataSource =
            source->GetPrim(entry.primPath).dataSource;

        auto iterBoolPair =
            _cache.insert({entry.primPath, HdSceneIndexPrim()});
        HdSceneIndexPrim &prim = iterBoolPair.first->second;

        // Always update the prim type.
        prim.primType = entry.primType;

        // If the wrapper exists, update the input datasource;
        // otherwise, create it.
        if (prim.dataSource) {
            _HdPrimDataSourceOverlay::Cast(prim.dataSource)->
                UpdateInputDataSource(inputDataSource);
        } else {
            prim.dataSource = _HdPrimDataSourceOverlay::New(
                inputDataSource, parentOverlay,
                shared_from_this());
        }
    }
}

void
HdPrimDataSourceOverlayCache::HandlePrimsRemoved(
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    for (const auto &entry : entries) {
        if (entry.primPath.IsAbsoluteRootPath()) {
            // Special case removing the whole scene, since this is a common
            // shutdown operation.
            _cache.ClearInParallel();
            TfReset(_cache);
        } else {
            auto startEndIt = _cache.FindSubtreeRange(entry.primPath);
            for (auto it = startEndIt.first; it != startEndIt.second; ++it) {
                WorkSwapDestroyAsync(it->second.dataSource);
            }
            if (startEndIt.first != startEndIt.second) {
                _cache.erase(startEndIt.first);
            }
        }
    }
}

void
HdPrimDataSourceOverlayCache::HandlePrimsDirtied(
    const HdSceneIndexObserver::DirtiedPrimEntries &entries,
    HdSceneIndexObserver::DirtiedPrimEntries *additionalDirtied)
{
    for (const auto &entry : entries) {
        HdDataSourceLocatorSet dirtyAttributes;
        for (const auto &pair : _overlayTopology) {
            if (pair.second.onPrim.Intersects(entry.dirtyLocators)) {
                dirtyAttributes.insert(HdDataSourceLocator(pair.first));
            }
        }

        if (dirtyAttributes.IsEmpty()) {
            continue;
        }

        const auto it = _cache.find(entry.primPath);
        if (it != _cache.end()) {
            _HdPrimDataSourceOverlay::Cast(it->second.dataSource)
                ->PrimDirtied(dirtyAttributes);
        }
        if (additionalDirtied) {
            additionalDirtied->push_back({entry.primPath, dirtyAttributes});
        }
    }
}

HdPrimDataSourceOverlayCache::_HdPrimDataSourceOverlay::_HdPrimDataSourceOverlay(
    HdContainerDataSourceHandle inputDataSource,
    HdContainerDataSourceHandle parentOverlayDataSource,
    const std::weak_ptr<const HdPrimDataSourceOverlayCache> cache)
: _inputDataSource(inputDataSource)
, _parentOverlayDataSource(parentOverlayDataSource)
, _cache(cache)
{
}

void
HdPrimDataSourceOverlayCache::_HdPrimDataSourceOverlay::UpdateInputDataSource(
    HdContainerDataSourceHandle inputDataSource)
{
    _inputDataSource = inputDataSource;
    _overlayMap.clear();
}

void
HdPrimDataSourceOverlayCache::_HdPrimDataSourceOverlay::PrimDirtied(
    const HdDataSourceLocatorSet &dirtyAttributes)
{
    TRACE_FUNCTION();

    for (const auto &attr : dirtyAttributes) {
        _overlayMap.erase(attr.GetFirstElement());
    }
}

TfTokenVector
HdPrimDataSourceOverlayCache::_HdPrimDataSourceOverlay::GetNames()
{
    TRACE_FUNCTION();

    if (ARCH_UNLIKELY(!_inputDataSource)) {
        return {};
    }

    TfTokenVector names = _inputDataSource->GetNames();
    TfDenseHashSet<TfToken, TfHash> namesAtStart(names.begin(), names.end());

    auto cache = _cache.lock();
    if (TF_VERIFY(cache)) {
        bool sortMe = false;
        for (const auto &overlay : cache->_overlayTopology) {
            if (overlay.second.dependenciesOptional) {
                names.push_back(overlay.first);
                sortMe = true;
                continue;
            }
            for (const auto &loc : overlay.second.onPrim) {
                if (namesAtStart.find(loc.GetFirstElement())
                        != namesAtStart.end()) {
                    names.push_back(overlay.first);
                    sortMe = true;
                    break;
                }
            }
        }
        if (sortMe) {
            // XXX: Possibly unnecessary...
            std::sort(names.begin(), names.end());
            auto it = std::unique(names.begin(), names.end());
            names.erase(it, names.end());
        }
    }

    return names;
}

HdDataSourceBaseHandle
HdPrimDataSourceOverlayCache::_HdPrimDataSourceOverlay::Get(
    const TfToken &name)
{
    if (ARCH_UNLIKELY(!_inputDataSource)) {
        return nullptr;
    }

    auto cache = _cache.lock();
    if (TF_VERIFY(cache)) {
        const auto topoIt = cache->_overlayTopology.find(name);
        if (topoIt != cache->_overlayTopology.end()) {
            auto valueIt = _overlayMap.find(name);
            if (valueIt != _overlayMap.end()) {
                return valueIt->second;
            }

            // If "name" is part of the overlays, but it's not in the overlay
            // map, it hasn't been computed... First, check for dependencies.
            if (!topoIt->second.dependenciesOptional) {
                for (const auto &loc : topoIt->second.onPrim) {
                    if (!_inputDataSource->Get(loc.GetFirstElement())) {
                        _overlayMap.insert(std::make_pair(name, nullptr));
                        return nullptr;
                    }
                }
            }

            // If the dependencies are ok, compute it.
            HdDataSourceBaseHandle ds =
                cache->_ComputeOverlayDataSource(name, _inputDataSource,
                        _parentOverlayDataSource);
            _overlayMap.insert(std::make_pair(name, ds));
            return ds;
        }
    }

    return _inputDataSource->Get(name);
}

PXR_NAMESPACE_CLOSE_SCOPE
