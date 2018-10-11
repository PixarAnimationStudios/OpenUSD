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
#ifndef HD_BUFFER_ARRAY_REGISTRY_H
#define HD_BUFFER_ARRAY_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/strategyBase.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/base/vt/dictionary.h"
#include "pxr/base/tf/token.h"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <tbb/concurrent_unordered_map.h>

#include <condition_variable>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE


typedef boost::shared_ptr<class HdBufferArray> HdBufferArraySharedPtr;

/// \class HdBufferArrayRegistry
///
/// Manages the pool of buffer arrays.
///
class HdBufferArrayRegistry : public boost::noncopyable {
public:
    HF_MALLOC_TAG_NEW("new HdBufferArrayRegistry");

    HD_API
    HdBufferArrayRegistry();
    ~HdBufferArrayRegistry()   = default;

    /// Allocate new buffer array range using strategy
    /// Thread-Safe
    HD_API
    HdBufferArrayRangeSharedPtr AllocateRange(
        HdAggregationStrategy *strategy,
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint);

    /// Triggers reallocation on all buffers managed by the registry.
    HD_API
    void   ReallocateAll(HdAggregationStrategy *strategy);

    /// Frees up buffers that no longer contain any allocated ranges.
    HD_API
    void   GarbageCollect();

    /// Generate a report on resources consumed by the managed
    /// buffer array.  The returned size is an esitmate of the 
    /// gpu memory consumed by the buffers
    HD_API
    size_t GetResourceAllocation(HdAggregationStrategy *strategy, 
                                 VtDictionary &result) const;
    
    /// Debug dump
    HD_API
    friend std::ostream &operator <<(std::ostream &out,
                                     const HdBufferArrayRegistry& self);

private:
    typedef std::list<HdBufferArraySharedPtr> _HdBufferArraySharedPtrList;

    /// \struct _Entry
    ///
    /// Entry in the buffer array cache.  The list is the buffer arrays which
    /// all have the same format.  There is as a lock for modifications to the
    /// entry and a condition used to determine if the entry has been
    /// construction.
    /// 
    /// A constructed entry always has at least 1 buffer array in its list.
    ///
    struct _Entry
    {
        _HdBufferArraySharedPtrList bufferArrays;
        std::mutex                  lock;
        std::condition_variable     emptyCondition;

        // Default / Copy constructors needed for std::make_pair.
        // as the version of TBB doesn't have emplace() yet.
        _Entry() {}
        _Entry(const _Entry &other) { TF_VERIFY(bufferArrays.empty()); }
    };


    /// Predicate class for determine if an entry
    /// has been consturcted (determined by there
    /// being at least 1 entry in the buffer array list.
    class _EntryIsNotEmpty
    {
    public:
        _EntryIsNotEmpty(const _Entry &entry) : _entry(entry) {}

        bool operator()() {
            return (!(_entry.bufferArrays.empty()));
        }

    private:
        const _Entry &_entry;
    };

    typedef tbb::concurrent_unordered_map< HdAggregationStrategy::AggregationId, _Entry> _BufferArrayIndex;

    _BufferArrayIndex _entries;

    /// Concurrently adds a new buffer to an entry in the cache.
    /// If expectedTail differs from the buffer at the end of the
    /// entries list after locking, then this function fails and
    /// does not add a new buffer (because another thread beat it
    /// to it).
    /// strategy is the factory class used to create the BufferArray.
    /// role and bufferSpecs are parameters to the BufferArray creation.
    void _InsertNewBufferArray(_Entry &entry,
                               const HdBufferArraySharedPtr &expectedTail,
                               HdAggregationStrategy *strategy,
                               TfToken const &role,
                               HdBufferSpecVector const &bufferSpecs,
                               HdBufferArrayUsageHint usageHint);
};
    

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_BUFFER_ARRAY_REGISTRY_H
