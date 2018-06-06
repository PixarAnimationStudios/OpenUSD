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

#include "pxr/base/trace/aggregateTreeBuilder.h"

#include "pxr/pxr.h"

#include "pxr/base/trace/collection.h"

#include <stack>

PXR_NAMESPACE_OPEN_SCOPE


void
Trace_AggregateTreeBuilder::AddCollectionDataToTree(
    TraceAggregateTree* aggregateTree, const TraceCollection& collection)
{
    Trace_AggregateTreeBuilder builder(aggregateTree, collection);
    builder._CreateAggregateNodes();
    builder._ProcessCounters(collection);
}

Trace_AggregateTreeBuilder::Trace_AggregateTreeBuilder(
    TraceAggregateTree* aggregateTree, const TraceCollection& collection)
    : _aggregateTree(aggregateTree)
    , _tree(TraceEventTree::New(collection))
{
}

void
Trace_AggregateTreeBuilder::_ProcessCounters(const TraceCollection& collection)
{
    collection.Iterate(*this);
    _aggregateTree->GetRoot()->CalculateInclusiveCounterValues();
}

void
Trace_AggregateTreeBuilder::_CreateAggregateNodes()
{
    using TreeIt = std::pair<TraceEventNodePtr, size_t>;
    std::stack<TreeIt> treeStack;
    std::stack<TraceAggregateNodePtr> aggStack;

    // Prime the aggregate stack with the root node.
    aggStack.push(_aggregateTree->GetRoot());

    // Prime the stack with the children of the root. These are the node that
    // represent threads.
    for (TraceEventNodeRefPtrVector::const_reverse_iterator it =
            _tree->GetRoot()->GetChildrenRef().rbegin(); 
            it != _tree->GetRoot()->GetChildrenRef().rend(); ++it) {
        treeStack.push(std::make_pair(*it, 0));
    }
    
    // A valid id needed for node creation.
    TraceAggregateNode::Id id = TraceAggregateNode::Id(TraceThreadId());

    while (!treeStack.empty()) {
        TreeIt it = treeStack.top();
        treeStack.pop();

        // The first time a node is visited, add it to the aggregate tree.
        if (it.second == 0) {
            const TraceEvent::TimeStamp duration =
                it.first->GetEndTime() - it.first->GetBeginTime();

            if (duration > 0 && aggStack.size() > 1) {
                _aggregateTree->_eventTimes[it.first->GetKey()] += duration;
            }

            TraceAggregateNodePtr newNode = aggStack.top()->Append(
                id, it.first->GetKey(), duration);
            aggStack.push(newNode);
        }
        // When there are no more children to visit, pop the aggregate tree
        // stack.
        if (it.second >= it.first->GetChildrenRef().size()) {
            aggStack.pop();
        } else {
            // Visit the current child and then the next child.
            treeStack.push(std::make_pair(it.first, it.second+1));
            treeStack.push(
                std::make_pair(it.first->GetChildrenRef()[it.second], 0));
        }
    }
}

void
Trace_AggregateTreeBuilder::OnBeginCollection()
{}

void
Trace_AggregateTreeBuilder::OnEndCollection()
{}

void
Trace_AggregateTreeBuilder::OnBeginThread(const TraceThreadId& threadId)
{}

void
Trace_AggregateTreeBuilder::OnEndThread(const TraceThreadId& threadId)
{}

bool
Trace_AggregateTreeBuilder::AcceptsCategory(TraceCategoryId categoryId) 
{
    return true;
}

void
Trace_AggregateTreeBuilder::OnEvent(
    const TraceThreadId& threadIndex, 
    const TfToken& key, 
    const TraceEvent& e)
{
    switch(e.GetType()) {
        case TraceEvent::EventType::CounterDelta:
        case TraceEvent::EventType::CounterValue:
            _OnCounterEvent(threadIndex, key, e);
            break;
        default:
            break;
    }
}

void
Trace_AggregateTreeBuilder::_OnCounterEvent(
    const TraceThreadId& threadIndex, 
    const TfToken& key, 
    const TraceEvent& e)
{
    bool isDelta = false;
    switch (e.GetType()) {
        case TraceEvent::EventType::CounterDelta: isDelta = true; break;
        case TraceEvent::EventType::CounterValue: break;
        default: return;
    }

    // Compute the total counter value
    TraceAggregateTree::CounterMap::iterator it =
        _aggregateTree->_counters.insert(
            std::make_pair(key, 0.0)).first;

    if (isDelta) {
        it->second += e.GetCounterValue();
    } else {
        it->second = e.GetCounterValue();
    }

    // Insert the counter index into the map, if one does not
    // already exist. If no counter index existed in the map, 
    // increment to the next available counter index.
    std::pair<TraceAggregateTree::_CounterIndexMap::iterator, bool> res =
        _aggregateTree->_counterIndexMap.insert(
            std::make_pair(key, _aggregateTree->_counterIndex));
    if (res.second) {
        ++_aggregateTree->_counterIndex;
    }

    // It only makes sense to store delta values in the specific nodes at 
    // the moment. This might need to be revisted in the future.
    if (isDelta) {
        // Set the counter value on the current node.
    
        TraceAggregateNodePtr node =
            _FindAggregateNode(threadIndex, e.GetTimeStamp());
        if (node) {
            node->AppendExclusiveCounterValue(res.first->second, e.GetCounterValue());
            node->AppendInclusiveCounterValue(res.first->second, e.GetCounterValue());
        }
    }
}

TraceAggregateNodePtr
 Trace_AggregateTreeBuilder::_FindAggregateNode(
        const TraceThreadId& threadId, const TraceEvent::TimeStamp ts) const
{
    // Find the root node of the thread.
    const TraceEventNodeRefPtrVector& threadNodeList =
        _tree->GetRoot()->GetChildrenRef();
    TfToken threadKey(threadId.ToString());
    TraceEventNodeRefPtrVector::const_iterator it =
        std::find_if(threadNodeList.begin(), threadNodeList.end(), 
        [&threadKey](const TraceEventNodeRefPtr& node) {
            return node->GetKey() == threadKey;
        });
    if (it == threadNodeList.end()) {
        return nullptr;
    }

    // Construct a sequence of node names from the thread root node to the
    // lowest node in the tree which contains this timestamp.
    TraceEventNodePtr node = *it;
    std::vector<TfToken> path;
    while (true) {
        path.push_back(node->GetKey());
        // Find the first child which contains this timestamp
        TraceEventNodeRefPtrVector::const_iterator childIt = 
            std::lower_bound(
                node->GetChildrenRef().begin(),
                node->GetChildrenRef().end(), ts, 
                []( const TraceEventNodeRefPtr& node,
                    TraceEvent::TimeStamp ts) {
                    return node->GetEndTime() < ts;
                });
        if (childIt == node->GetChildrenRef().end()) {
            break;
        } else {
            node = *childIt;
        }
    }

    // Use the sequence of node names to find the corresponding node in the
    // aggregate tree.
    TraceAggregateNodePtr aggNode = _aggregateTree->GetRoot();
    for (const TfToken& name : path) {
        TraceAggregateNodePtr child = aggNode->GetChild(name);
        if (!child) {
            return nullptr;
        }
        aggNode = child;
    }
    return aggNode;
}

PXR_NAMESPACE_CLOSE_SCOPE