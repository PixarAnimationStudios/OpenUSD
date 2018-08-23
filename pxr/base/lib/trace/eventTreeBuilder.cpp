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

#include "pxr/base/trace/eventTreeBuilder.h"

#include "pxr/pxr.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

Trace_EventTreeBuilder::Trace_EventTreeBuilder() 
    : _root(TraceEventNode::New())
{}

// Visitor interface
void
Trace_EventTreeBuilder::OnBeginCollection()
{

}

void
Trace_EventTreeBuilder::OnEndCollection()
{
    _threadStacks.clear();

    // for each key, sort the corresponding timestamps 
    for (TraceEventTree::MarkerValuesMap::value_type& item : _markersMap) {
        std::sort(item.second.begin(), item.second.end());
    }
}

bool
Trace_EventTreeBuilder::AcceptsCategory(TraceCategoryId id)
{
    return true;
}

void
Trace_EventTreeBuilder::OnBeginThread(const TraceThreadId& threadId)
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
Trace_EventTreeBuilder::OnEndThread(const TraceThreadId& threadId)
{
    _ThreadStackMap::iterator it = _threadStacks.find(threadId);
    if (it != _threadStacks.end()) {
        _PendingNodeStack& stack = it->second;
        // Close any incomplete nodes.
        TraceEventNodeRefPtr threadRoot;
        while (!stack.empty()) {
            // TODO: Incomplete events are treated as zero duration.
            stack.back().start = 0;
            threadRoot = stack.back().Close(0, /* separateEvents = */ true);
            threadRoot->SetBeginAndEndTimesFromChildren();
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
Trace_EventTreeBuilder::OnEvent(
    const TraceThreadId& threadIndex, const TfToken& key, const TraceEvent& e) 
{
    switch(e.GetType()) {
        case TraceEvent::EventType::Begin:
            _OnBegin(threadIndex, key, e);
            break;
        case TraceEvent::EventType::End:
            _OnEnd(threadIndex, key, e);
            break;
        case TraceEvent::EventType::CounterDelta:
        case TraceEvent::EventType::CounterValue:
            // Handled by the counter accumulator
            break;
        case TraceEvent::EventType::Timespan:
            _OnTimespan(threadIndex, key, e);
            break;
        case TraceEvent::EventType::Marker:
            _OnMarker(threadIndex, key, e);
            break;
        case TraceEvent::EventType::ScopeData:
            _OnData(threadIndex, key, e);
            break;
        case TraceEvent::EventType::Unknown:
            break;
    }
}

void
Trace_EventTreeBuilder::_OnBegin(
    const TraceThreadId& threadId, const TfToken& key, const TraceEvent& e)
{
    // For begin events, push a partial node
    _threadStacks[threadId].emplace_back(
        key, e.GetCategory(), e.GetTimeStamp());
}

void
Trace_EventTreeBuilder::_OnEnd(
    const TraceThreadId& threadId, const TfToken& key, const TraceEvent& e)
{
    // For an end event, create the node and pop the stack
    _PendingNodeStack& stack = _threadStacks[threadId];
    if (stack.back().key == key) {
        TraceEventNodeRefPtr node = 
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
        
        // Incomplete events set their duration to match their children.
        _PendingEventNode pending(key, e.GetCategory(), 0);
        swap(pending.children, stack.back().children);
        swap(pending.attributes, stack.back().attributes);
        TraceEventNodeRefPtr node =
            pending.Close(0, /* separateEvents = */ true);
        node->SetBeginAndEndTimesFromChildren();
        stack.back().children.push_back(node);
    }
}

void 
Trace_EventTreeBuilder::_OnTimespan(
    const TraceThreadId& threadId, const TfToken& key, const TraceEvent& e)
{
    // If we encounter a timespan, we need to insert a new node and take any
    // pending children and data from the top of the stack that are within the
    // duration and add them to this new node.
    
   _PendingNodeStack& stack = _threadStacks[threadId];

    const TraceEvent::TimeStamp start = e.GetStartTimeStamp();
    const TraceEvent::TimeStamp end = e.GetEndTimeStamp();
    const bool incompleteEvent = start == 0;

    _PendingEventNode pending(key, e.GetCategory(), start);

    // Move the children that fall in the timespan to the new node
    using ChildList = std::vector<TraceEventNodeRefPtr>;
    ChildList& newChildren = pending.children;
    ChildList& currentChildren = stack.back().children;
    ChildList::iterator childStart = std::lower_bound(
        currentChildren.begin(),
        currentChildren.end(),
        start, 
        [] (const TraceEventNodeRefPtr& lhs,
            const TraceEvent::TimeStamp& rhs) -> bool {
            return lhs->GetBeginTime() <= rhs;
        });
    newChildren.insert(newChildren.end(), childStart, currentChildren.end());
    currentChildren.erase(childStart, currentChildren.end());

    // Move the attributes that fall in the timespan to the new node
    using AttributeList = std::vector<_PendingEventNode::AttributeData>;
    AttributeList& newAttrs = pending.attributes;
    AttributeList& currentAttrs = stack.back().attributes;
    AttributeList::iterator attrStart = std::lower_bound(
        currentAttrs.begin(),
        currentAttrs.end(),
        start,
        [] (const _PendingEventNode::AttributeData& lhs,
            const TraceEvent::TimeStamp& rhs ) -> bool {
                return lhs.time <= rhs;
        });
    newAttrs.insert(newAttrs.end(), attrStart, currentAttrs.end());
    currentAttrs.erase(attrStart, currentAttrs.end());

    TraceEventNodeRefPtr node = 
        pending.Close(incompleteEvent ? 0 : end, /* separateEvents = */ false);
    currentChildren.push_back(node);
}

void
Trace_EventTreeBuilder::_OnMarker(
    const TraceThreadId& threadId, const TfToken& key, const TraceEvent& e)
{
    _markersMap[key].push_back(std::make_pair(e.GetTimeStamp(), threadId));
}

void
Trace_EventTreeBuilder::_OnData(
    const TraceThreadId& threadId, const TfToken& key, const TraceEvent& e)
{
    _PendingNodeStack& stack = _threadStacks[threadId];
    if (!stack.empty()) {
        _threadStacks[threadId].back().attributes.push_back(
            _PendingEventNode::AttributeData{
            e.GetTimeStamp(), key, e.GetData()});
    }
}

Trace_EventTreeBuilder::_PendingEventNode::_PendingEventNode(
    const TfToken& key, TraceCategoryId category, TimeStamp start)
    : key(key)
    , category(category)
    , start(start)
{
}

TraceEventNodeRefPtr 
Trace_EventTreeBuilder::_PendingEventNode::Close(
    TimeStamp end, bool separateEvents) 
{
    TraceEventNodeRefPtr node = 
        TraceEventNode::New(key, category, start, end, 
            std::move(children), separateEvents);
    for (AttributeData& it : attributes) {
        node->AddAttribute(TfToken(it.key), std::move(it.data));
    }
    return node;
}

void
Trace_EventTreeBuilder::CreateTree(const TraceCollection& collection)
{
    collection.Iterate(*this);
    _counterAccum.Update(collection);
    _tree = TraceEventTree::New(_root, _counterAccum.GetCounters(), _markersMap);
}

bool
Trace_EventTreeBuilder::_CounterAccumulator::_AcceptsCategory(
    TraceCategoryId)
{
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
