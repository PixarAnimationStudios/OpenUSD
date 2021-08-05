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

#include "pxr/imaging/hdSt/renderParam.h"

PXR_NAMESPACE_OPEN_SCOPE

HdStRenderParam::HdStRenderParam()
    : _drawBatchesVersion(1)
    , _materialTagsVersion(1)
    , _geomSubsetDrawItemsVersion(1)
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

static
bool
_IsMaterialTagCounted(const TfToken &materialTag)
{
    return !materialTag.IsEmpty();
}

bool
HdStRenderParam::HasMaterialTag(const TfToken &materialTag) const
{
    if (!_IsMaterialTagCounted(materialTag)) {
        return true;
    }

    std::shared_lock<std::shared_timed_mutex> lock(_materialTagToCountMutex);

    const auto it = _materialTagToCount.find(materialTag);
    if (it == _materialTagToCount.end()) {
        return false;
    }
    
    return it->second > 0;
}

void
HdStRenderParam::_AdjustMaterialTagCount(const TfToken &materialTag,
                                         const int i)
{
    if (!_IsMaterialTagCounted(materialTag)) {
        return;
    }

    {
        // Map already had entry for materialTag.
        // Shared lock is sufficient because the entry's integer is atomic.
        std::shared_lock<std::shared_timed_mutex> l(_materialTagToCountMutex);
        const auto it = _materialTagToCount.find(materialTag);
        if (it != _materialTagToCount.end()) {
            it->second += i;
            return;
        }
    }

    {
        // Map had no entry for materialTag.
        std::unique_lock<std::shared_timed_mutex> l(_materialTagToCountMutex);
        _materialTagToCount[materialTag] += i;
    }
}

void
HdStRenderParam::IncreaseMaterialTagCount(const TfToken &materialTag)
{
    _AdjustMaterialTagCount(materialTag, +1);
}

void
HdStRenderParam::DecreaseMaterialTagCount(const TfToken &materialTag)
{
    _AdjustMaterialTagCount(materialTag, -1);

    // Note that it is difficult to remove zero entries from the map here during
    // multi-threaded access.
    // It is probably not worth implementing a garbage collection for this map.
}

PXR_NAMESPACE_CLOSE_SCOPE
