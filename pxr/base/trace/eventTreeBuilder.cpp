//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/trace/eventTreeBuilder.h"

#include "pxr/pxr.h"

#include "pxr/base/trace/trace.h"

#include <algorithm>

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
    _curStack.clear();

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
    //
    // The TraceCollection::Visitor interface always sends events between
    // Begin/EndThread, so just clear the stack and emplace a new root.
    _curStack.clear();
    _curStack.emplace_back(TfToken(threadId.ToString()), 
                           TraceCategory::Default, 0, 0,
                           /*separateEvents=*/false, /*isComplete=*/true);
}

void
Trace_EventTreeBuilder::OnEndThread(const TraceThreadId& threadId)
{
    // Close any incomplete nodes, attach any unattached children nodes
    TraceEventNodeRefPtr firstNode;
    while (!_curStack.empty()) {
        
        // close any timespan events left on the stack
        _PendingEventNode* backNode = &_curStack.back();
        firstNode = backNode->Close();
        
        // if this was incomplete event, get Begin/End times from children
        if (!backNode->isComplete) {
            firstNode->SetBeginAndEndTimesFromChildren();
        }
        
        _curStack.pop_back();
        if (!_curStack.empty()) {
            _curStack.back().node->_children.push_back(firstNode);
        }
    }
    // firstNode is now the thread root
    firstNode->SetBeginAndEndTimesFromChildren();
    _root->Append(std::move(firstNode));
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
    _PendingEventNode* prev = &_curStack.back();
    int index = _curStack.size()-1;

    while ((prev->isComplete ||
            prev->node->GetKey() != key) && _curStack.size() > 1) {

        --index;
        if (prev->isComplete) {
            _PopAndClose();
            prev = &_curStack.back();
        } else {
            prev = &_curStack[index];
        }
    }

    // Successfully found the matching End!
    if (_curStack.size() >= 1 && prev->node->GetKey() == key) {
        prev->node->_beginTime = e.GetTimeStamp();
        prev->node->_SetIsSeparateEvents(true);
        prev->isComplete = true;
        
    // Couldn't find the matching End, so treat as incomplete
    } else {
        // If we encounter a begin event that does not match an end 
        // event it means its from an incomplete scope. We need to 
        // insert a new node and take any pending children from the 
        // top of the stack and parent them under this new node.
        
        // Incomplete events set their duration to match their children.
        _PendingEventNode pending(key, e.GetCategory(), 0, 0, true, false);
        swap(pending.node->_children, _curStack.back().node->_children);
        swap(pending.node->_attributesAndSeparateEvents,
             _curStack.back().node->_attributesAndSeparateEvents);
        TraceEventNodeRefPtr node = pending.Close();
        node->SetBeginAndEndTimesFromChildren();
        _curStack.back().node->_children.push_back(std::move(node));
    }
}

void
Trace_EventTreeBuilder::_OnEnd(
    const TraceThreadId& threadId, const TfToken& key, const TraceEvent& e)
{
    _PendingEventNode* prev = &_curStack.back();

    // While this End can't be child of prevNode, pop and close prevNode
    while (prev->isComplete && !(e.GetTimeStamp() > prev->node->GetBeginTime()) 
           && _curStack.size() > 1) {
        _PopAndClose();
        prev = &_curStack.back();
    }

    // For end events, push a node with a temporary start time
    _curStack.emplace_back(
        key, e.GetCategory(), 0, e.GetTimeStamp(),
        /*separateEvents=*/true, /*isComplete=*/false);
}

void 
Trace_EventTreeBuilder::_OnTimespan(
    const TraceThreadId& threadId, const TfToken& key, const TraceEvent& e)
{
    const auto [start, end] = e.GetTimeSpanStamps();

    if (!TF_VERIFY(!_curStack.empty())) {
        return;
    }
    
    _PendingEventNode* prev = &_curStack.back();

    // while this new node is not a child of prevNode
    while ((start < prev->node->GetBeginTime() ||
            end > prev->node->GetEndTime()) && _curStack.size() > 1) {
        _PopAndClose();
        prev = &_curStack.back();
    }
        
    // In all cases, add this new node to the stack
    _curStack.emplace_back(key, e.GetCategory(), start, end,
                           /*separateEvents=*/false, /*isComplete=*/true);
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
    if (_curStack.empty()) {
        return;
    }

    const TraceEvent::TimeStamp eventTime = e.GetTimeStamp();
    
    _PendingEventNode* prev = &_curStack.back(); 

    // if that data doesn't fall in this node's timespan, look to prevNode
    while ((eventTime < prev->node->GetBeginTime() || 
            eventTime > prev->node->GetEndTime()) && _curStack.size() > 1) {
        _PopAndClose();
        prev = &_curStack.back();
    }
    
    // Add data to the real node in the stack
    prev->node->AddAttribute(key, e.GetData());
}

void
Trace_EventTreeBuilder::_PopAndClose()
{
    TraceEventNodeRefPtr closed = _curStack.back().Close();
    _curStack.pop_back();
    _curStack.back().node->_children.push_back(std::move(closed));
}

Trace_EventTreeBuilder::_PendingEventNode::_PendingEventNode(
    const TfToken& key, TraceCategoryId category, TimeStamp start, 
    TimeStamp end, bool separateEvents, bool isComplete)
    : node(TraceEventNode::New(key, category, start, end, separateEvents))
    , isComplete(isComplete)
{
}

TraceEventNodeRefPtr 
Trace_EventTreeBuilder::_PendingEventNode::Close() 
{
    // We iterate backwards to build the tree so children and attributes were
    // encountered in reverse order.  TraceEventNode::AddAttribute handles
    // reversal by prepending added attributes, but we have to handle child
    // nodes here.
    auto &nodeChildren = node->_children;
    if (ARCH_UNLIKELY(nodeChildren.size() > 1)) {
        std::reverse(nodeChildren.begin(), nodeChildren.end());
    }
    return std::move(node);
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
