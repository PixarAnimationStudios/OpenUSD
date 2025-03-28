//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/trace/eventContainer.h"
#include "pxr/base/tf/diagnostic.h"

#include <cstdlib>
#include <new>

PXR_NAMESPACE_OPEN_SCOPE

TraceEventContainer::TraceEventContainer()
    : _nextEvent(nullptr)
    , _front(nullptr)
    , _back(nullptr)
    , _blockSizeBytes(512)
{
    Allocate();
}

TraceEventContainer::~TraceEventContainer()
{
    _Node::DestroyList(_front);
}

TraceEventContainer::TraceEventContainer(TraceEventContainer&& other)
    : TraceEventContainer()
{
    using std::swap;
    swap(_nextEvent, other._nextEvent);
    swap(_back, other._back);
    swap(_front, other._front);
    _blockSizeBytes = other._blockSizeBytes;
}

TraceEventContainer& TraceEventContainer::operator=(TraceEventContainer&& other)
{
    TraceEventContainer temp(std::move(other));

    using std::swap;
    swap(_nextEvent, temp._nextEvent);
    swap(_back, temp._back);
    swap(_front, temp._front);

    return *this;
}

void 
TraceEventContainer::Append(TraceEventContainer&& other)
{
    if (other.empty()) {
        return;
    }
    if (empty()) {
        *this = std::move(other);
        return;
    }

    // In the interest of keeping the iterator implementation simple, we
    // cannot allow empty internal nodes in the list.  By construction, there
    // can be at most one empty node at the tail of each list.
    if (_back->begin() == _back->end()) {
        _Node *empty = _back;
        _back = _back->GetPrevNode();
        empty->Unlink();
        _Node::DestroyList(empty);
    }

    _Node::Join(_back, other._front);
    _back = other._back;
    _nextEvent = other._nextEvent;
    other._nextEvent = nullptr;
    other._front = nullptr;
    other._back = nullptr;
    other.Allocate();
}

void
TraceEventContainer::Allocate()
{
    size_t capacity = (_blockSizeBytes - sizeof(_Node)) / sizeof(TraceEvent);
    _Node *node = _Node::New(capacity);
    if (!_front) {
        _front = node;
    }
    else {
        _Node::Join(_back, node);
    }
    _back = node;

    char *p = reinterpret_cast<char *>(node);
    p += sizeof(_Node);
    _nextEvent = reinterpret_cast<TraceEvent *>(p);
    _blockSizeBytes *= 2;
}

TraceEventContainer::_Node *
TraceEventContainer::_Node::New(size_t capacity)
{
    void *p = malloc(sizeof(_Node)+sizeof(TraceEvent)*capacity);
    TraceEvent *eventEnd = reinterpret_cast<TraceEvent*>(
        reinterpret_cast<char *>(p) + sizeof(_Node));
    return new (p) _Node(eventEnd, capacity);
}

void
TraceEventContainer::_Node::DestroyList(_Node *head)
{
    // The node passed to DestroyList (if any) must be the first node.
    TF_DEV_AXIOM(!head || !head->_prev);

    while (head) {
        _Node *next = head->_next;
        head->~_Node();
        free(head);
        head = next;
    }
}

TraceEventContainer::_Node::_Node(TraceEvent *end, size_t capacity)
    : _end(end)
    , _sentinel(end+capacity)
    , _prev(nullptr)
    , _next(nullptr)
{
}

TraceEventContainer::_Node::~_Node()
{
    for (const TraceEvent &ev : *this) {
        ev.~TraceEvent();
    }
}

void
TraceEventContainer::_Node::Join(_Node *lhs, _Node *rhs)
{
    TF_DEV_AXIOM(lhs);
    TF_DEV_AXIOM(rhs);
    // lhs must be the last node in its list and rhs must be the first.
    TF_DEV_AXIOM(!lhs->_next);
    TF_DEV_AXIOM(!rhs->_prev);

    lhs->_next = rhs;
    rhs->_prev = lhs;
}

inline void
TraceEventContainer::_Node::Unlink()
{
    if (_prev) {
        _prev->_next = _next;
    }
    if (_next) {
        _next->_prev = _prev;
    }
    _prev = nullptr;
    _next = nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
