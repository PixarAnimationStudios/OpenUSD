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

#ifndef PXR_BASE_TRACE_EVENT_CONTAINER_H
#define PXR_BASE_TRACE_EVENT_CONTAINER_H

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"
#include "pxr/base/trace/event.h"

#include <iterator>
#include <new>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
/// \class TraceEventContainer
///
/// Holds TraceEvent instances. This container only allows appending events at 
/// the end and supports both forward and reverse iteration.
///
class TraceEventContainer {
    // Intrusively doubly-linked list node that provides contiguous storage
    // for events.  Only appending events and iterating held events is
    // supported.
    class _Node
    {
    public:
        using const_iterator = const TraceEvent *;

        // Allocate a new node that is able to hold capacity events.
        static _Node* New(size_t capacity);

        // Destroys the list starting at head, which must be the first node
        // in its list.
        static void DestroyList(_Node *head);

        // Join the last and first nodes of two lists to form a new list.
        static void Join(_Node *lhs, _Node *rhs);

        // Returns true if the node cannot hold any more events.
        bool IsFull() const { return _end == _sentinel; }

        const_iterator begin() const {
            const char *p = reinterpret_cast<const char *>(this);
            p += sizeof(_Node);
            return reinterpret_cast<const TraceEvent *>(p);
        }

        const_iterator end() const {
            return _end;
        }

        _Node *GetPrevNode() {
            return _prev;
        }

        const _Node *GetPrevNode() const {
            return _prev;
        }

        _Node *GetNextNode() {
            return _next;
        }

        const _Node *GetNextNode() const {
            return _next;
        }

        void ClaimEventEntry() {
            ++_end;
        }

        // Remove this node from the linked list to which it belongs.
        void Unlink();

    private:
        _Node(TraceEvent *end, size_t capacity);
        ~_Node();

    private:
        union {
            struct {
                TraceEvent *_end;
                TraceEvent *_sentinel;
                _Node *_prev;
                _Node *_next;
            };
            // Ensure that _Node is aligned to at least the alignment of
            // TraceEvent.
            alignas(TraceEvent) char _unused;
        };
    };

public:
    ////////////////////////////////////////////////////////////////////////////
    /// \class const_iterator
    /// Bidirectional iterator of TraceEvents.
    ///
    class const_iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = const TraceEvent;
        using difference_type = int64_t;
        using pointer = const TraceEvent*;
        using reference = const TraceEvent&;

        reference operator*() {
            return *_event;
        }

        pointer operator->() {
            return _event;
        }

        bool operator !=(const const_iterator& other) const {
            return !operator==(other);
        }

        bool operator == (const const_iterator& other) const {
            return _event == other._event;
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

        const_iterator& operator --() {
            Reverse();
            return *this;
        }

        const_iterator operator --(int) {
            const_iterator result(*this);
            Reverse();
            return result;
        }

    private:
        const_iterator(const _Node *node, const TraceEvent *event)
            : _node(node)
            , _event(event)
        {}

        void Advance() {
            ++_event;
            if (_event == _node->end() && _node->GetNextNode()) {
                _node = _node->GetNextNode();
                _event = _node->begin();
            }
        }

        void Reverse() {
            if (_event == _node->begin()) {
                _node = _node->GetPrevNode();
                _event = _node->end();
            }
            --_event;
        }

        const _Node *_node;
        const TraceEvent *_event;

        friend class TraceEventContainer;
    };

    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    /// Constructor.
    TraceEventContainer();

    /// Move Constructor.
    TraceEventContainer(TraceEventContainer&&);

    /// Move Assignment.
    TraceEventContainer& operator=(TraceEventContainer&&);

    // No copies
    TraceEventContainer(const TraceEventContainer&) = delete;
    TraceEventContainer& operator=(const TraceEventContainer&) = delete;

    TRACE_API
    ~TraceEventContainer();

    /// \name Subset of stl container interface.
    /// @{
    template < class... Args>
    TraceEvent& emplace_back(Args&&... args) {
        TraceEvent *event =
            new (_nextEvent++) TraceEvent(std::forward<Args>(args)...);
        _back->ClaimEventEntry();
        if (_back->IsFull()) {
            Allocate();
        }
        return *event;
    }

    const_iterator begin() const { 
        return const_iterator(_front, _front ? _front->begin() : nullptr);
    }

    const_iterator end() const { 
        return const_iterator(_back, _back ? _back->end() : nullptr);
    }

    const_reverse_iterator rbegin() const { 
        return const_reverse_iterator(end());
    }

    const_reverse_iterator rend() const { 
        return const_reverse_iterator(begin());
    }

    bool empty() const { return begin() == end(); }
    /// @}

    /// Append the events in \p other to the end of this container. This takes 
    /// ownership of the events that were in \p other.
    TRACE_API void Append(TraceEventContainer&& other);

private:
    // Allocates a new block of memory for TraceEvent items.
    TRACE_API void Allocate();

    // Points to where the next event should be constructed.
    TraceEvent* _nextEvent;
    _Node* _front;
    _Node* _back;
    size_t _blockSizeBytes;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TRACE_EVENT_CONTAINER_H
