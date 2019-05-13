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

#include "pxr/pxr.h"

#include "pxr/base/trace/eventContainer.h"
#include "pxr/base/tf/diagnostic.h"

#include <cstdlib>
#include <new>

PXR_NAMESPACE_OPEN_SCOPE

TraceEventContainer::TraceEventContainer()
    : _front(nullptr)
    , _back(nullptr)
    , _blockSizeBytes(512)
{}

TraceEventContainer::~TraceEventContainer()
{
    _Node::DestroyList(_front);
}

TraceEventContainer::TraceEventContainer(TraceEventContainer&& other)
    : _front(other._front)
    , _back(other._back)
    , _blockSizeBytes(other._blockSizeBytes)
{
    other._back = nullptr;
    other._front = nullptr;
}

TraceEventContainer& TraceEventContainer::operator=(TraceEventContainer&& other)
{
    TraceEventContainer temp(std::move(other));

    using std::swap;
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
    _Node::Join(_back, other._front);
    _back = other._back;
    other._front = nullptr;
    other._back = nullptr;
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
    _blockSizeBytes *= 2;
}

TraceEventContainer::_Node *
TraceEventContainer::_Node::New(size_t capacity)
{
    void *p = malloc(sizeof(_Node)+sizeof(TraceEvent)*capacity);
    return new (p) _Node(capacity);
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

TraceEventContainer::_Node::_Node(size_t capacity)
    : _prev(nullptr)
    , _next(nullptr)
    , _size(0)
    , _capacity(capacity)
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

PXR_NAMESPACE_CLOSE_SCOPE
