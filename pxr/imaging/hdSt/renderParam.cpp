//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdSt/renderParam.h"

PXR_NAMESPACE_OPEN_SCOPE

HdStRenderParam::HdStRenderParam()
    : _drawBatchesVersion(1)
    , _materialTagsVersion(1)
    , _geomSubsetDrawItemsVersion(1)
    , _activeDrawTargetSetVersion(1)
    , _needsGarbageCollection(false)
{
}

HdStRenderParam::~HdStRenderParam() = default;

void
HdStRenderParam::MarkDrawBatchesDirty()
{
    ++_drawBatchesVersion; // uses std::memory_order_seq_cst 
}

unsigned int
HdStRenderParam::GetDrawBatchesVersion() const
{
    // Can use relaxed ordering because render passes are expected to
    // only read the value, and that too in a single threaded fashion.
    return _drawBatchesVersion.load(std::memory_order_relaxed);
}

void
HdStRenderParam::MarkMaterialTagsDirty()
{
    ++_materialTagsVersion; // uses std::memory_order_seq_cst 
}

unsigned int
HdStRenderParam::GetMaterialTagsVersion() const
{
    // Can use relaxed ordering because render passes are expected to
    // only read the value, and that too in a single threaded fashion.
    return _materialTagsVersion.load(std::memory_order_relaxed);
}

void
HdStRenderParam::MarkGeomSubsetDrawItemsDirty()
{
    ++_geomSubsetDrawItemsVersion; // uses std::memory_order_seq_cst 
}

unsigned int
HdStRenderParam::GetGeomSubsetDrawItemsVersion() const
{
    // Can use relaxed ordering because render passes are expected to
    // only read the value, and that too in a single threaded fashion.
    return _geomSubsetDrawItemsVersion.load(std::memory_order_relaxed);
}

////////////////////////////////////////////////////////////////////////////////
// materialTag tracking

bool
HdStRenderParam::HasMaterialTag(const TfToken &materialTag) const
{
    return _HasTag(&_materialTagToCountMutex,&_materialTagToCount, materialTag);
}

void
HdStRenderParam::IncreaseMaterialTagCount(const TfToken &materialTag)
{
    _AdjustTagCount(&_materialTagToCountMutex, &_materialTagToCount,
        materialTag, +1);
}

void
HdStRenderParam::DecreaseMaterialTagCount(const TfToken &materialTag)
{
    _AdjustTagCount(&_materialTagToCountMutex, &_materialTagToCount,
        materialTag, -1);
}

////////////////////////////////////////////////////////////////////////////////
// renderTag tracking

bool
HdStRenderParam::HasAnyRenderTag(const TfTokenVector &renderTags) const
{
    for (const TfToken &tag : renderTags) {
        if (_HasTag(&_renderTagToCountMutex, &_renderTagToCount, tag)) {
            return true;
        }
    } 
    return false;
}

void
HdStRenderParam::IncreaseRenderTagCount(const TfToken &renderTag)
{
    _AdjustTagCount(&_renderTagToCountMutex, &_renderTagToCount,
        renderTag, +1);
}

void
HdStRenderParam::DecreaseRenderTagCount(const TfToken &renderTag)
{
    _AdjustTagCount(&_renderTagToCountMutex, &_renderTagToCount,
        renderTag, -1);
}

void
HdStRenderParam::_AdjustTagCount(
    std::shared_timed_mutex *mutex,
    _TagToCountMap *tagToCountMap,
    const TfToken &tag,
    const int increment)
{
    if (tag.IsEmpty()) {
        return;
    }

    {
        // Map already had entry for tag.
        // Shared lock is sufficient because the entry's integer is atomic.
        std::shared_lock<std::shared_timed_mutex> l(*mutex);
        const auto it = tagToCountMap->find(tag);
        if (it != tagToCountMap->end()) {
            it->second += increment;
            return;
        }
    }

    {
        // Map had no entry for tag.
        std::unique_lock<std::shared_timed_mutex> l(*mutex);
        (*tagToCountMap)[tag] += increment;
    }

    // Note that it is difficult to remove zero entries from the map here during
    // multi-threaded access.
    // It is probably not worth implementing a garbage collection for this map.
}

bool
HdStRenderParam::_HasTag(
    std::shared_timed_mutex *mutex,
    const _TagToCountMap *tagToCountMap,
    const TfToken &tag) const
{
    if (tag.IsEmpty()) {
        return true;
    }

    std::shared_lock<std::shared_timed_mutex> lock(*mutex);

    const auto it = tagToCountMap->find(tag);
    if (it == tagToCountMap->end()) {
        return false;
    }
    
    return it->second > 0;
}

void
HdStRenderParam::MarkActiveDrawTargetSetDirty()
{
    ++_activeDrawTargetSetVersion;
}

unsigned int
HdStRenderParam::GetActiveDrawTargetSetVersion() const
{
    return _activeDrawTargetSetVersion.load(std::memory_order_relaxed);
}

PXR_NAMESPACE_CLOSE_SCOPE
