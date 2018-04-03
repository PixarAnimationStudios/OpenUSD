//
// Copyright 2018 Pixar
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

#ifndef TRACE_COLLECTION_H
#define TRACE_COLLECTION_H

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"
#include "pxr/base/trace/event.h"
#include "pxr/base/trace/eventList.h"
#include "pxr/base/trace/threads.h"

#include "pxr/base/tf/mallocTag.h"

#include <map>

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
///
/// \class TraceCollection
///
/// This class owns lists of TraceEvent instances per thread, and allows 
/// read access to them.
///
class TraceCollection {
public:
    TF_MALLOC_TAG_NEW("Trace", "TraceCollection");

    using This = TraceCollection;

    using EventList = TraceEventList;
    using EventListPtr = std::unique_ptr<EventList>;

    /// Constructor.
    TraceCollection() = default;

    /// Move constructor.
    TraceCollection(TraceCollection&&) = default;

    /// Move assignement operator.
    TraceCollection& operator=(TraceCollection&&) = default;

    // Collections should not be copied because TraceEvents contain 
    // pointers to elements in the Key cache.
    TraceCollection(const TraceCollection&) = delete;
    TraceCollection& operator=(const TraceCollection&) = delete;


    /// Appends \p events to the collection. The collection will 
    /// take ownership of the data.
    TRACE_API void AddToCollection(const TraceThreadId& id, EventListPtr&& events);

    ////////////////////////////////////////////////////////////////////////
    ///
    /// \class Visitor
    ///
    /// This interface provides a way to access data a TraceCollection.
    ///
    class Visitor {
    public:
        /// Destructor
        TRACE_API virtual ~Visitor();

        /// Called at the beginning of the an iteration.
        virtual void OnBeginCollection() = 0;

        /// Called at the end of the an iteration.
        virtual void OnEndCollection() = 0;

        /// Called before the first event of from the thread with 
        /// \p threadId is encountered.
        virtual void OnBeginThread(const TraceThreadId& threadId) = 0;

        /// Called after the last event of from the thread with 
        /// \p threadId is encountered.
        virtual void OnEndThread(const TraceThreadId& threadId) = 0;

        /// Called before an event with \p categoryId is visited. If the 
        /// return value is false, the event will be visited.
        virtual bool AcceptsCategory(TraceCategoryId categoryId) = 0;

        /// Called for every event \p event with \p key on thread 
        /// \p threadId if AcceptsCategory returns true.
        virtual void OnEvent(
            const TraceThreadId& threadId, 
            const TfToken& key, 
            const TraceEvent& event) = 0;
    };

    /// Iterates over the events of the collection and calls the \p visitor 
    /// callbacks.
    TRACE_API void Iterate(Visitor& visitor) const;

private:
    using EventTable = std::map<TraceThreadId, EventListPtr>;

    EventTable _eventsPerThread;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TRACE_COLLECTION_H