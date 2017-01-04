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
#include "pxr/usd/pcp/iterator.h"

#include "pxr/usd/pcp/arc.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/pcp/primIndex_Graph.h"
#include "pxr/usd/pcp/propertyIndex.h"
#include "pxr/usd/pcp/types.h"

PcpNodeIterator::PcpNodeIterator()
    : _graph(0)
    , _nodeIdx(PCP_INVALID_INDEX)
{
    // Do nothing
}

PcpNodeIterator::PcpNodeIterator(PcpPrimIndex_Graph* graph, size_t nodeIdx)
    : _graph(graph)
    , _nodeIdx(nodeIdx)
{
    // Do nothing
}

void 
PcpNodeIterator::increment()
{
    if (!_graph) {
        TF_CODING_ERROR("Cannot increment invalid iterator");
        return;
    }

    ++_nodeIdx;
}

void 
PcpNodeIterator::decrement()
{
    if (!_graph) {
        TF_CODING_ERROR("Cannot decrement invalid iterator");
        return;
    }

    --_nodeIdx;
}

void 
PcpNodeIterator::advance(difference_type n)
{
    if (!_graph) {
        TF_CODING_ERROR("Cannot advance invalid iterator");
        return;
    }

    _nodeIdx += n;
}

PcpNodeIterator::difference_type 
PcpNodeIterator::distance_to(const PcpNodeIterator& other) const
{
    if (!_graph || !other._graph) {
        TF_CODING_ERROR("Invalid iterator");
        return 0;
    }

    if (_graph != other._graph) {
        TF_CODING_ERROR("Cannot compute distance for iterators "
                        "from different graphs");
        return 0;
    }

    return (difference_type)(other._nodeIdx) - _nodeIdx;
}

bool 
PcpNodeIterator::equal(const PcpNodeIterator& other) const
{
    return _graph == other._graph &&
        _nodeIdx == other._nodeIdx;
}

PcpNodeIterator::reference
PcpNodeIterator::dereference() const
{
    return PcpNodeRef(_graph, _nodeIdx);
}

////////////////////////////////////////////////////////////

PcpPrimIterator::PcpPrimIterator()
    : _primIndex(NULL)
    , _pos(PCP_INVALID_INDEX)
{
    // Do nothing
}

PcpPrimIterator::PcpPrimIterator(
    const PcpPrimIndex* primIndex, size_t pos)
    : _primIndex(primIndex)
    , _pos(pos)
{
    // Do nothing
}

void 
PcpPrimIterator::increment()
{
    if (!_primIndex) {
        TF_CODING_ERROR("Cannot increment invalid iterator");
        return;
    }

    ++_pos;
}

void 
PcpPrimIterator::decrement()
{
    if (!_primIndex) {
        TF_CODING_ERROR("Cannot decrement invalid iterator");
        return;
    }

    --_pos;
}

void 
PcpPrimIterator::advance(difference_type n)
{
    if (!_primIndex) {
        TF_CODING_ERROR("Cannot advance invalid iterator");
        return;
    }

    _pos += n;
}

PcpPrimIterator::difference_type 
PcpPrimIterator::distance_to(const PcpPrimIterator& other) const
{
    if (!_primIndex || !other._primIndex) {
        TF_CODING_ERROR("Invalid iterator");
        return 0;
    }

    if (_primIndex != other._primIndex) {
        TF_CODING_ERROR("Cannot compute distance for iterators "
                        "from different prim indexes.");
        return 0;
    }

    return (difference_type)(other._pos) - _pos;
}

bool 
PcpPrimIterator::equal(const PcpPrimIterator& other) const
{
    return _primIndex == other._primIndex && _pos == other._pos;
}

PcpPrimIterator::reference 
PcpPrimIterator::dereference() const
{
    return _primIndex->_graph->GetSdSite(_primIndex->_primStack[_pos]);
}

PcpNodeRef
PcpPrimIterator::GetNode() const
{
    return _primIndex->_graph->GetNode(_primIndex->_primStack[_pos]);
}

Pcp_SdSiteRef
PcpPrimIterator::_GetSiteRef() const
{
    return _primIndex->_graph->GetSiteRef(_primIndex->_primStack[_pos]);
}

////////////////////////////////////////////////////////////

PcpPropertyIterator::PcpPropertyIterator()
    : _propertyIndex(NULL)
    , _pos(0)
{
}

PcpPropertyIterator::PcpPropertyIterator(
    const PcpPropertyIndex& index, size_t pos)
    : _propertyIndex(&index)
    , _pos(pos)
{
}

void 
PcpPropertyIterator::increment()
{
    if (!_propertyIndex) {
        TF_CODING_ERROR("Cannot increment invalid iterator");
        return;
    }

    ++_pos;
}

void 
PcpPropertyIterator::decrement()
{
    if (!_propertyIndex) {
        TF_CODING_ERROR("Cannot decrement invalid iterator");
        return;
    }

    --_pos;
}

void 
PcpPropertyIterator::advance(difference_type n)
{
    if (!_propertyIndex) {
        TF_CODING_ERROR("Cannot advance invalid iterator");
        return;
    }

    _pos += n;
}

PcpPropertyIterator::difference_type 
PcpPropertyIterator::distance_to(const PcpPropertyIterator& other) const
{
    if (!_propertyIndex || !other._propertyIndex) {
        TF_CODING_ERROR("Invalid iterator");
        return 0;
    }

    if (_propertyIndex != other._propertyIndex) {
        TF_CODING_ERROR("Cannot compute distance for iterators "
                        "from different property indexes");
        return 0;
    }

    return (difference_type)(other._pos) - _pos;
}

bool 
PcpPropertyIterator::equal(const PcpPropertyIterator& other) const
{
    return _propertyIndex == other._propertyIndex && 
        _pos == other._pos;
}

PcpPropertyIterator::reference 
PcpPropertyIterator::dereference() const
{
    return _propertyIndex->_propertyStack[_pos].propertySpec;
}

PcpNodeRef 
PcpPropertyIterator::GetNode() const
{
    return _propertyIndex->_propertyStack[_pos].originatingNode;
}

bool 
PcpPropertyIterator::IsLocal() const
{
    return _pos < _propertyIndex->GetNumLocalSpecs();
}
