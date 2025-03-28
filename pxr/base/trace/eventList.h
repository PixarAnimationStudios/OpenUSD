//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TRACE_EVENT_LIST_H
#define PXR_BASE_TRACE_EVENT_LIST_H

#include "pxr/pxr.h"

#include "pxr/base/trace/dataBuffer.h"
#include "pxr/base/trace/dynamicKey.h"
#include "pxr/base/trace/event.h"
#include "pxr/base/trace/eventContainer.h"

#include <list>
#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
/// \class TraceEventList
///
/// This class represents an ordered collection of TraceEvents and the
/// TraceDynamicKeys and data that the events reference.
///
class TraceEventList {
public:
    /// Constructor.
    TRACE_API TraceEventList();

    /// Move Constructor.
    TraceEventList(TraceEventList&&) = default;

    /// Move Assignment.
    TraceEventList& operator=(TraceEventList&&) = default;

    // No copies
    TraceEventList(const TraceEventList&) = delete;
    TraceEventList& operator=(const TraceEventList&) = delete;

    /// \name Iterator support.
    /// @{
    using const_iterator = TraceEventContainer::const_iterator;
    const_iterator begin() const { return _events.begin();}
    const_iterator end() const { return _events.end();}

    using const_reverse_iterator = TraceEventContainer::const_reverse_iterator;
    const_reverse_iterator rbegin() const { return _events.rbegin();}
    const_reverse_iterator rend() const { return _events.rend();}
    /// @}

    /// Returns whether there are any events in the list.
    bool IsEmpty() const { return _events.empty();}

    /// Construct a TraceEvent at the end on the list.
    /// Returns a reference to the newly constructed event.
    template < class... Args>
    const TraceEvent& EmplaceBack(Args&&... args) {
        return _events.emplace_back(std::forward<Args>(args)...);
    }

    /// For speed the TraceEvent class holds a pointer to a TraceStaticKeyData. 
    /// This method creates a key which can be referenced by events in this 
    /// container. Returns a TraceKey which will remain valid for the lifetime
    /// of the container.
    TraceKey CacheKey(const TraceDynamicKey& key) {
        KeyCache::const_iterator it = _caches.front().insert(key).first;
        return TraceKey(it->GetData());
    }

    /// Appends the given list to the end of this list. This object will take 
    /// ownership of the events and keys in the appended list.
    TRACE_API void Append(TraceEventList&& other);

    /// Copy data to the buffer and return a pointer to the cached data that is 
    /// valid for the lifetime of the Eventlist.
    template < typename T>
    decltype(std::declval<TraceDataBuffer>().StoreData(std::declval<T>()))
    StoreData(const T& value) { 
        return _dataCache.StoreData(value); 
    }

private:

    TraceEventContainer _events;

    // For speed the TraceEvent class holds a pointer to a TraceStaticKeyData.
    // For some events (ones not created by TRACE_FUNCTION and
    // TRACE_SCOPE macros), we need to hold onto the TraceDynamicKey to
    // keep the reference valid. Using std::list to avoid reallocation,
    // keep reference valid by keeping things stable in memory.
    using KeyCache = 
        std::unordered_set<TraceDynamicKey, TraceDynamicKey::HashFunctor>;
    std::list<KeyCache> _caches;

    TraceDataBuffer _dataCache;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_BASE_TRACE_EVENT_LIST_H
