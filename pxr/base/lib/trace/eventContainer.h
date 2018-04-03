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

#ifndef TRACE_EVENT_CONTAINER_H
#define TRACE_EVENT_CONTAINER_H

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"
#include "pxr/base/trace/event.h"

#include <list>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
/// \class TraceEventContainer
///
/// Holds TraceEvent instances. This container only allows appending events at 
/// the end and only supports forward iteration.
///
class TraceEventContainer {
    using InnerStorage = std::vector<TraceEvent>;
    using OuterStorage = std::list<InnerStorage>;

public:
    ////////////////////////////////////////////////////////////////////////////
    /// \class const_iterator
    /// Forward iterator of TraceEvents.
    ///
    class const_iterator {
        using Inner = typename InnerStorage::const_iterator;
        using Outer = typename OuterStorage::const_iterator;

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = const TraceEvent;
        using difference_type = int64_t;
        using pointer = const TraceEvent*;
        using reference = const TraceEvent&;
        
        reference operator*() {
            return *_inner;
        }

        pointer operator->() {
            return &(*_inner);
        }

        bool operator !=(const const_iterator& other) const {
            return !operator==(other);
        }

        bool operator == (const const_iterator& other) const {
            return _outer == other._outer && _inner == other._inner;
        }

        const_iterator& operator ++() {
            Advance();
            return *this;
        }

        const_iterator operator ++(int) {
            const_iterator result(*this);
            Advance();
            return result;
        }

    private:
        const_iterator(Outer outer, Outer outerEnd, Inner inner)
            : _inner(inner)
            , _outer(outer)
            , _outerEnd(outerEnd)
             {}
        const_iterator(Outer outer, Outer outerEnd)
            : _inner()
            , _outer(outer)
            , _outerEnd(outerEnd)
             {}

        void Advance() {
            ++_inner;
            if (_inner == _outer->end()) {
                ++_outer;
                if (!IsEnd()) {
                    _inner = _outer->begin();
                } else {
                    _inner = Inner();
                }
            }
        }

        bool IsEnd() const {
            return _outerEnd == _outer;
        }

        Inner _inner;
        Outer _outer;
        Outer _outerEnd;

        friend class TraceEventContainer;
    };

    /// Constructor.
    TraceEventContainer();

    /// Move Constructor.
    TraceEventContainer(TraceEventContainer&&);

    /// Move Assignment.
    TraceEventContainer& operator=(TraceEventContainer&&);

    // No copies
    TraceEventContainer(const TraceEventContainer&) = delete;
    TraceEventContainer& operator=(const TraceEventContainer&) = delete;


    /// \name Subset of stl container interface.
    /// @{
    template < class... Args>
    void emplace_back(Args&&... args) {
        if (ARCH_UNLIKELY(!_back || _back->size() == _back->capacity())) {
            Allocate();
        }
        _back->emplace_back(std::forward<Args>(args)...);
    }

    const TraceEvent& back() const { return _back->back(); }

    const_iterator begin() const { 
        return const_iterator(_outer.begin(), 
            _outer.end(), 
            _outer.begin()->begin()); 
    }

    const_iterator end() const { 
        return const_iterator(_outer.end(), _outer.end()); 
    }

    bool empty() const { return _outer.empty();}
    /// @}

    /// Append the events in \p other to the end of this container. This takes 
    /// ownership of the events that were in \p other.
    TRACE_API void Append(TraceEventContainer&& other);

private:
    // Allocates a new block of memory for TraceEvent items.
    TRACE_API void Allocate();

    InnerStorage* _back;
    OuterStorage _outer;
    size_t _blockSizeBytes;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TRACE_EVENT_CONTAINER_H