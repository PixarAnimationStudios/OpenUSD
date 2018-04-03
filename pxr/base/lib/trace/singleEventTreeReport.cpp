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

#include "pxr/base/trace/singleEventTreeReport.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

TraceSingleEventTreeReport::TraceSingleEventTreeReport() 
    : _root(TraceSingleEventNode::New())
{}

// Visitor interface
void
TraceSingleEventTreeReport::OnBeginCollection()
{

}

void
TraceSingleEventTreeReport::OnEndCollection()
{
    _threadStacks.clear();
    // Convert the counter delta values to absolute values;
    TraceSingleEventGraph::CounterMap counterValues;
    for (const TraceSingleEventGraph::CounterMap::value_type& c 
        : _counterDeltas) {

        double curValue = 0;
        for (const TraceSingleEventGraph::CounterValues::value_type& v 
            : c.second) {

            curValue += v.second;
            counterValues[c.first].insert(std::make_pair(v.first, curValue));
        }
    }
    _counterDeltas.clear();
    _graph = TraceSingleEventGraph::New(_root, std::move(counterValues));
}

bool
TraceSingleEventTreeReport::AcceptsCategory(TraceCategoryId id)
{
    return true;
}

void
TraceSingleEventTreeReport::OnBeginThread(const TraceThreadId& threadId)
{
    // Note, that TraceGetThreadId() returns the id of the current thread,
    // i.e. the reporting thread. Since we always report from the main
    // thread, we label the current thread "Main Thread" in the trace.
    _threadStacks[threadId] = _PendingNodeStack();
    {
        _threadStacks[threadId].emplace_back(
            TfToken(threadId.ToString()), TraceCategory::Default, 0);
    }
}

void
TraceSingleEventTreeReport::OnEndThread(const TraceThreadId& threadId)
{
    _ThreadStackMap::iterator it = _threadStacks.find(threadId);
    if (it != _threadStacks.end()) {
        _PendingNodeStack& stack = it->second;
        // Close any incomplete nodes.
        TraceSingleEventNodeRefPtr threadRoot;
        while (!stack.empty()) {
            // TODO: Incomplete events are treated as zero duration.
            stack.back().start = 0;
            threadRoot = stack.back().Close(0, /* separateEvents = */ true);
            stack.pop_back();
            if (!stack.empty()) {
                stack.back().children.push_back(threadRoot);
            }
        }
        threadRoot->SetBeginAndEndTimesFromChildren();
        _root->Append(threadRoot);
        _threadStacks.erase(it);
    }
}

void
TraceSingleEventTreeReport::OnEvent(
    const TraceThreadId& threadIndex, const TfToken& key, const TraceEvent& e) 
{
    switch(e.GetType()) {
        case TraceEvent::EventType::Begin:
            _OnBegin(threadIndex, key, e);
            break;
        case TraceEvent::EventType::End:
            _OnEnd(threadIndex, key, e);
            break;
        case TraceEvent::EventType::Counter:
            _OnCounter(threadIndex, key, e);
            break;
        case TraceEvent::EventType::Timespan:
            _OnTimespan(threadIndex, key, e);
            break;
        case TraceEvent::EventType::ScopeData:
            _OnData(threadIndex, key, e);
            break;
        case TraceEvent::EventType::Unknown:
            break;
    }
}

void
TraceSingleEventTreeReport::_OnBegin(
    const TraceThreadId& threadId, const TfToken& key, const TraceEvent& e)
{
    // For begin events, push a partial node
    _threadStacks[threadId].emplace_back(
        key, e.GetCategory(), e.GetTimeStamp());
}

void
TraceSingleEventTreeReport::_OnEnd(
    const TraceThreadId& threadId, const TfToken& key, const TraceEvent& e)
{
    // For an end event, create the node and pop the stack
    _PendingNodeStack& stack = _threadStacks[threadId];
    if (stack.back().key == key) {
        TraceSingleEventNodeRefPtr node = 
            stack.back().Close(e.GetTimeStamp(), /* separateEvents = */ true);
        stack.pop_back();
        if (!stack.empty()) {
            stack.back().children.push_back(node);
        }
    } else {
        // If we encounter an end event that does not match a begin 
        // event it means its from an incomplete scope. We need to 
        // insert a new node and take any pending children from the 
        // top of the stack and parent them under this new node.
        
        // TODO: Incomplete events are treated as zero duration.
        _PendingSingleEventNode pending(key, e.GetCategory(), 0);
        swap(pending.children, stack.back().children);
        swap(pending.attributes, stack.back().attributes);
        TraceSingleEventNodeRefPtr node =
            pending.Close(0, /* separateEvents = */ true);
        stack.back().children.push_back(node);
    }
}

