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

HdContainerDataSourceHandle
HdPrimDataSourceOverlayCache::_AddPrim(
    const SdfPath &primPath, const HdSceneIndexBaseRefPtr &source)
{
    HdContainerDataSourceHandle parentOverlay = nullptr;
    if (_hierarchical && primPath != SdfPath::AbsoluteRootPath()) {
        SdfPath parentPath = primPath.GetParentPath();
        parentOverlay = GetPrim(parentPath).dataSource;
        if (!parentOverlay) {
            parentOverlay = _AddPrim(parentPath, source);
        }
    }

    HdSceneIndexPrim inputPrim = source->GetPrim(primPath);
    HdContainerDataSourceHandle input = inputPrim.dataSource;

    HdSceneIndexPrim cached = {
        inputPrim.primType, 
        _HdPrimDataSourceOverlay::New(input, parentOverlay, shared_from_this()),
    };

    _cache[primPath] = cached;
    return cached.dataSource;
}

void
HdPrimDataSourceOverlayCache::HandlePrimsAdded(
    const HdSceneIndexObserver::AddedPrimEntries &entries,
    const HdSceneIndexBaseRefPtr &source)
{
    for (const auto &entry : entries) {
        _AddPrim(entry.primPath, source);
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
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    for (const auto &entry : entries) {
        const auto it = _cache.find(entry.primPath);
        if (it != _cache.end()) {
            _HdPrimDataSourceOverlay::Cast(it->second.dataSource)
                ->PrimDirtied(entry.dirtyLocators);
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
    auto c = _cache.lock();
    if (TF_VERIFY(c)) {
        _overlayNames = c->_GetOverlayNames(_inputDataSource);
    }
}

void
HdPrimDataSourceOverlayCache::_HdPrimDataSourceOverlay::PrimDirtied(
    const HdDataSourceLocatorSet &locators)
{
    auto cache = _cache.lock();
    if (TF_VERIFY(cache)) {
        // Refresh the overlay names.
        _overlayNames = cache->_GetOverlayNames(_inputDataSource);

        // If the dirty locators intersect the dependencies of any overlay,
        // boot the cached copy.
        for (_OverlayMap::iterator i = _overlayMap.begin();
             i != _overlayMap.end();) {

            HdDataSourceLocatorSet dependencies =
                cache->_GetOverlayDependencies(i->first);
            if (dependencies.Intersects(locators)) {
                i = _overlayMap.erase(i);
            } else {
                ++i;
            }
        }
    }
}

bool
HdPrimDataSourceOverlayCache::_HdPrimDataSourceOverlay::Has(
    const TfToken &name)
{
    if (std::find(_overlayNames.begin(), _overlayNames.end(), name) !=
        _overlayNames.end()) {
        return true;
    }

    if (!_inputDataSource) {
        return false;
    }

    return _inputDataSource->Has(name);
}

TfTokenVector
HdPrimDataSourceOverlayCache::_HdPrimDataSourceOverlay::GetNames()
{
    if (!_inputDataSource) {
        return {};
    }

    TfTokenVector names = _inputDataSource->GetNames();
    names.insert(names.end(), _overlayNames.begin(), _overlayNames.end());

    // XXX: Possibly unnecessary...
    std::sort(names.begin(), names.end());
    auto it = std::unique(names.begin(), names.end());
    names.erase(it, names.end());

    return names;
}

HdDataSourceBaseHandle
HdPrimDataSourceOverlayCache::_HdPrimDataSourceOverlay::Get(
    const TfToken &name)
{
    if (std::find(_overlayNames.begin(), _overlayNames.end(), name) !=
        _overlayNames.end()) {
        auto it = _overlayMap.find(name);
        if (it != _overlayMap.end()) {
            return it->second;
        }

        // If "name" is part of the overlays, but it's not in the overlay
        // map, it hasn't been computed...
        auto cache = _cache.lock();
        if (TF_VERIFY(cache)) {
            HdDataSourceBaseHandle ds =
                cache->_ComputeOverlayDataSource(name, _inputDataSource,
                        _parentOverlayDataSource);
            _overlayMap.insert(std::make_pair(name, ds));
            return ds;
        }
    }

    if (!_inputDataSource) {
        return nullptr;
    }
    return _inputDataSource->Get(name);
}

PXR_NAMESPACE_CLOSE_SCOPE
