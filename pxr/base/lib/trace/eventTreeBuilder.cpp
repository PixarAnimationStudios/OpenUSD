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
            TfToken(threadId.ToString()), 
            TraceCategory::Default, 0, 0, false, true);
    }
}

void
Trace_EventTreeBuilder::OnEndThread(const TraceThreadId& threadId)
{
    _ThreadStackMap::iterator it = _threadStacks.find(threadId);
    if (it != _threadStacks.end()) {
        _PendingNodeStack& stack = it->second;

        // Close any incomplete nodes, attach any unattached children nodes
        TraceEventNodeRefPtr firstNode;
        while (!stack.empty()) {

            // close any timespan events left on the stack
            _PendingEventNode* backNode = &stack.back();
            firstNode = backNode->Close();

            // if this was incomplete event, get Begin/End times from children
            if (!backNode->isComplete) {
                firstNode->SetBeginAndEndTimesFromChildren();
            }

            stack.pop_back();
            if (!stack.empty()) {
                stack.back().children.push_back(firstNode);
            }
        }
        // firstNode is now the thread root
        firstNode->SetBeginAndEndTimesFromChildren();
        _root->Append(firstNode);
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

    // For a begin event, find and modify the matching end event
    // First, search the stack for a matching End
    _PendingNodeStack& stack = _threadStacks[threadId];
    _PendingEventNode* prevNode = &stack.back();
    int index = stack.size()-1;

    while ((prevNode->isComplete || prevNode->key != key) && stack.size() > 1) {

        if (prevNode->isComplete) {
            _PopAndClose(stack);
            prevNode = &stack.back();
            --index;
        } else {
            --index;
            prevNode = &stack[index];
        }

    }

    // Successfully found the matching End!
    if (stack.size() >= 1 && prevNode->key == key) {
        prevNode->start = e.GetTimeStamp();  
        prevNode->separateEvents = true;
        prevNode->isComplete = true;

    // Couldn't find the matching End, so treat as incomplete
    } else {
        // If we encounter a begin event that does not match an end 
        // event it means its from an incomplete scope. We need to 
        // insert a new node and take any pending children from the 
        // top of the stack and parent them under this new node.
        
        // Incomplete events set their duration to match their children.
        _PendingEventNode pending(key, e.GetCategory(), 0, 0, true, false);
        swap(pending.children, stack.back().children);
        swap(pending.attributes, stack.back().attributes);
        TraceEventNodeRefPtr node =
            pending.Close();
        node->SetBeginAndEndTimesFromChildren();
        stack.back().children.push_back(node);
    }
}

void
Trace_EventTreeBuilder::_OnEnd(
    const TraceThreadId& threadId, const TfToken& key, const TraceEvent& e)
{
    _PendingNodeStack& stack = _threadStacks[threadId];
    _PendingEventNode* prevNode = &stack.back();

    // While this End can't be child of prevNode, pop and close prevNode
    while(prevNode->isComplete && !(e.GetTimeStamp() > prevNode->start) 
        && stack.size() > 1) {
        _PopAndClose(stack);
        prevNode = &stack.back();
    }

    // For end events, push a node with a temporary start time
    stack.emplace_back(
        key, e.GetCategory(), 0, e.GetTimeStamp(), true, false);
}

void 
Trace_EventTreeBuilder::_OnTimespan(
    const TraceThreadId& threadId, const TfToken& key, const TraceEvent& e)
{

    const TraceEvent::TimeStamp start = e.GetStartTimeStamp();
    const TraceEvent::TimeStamp end = e.GetEndTimeStamp();

    _PendingEventNode thisNode(key, e.GetCategory(), start, end, false, true);

    _PendingNodeStack& stack = _threadStacks[threadId];
    _PendingEventNode* prevNode = &stack.back();

    // while thisNode is not a child of prevNode
    while ((thisNode.start < prevNode->start || thisNode.end > prevNode->end) 
        && stack.size() > 1) {
        _PopAndClose(stack);
        prevNode = &stack.back();
    }
        
    // In all cases, add thisNode to the stack
    stack.push_back(std::move(thisNode));
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

        _PendingEventNode* prevNode = &stack.back(); 

        // if that data doesn't fall in this node's timespan, look to prevNode
        while ((e.GetTimeStamp() < prevNode->start || 
            e.GetTimeStamp() > prevNode->end) && stack.size() > 1) {
            _PopAndClose(stack);
            prevNode = &stack.back();
        }

        // Add data to the real node in the stack
        prevNode->attributes.push_back(
            _PendingEventNode::AttributeData{
            e.GetTimeStamp(), key, e.GetData()});
    }
}

void
Trace_EventTreeBuilder::_PopAndClose(_PendingNodeStack& stack) 
{
    _PendingEventNode* prevNode = &stack.back();
    TraceEventNodeRefPtr closedOldPrev = prevNode->Close();
    stack.pop_back();
    // _PendingEventNode* newPrev = &stack.back();
    stack.back().children.push_back(closedOldPrev);
}
Trace_EventTreeBuilder::_PendingEventNode::_PendingEventNode(
    const TfToken& key, TraceCategoryId category, TimeStamp start, 
    TimeStamp end, bool separateEvents, bool isComplete)
    : key(key)
    , category(category)
    , start(start)
    , end(end)
    , separateEvents(separateEvents)
    , isComplete(isComplete)
{
}

TraceEventNodeRefPtr 
Trace_EventTreeBuilder::_PendingEventNode::Close() 
{
    // We are now iterating backwards to build the tree,
    // So children and attributes were encountered in reverse order
    std::reverse(children.begin(), children.end());
    std::reverse(attributes.begin(), attributes.end());

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
    collection.ReverseIterate(*this);
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