void 
TraceSingleEventTreeReport::_OnTimespan(
    const TraceThreadId& threadId, const TfToken& key, const TraceEvent& e)
{
    // If we encounter a timespan, we need to insert a new node and take any
    // pending children and data from the top of the stack that are within the
    // duration and add them to this new node.
    
   _PendingNodeStack& stack = _threadStacks[threadId];

    const TraceEvent::TimeStamp start = e.GetStartTimeStamp();
    const TraceEvent::TimeStamp end = e.GetEndTimeStamp();
    const bool incompleteEvent = start == 0;

    _PendingSingleEventNode pending(key, e.GetCategory(), start);

    // Move the children that fall in the timespan to the new node
    using ChildList = std::vector<TraceSingleEventNodeRefPtr>;
    ChildList& newChildren = pending.children;
    ChildList& currentChildren = stack.back().children;
    ChildList::iterator childStart = std::lower_bound(
        currentChildren.begin(),
        currentChildren.end(),
        start, 
        [] (const TraceSingleEventNodeRefPtr& lhs,
            const TraceEvent::TimeStamp& rhs) -> bool {
            return lhs->GetBeginTime() <= rhs;
        });
    newChildren.insert(newChildren.end(), childStart, currentChildren.end());
    currentChildren.erase(childStart, currentChildren.end());

    // Move the attributes that fall in the timespan to the new node
    using AttributeList = std::vector<_PendingSingleEventNode::AttributeData>;
    AttributeList& newAttrs = pending.attributes;
    AttributeList& currentAttrs = stack.back().attributes;
    AttributeList::iterator attrStart = std::lower_bound(
        currentAttrs.begin(),
        currentAttrs.end(),
        start,
        [] (const _PendingSingleEventNode::AttributeData& lhs,
            const TraceEvent::TimeStamp& rhs ) -> bool {
                return lhs.time <= rhs;
        });
    newAttrs.insert(newAttrs.end(), attrStart, currentAttrs.end());
    currentAttrs.erase(attrStart, currentAttrs.end());

    TraceSingleEventNodeRefPtr node = 
        pending.Close(incompleteEvent ? 0 : end, /* separateEvents = */ false);
    currentChildren.push_back(node);
}

void
TraceSingleEventTreeReport::_OnCounter(
    const TraceThreadId&, const TfToken& key, const TraceEvent& e)
{
    _counterDeltas[key].insert(
        std::make_pair(e.GetTimeStamp(), e.GetCounterValue()));
}

void
TraceSingleEventTreeReport::_OnData(
    const TraceThreadId& threadId, const TfToken& key, const TraceEvent& e)
{
    _PendingNodeStack& stack = _threadStacks[threadId];
    if (!stack.empty()) {
        _threadStacks[threadId].back().attributes.push_back(
            _PendingSingleEventNode::AttributeData{
            e.GetTimeStamp(), key, e.GetData()});
    }
}

TraceSingleEventTreeReport::_PendingSingleEventNode::_PendingSingleEventNode(
    const TfToken& key, TraceCategoryId category, TimeStamp start)
    : key(key)
    , category(category)
    , start(start)
{
}

TraceSingleEventNodeRefPtr 
TraceSingleEventTreeReport::_PendingSingleEventNode::Close(
    TimeStamp end, bool separateEvents) 
{
    TraceSingleEventNodeRefPtr node = 
        TraceSingleEventNode::New(key, category, start, end, separateEvents);
    for (TraceSingleEventNodeRefPtr& child : children) {
        node->Append(child);
    }
    for (AttributeData& it : attributes) {
        node->AddAttribute(TfToken(it.key), std::move(it.data));
    }
    return node;
}

PXR_NAMESPACE_CLOSE_SCOPE
