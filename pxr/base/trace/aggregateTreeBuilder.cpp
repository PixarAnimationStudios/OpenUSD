//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/trace/aggregateTreeBuilder.h"

#include "pxr/pxr.h"

#include "pxr/base/trace/collection.h"

#include <stack>

PXR_NAMESPACE_OPEN_SCOPE


void
Trace_AggregateTreeBuilder::AddEventTreeToAggregate(
    TraceAggregateTree* aggregateTree,
    const TraceEventTreeRefPtr& eventTree,
    const TraceCollection& collection)
{
    Trace_AggregateTreeBuilder builder(aggregateTree, eventTree);

    builder._CreateAggregateNodes();
    builder._ProcessCounters(collection);
}

Trace_AggregateTreeBuilder::Trace_AggregateTreeBuilder(
    TraceAggregateTree* aggregateTree, const TraceEventTreeRefPtr& eventTree)
    : _aggregateTree(aggregateTree)
    , _tree(eventTree)
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
    // A stack of these structures tracks our place in processing the EventNode
    // tree while we build the AggregateNode tree.
    //
    // Each entry contains a reference to the EventNode and its remaining
    // children we have yet to process.  We process children first to last,
    // updating `children` to be `children.subspan(1)` at each step.  Once the
    // children span is empty we pop the entry from the stack.
    //
    // We also track the corresponding aggregate tree node so we can append new
    // nodes to it.  This is how we build up the Aggregate tree as we process
    // the EventNodes.
    struct StackEntry {
        StackEntry(TraceEventNodeRefPtr const &node,
                   TfSpan<const TraceEventNodeRefPtr> children,
                   TraceAggregateNodeRefPtr &&aggNode)
            : node(node), children(children), aggNode(std::move(aggNode)) {}

        // Disallow rvalue node args since we store by-reference to avoid a
        // refcount hit.
        StackEntry(TraceEventNodeRefPtr &&node,
                   TfSpan<const TraceEventNodeRefPtr> children,
                   TraceAggregateNodeRefPtr &&aggNode) = delete;
        
        const TraceEventNodeRefPtr &node;
        TfSpan<const TraceEventNodeRefPtr> children;
        TraceAggregateNodeRefPtr aggNode;
    };
    
    std::stack<StackEntry, std::vector<StackEntry>> treeStack;

    {
        // Prime the stack with the root's direct children.  These are typically
        // the nodes that represent threads.
        const TraceAggregateNodePtr aggRoot = _aggregateTree->GetRoot();
        const TfSpan<const TraceEventNodeRefPtr>
            firstNodes = _tree->GetRoot()->GetChildrenRef();
        for (auto i = firstNodes.rbegin(); i != firstNodes.rend(); ++i) {
            const TraceEventNodeRefPtr &node = *i;
            treeStack.emplace(
                node, node->GetChildrenRef(),
                aggRoot->Append(node->GetKey(), node->GetTimeDuration()));
        }
    }
        
    while (!treeStack.empty()) {
        StackEntry *entry = &treeStack.top();

        // If this node has no children left, pop and continue.
        if (entry->children.empty()) {
            treeStack.pop();
            continue;
        }

        // Take the next child from the front of the span.
        const TraceEventNodeRefPtr &curNode = entry->children.front();
        entry->children = entry->children.subspan(1);

        const TraceEvent::TimeStamp duration = curNode->GetTimeDuration();
        if (duration > 0) {
            _aggregateTree->_eventTimes[curNode->GetKey()] += duration;
        }

        // If the child has children, push it on the stack.
        const TfSpan<const TraceEventNodeRefPtr>
            children = curNode->GetChildrenRef();
        if (!children.empty()) {
            treeStack.emplace(
                curNode, children,
                entry->aggNode->Append(curNode->GetKey(), duration));
        }
        else {
            // Otherwise, just blindly append or update the aggregate node.
            entry->aggNode->AppendBlind(curNode->GetKey(), duration);
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
    const TfSpan<const TraceEventNodeRefPtr>
        threadNodes = _tree->GetRoot()->GetChildrenRef();
    TfToken threadKey(threadId.ToString());
    auto it =
        std::find_if(threadNodes.begin(), threadNodes.end(), 
        [&threadKey](const TraceEventNodeRefPtr& node) {
            return node->GetKey() == threadKey;
        });
    if (it == threadNodes.end()) {
        return nullptr;
    }

    // Construct a sequence of node names from the thread root node to the
    // lowest node in the tree which contains this timestamp.
    TraceEventNodeRefPtr node = *it;
    std::vector<TfToken> path;
    while (true) {
        path.push_back(node->GetKey());
        // Find the first child which contains this timestamp
        auto childIt = 
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
