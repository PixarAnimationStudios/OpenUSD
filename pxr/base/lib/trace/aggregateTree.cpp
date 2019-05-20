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

#include "pxr/base/trace/aggregateTree.h"

#include "pxr/pxr.h"

#include "pxr/base/trace/aggregateTreeBuilder.h"
#include "pxr/base/trace/collection.h"
#include "pxr/base/trace/eventTree.h"

#include <stack>

PXR_NAMESPACE_OPEN_SCOPE

TraceAggregateTree::TraceAggregateTree()
{
    Clear();
}

void
TraceAggregateTree::Clear()
{
    _root = TraceAggregateNode::New();
    _eventTimes.clear();
    _counters.clear();
    _counterIndexMap.clear();
    _counterIndex = 0;
}

int
TraceAggregateTree::GetCounterIndex(const TfToken &key) const
{
    _CounterIndexMap::const_iterator it = _counterIndexMap.find(key);
    return it != _counterIndexMap.end() ? it->second : -1;
}

bool
TraceAggregateTree::AddCounter(const TfToken &key, int index, double totalValue)
{
    // Don't add counters with invalid indices
    if (!TF_VERIFY(index >= 0)) {
        return false;
    }

    // We don't expect a counter entry to exist with this key
    if (!TF_VERIFY(_counters.find(key) == _counters.end())) {
        return false;
    }

    // We also don't expect the given index to be used by a different counter
    for (const _CounterIndexMap::value_type& it : _counterIndexMap) {
        if (!TF_VERIFY(it.second != index)) {
            return false;
        }
    }

    // Add the new counter
    _counters[key] = totalValue;
    _counterIndexMap[key] = index;

    return true;
}

void
TraceAggregateTree::Append(const TraceCollection& collection) {
    Trace_AggregateTreeBuilder::AddCollectionDataToTree(this, collection);
}

PXR_NAMESPACE_CLOSE_SCOPE