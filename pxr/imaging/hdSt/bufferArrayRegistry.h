//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_BUFFER_ARRAY_REGISTRY_H
#define PXR_IMAGING_HD_ST_BUFFER_ARRAY_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/strategyBase.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/base/vt/dictionary.h"
#include "pxr/base/tf/token.h"

#include <tbb/concurrent_unordered_map.h>

#include <condition_variable>
#include <memory>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE


using HdBufferArraySharedPtr = std::shared_ptr<class HdBufferArray>;

/// \class HdStBufferArrayRegistry
///
/// Manages the pool of buffer arrays.
///
class HdStBufferArrayRegistry 
{
public:
    HF_MALLOC_TAG_NEW("new HdStBufferArrayRegistry");

    HDST_API
    HdStBufferArrayRegistry();
    ~HdStBufferArrayRegistry() = default;

    /// Allocate new buffer array range using strategy
    /// Thread-Safe
    HDST_API
    HdBufferArrayRangeSharedPtr AllocateRange(
        HdStAggregationStrategy *strategy,
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint);

    /// Triggers reallocation on all buffers managed by the registry.
    HDST_API
    void   ReallocateAll(HdStAggregationStrategy *strategy);

    /// Frees up buffers that no longer contain any allocated ranges.
    HDST_API
    void   GarbageCollect();

    /// Generate a report on resources consumed by the managed
    /// buffer array.  The returned size is an esitmate of the 
    /// gpu memory consumed by the buffers
    HDST_API
    size_t GetResourceAllocation(HdStAggregationStrategy *strategy, 
                                 VtDictionary &result) const;
    
    /// Debug dump
    HDST_API
    friend std::ostream &operator <<(std::ostream &out,
                                     const HdStBufferArrayRegistry& self);

private:

    HdStBufferArrayRegistry(const HdStBufferArrayRegistry &) = delete;
    HdStBufferArrayRegistry &operator=(const HdStBufferArrayRegistry &)=delete;

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

    using _BufferArrayIndex = tbb::concurrent_unordered_map<
                                HdStAggregationStrategy::AggregationId, _Entry>;
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
                               HdStAggregationStrategy *strategy,
                               TfToken const &role,
                               HdBufferSpecVector const &bufferSpecs,
                               HdBufferArrayUsageHint usageHint);
};
    

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_BUFFER_ARRAY_REGISTRY_H
