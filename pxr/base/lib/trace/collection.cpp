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

#include "pxr/base/trace/collection.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

void TraceCollection::AddToCollection(const TraceThreadId& id, EventListPtr&& events)
{
    EventTable::iterator it = _eventsPerThread.find(id);
    if (it == _eventsPerThread.end()) {
        _eventsPerThread.emplace(id, std::move(events));
    } else {
        it->second->Append(std::move(*events));
    }
}

template <class I>
void TraceCollection::_IterateEvents(Visitor& visitor,
    KeyTokenCache& cache,
    const TraceThreadId& threadIndex, 
    I begin,
    I end) const {

    for (I iter = begin; 
        iter != end; ++iter){
        const TraceEvent& e = *iter;
        if (visitor.AcceptsCategory(e.GetCategory())) {
            // Create the token from the hash using a cache because there 
            // are likely to be many duplicate keys.
            KeyTokenCache::const_iterator it = cache.find(e.GetKey());
            if (it == cache.end()) {
                it = cache.insert(
                    std::make_pair(e.GetKey(),
                        TfToken(e.GetKey()._ptr->GetString()))).first;
            }
            visitor.OnEvent(threadIndex, it->second, e);
        }
    }
}

void 
TraceCollection::_Iterate(Visitor& visitor, bool doReverse) const {
    KeyTokenCache cache;
    visitor.OnBeginCollection();
    for (const EventTable::value_type& i : _eventsPerThread) {
        const TraceThreadId& threadIndex = i.first;
        const EventListPtr &events = i.second;
        visitor.OnBeginThread(threadIndex);
        
        if (doReverse) {
            _IterateEvents(visitor, cache, 
                threadIndex, events->rbegin(), events->rend());
        }
        else {
            _IterateEvents(visitor, cache, 
                threadIndex, events->begin(), events->end());
        }

        visitor.OnEndThread(threadIndex);
    }
    visitor.OnEndCollection();
}

void 
TraceCollection::Iterate(Visitor& visitor) const {
    _Iterate(visitor, false);
}

void 
TraceCollection::ReverseIterate(Visitor& visitor) const {
    _Iterate(visitor, true);
}

TraceCollection::Visitor::~Visitor() {}

PXR_NAMESPACE_CLOSE_SCOPE
