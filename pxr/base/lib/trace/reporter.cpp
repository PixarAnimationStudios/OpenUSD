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

#include "pxr/base/trace/reporter.h"

#include "pxr/pxr.h"
#include "pxr/base/trace/collection.h"
#include "pxr/base/trace/collectionNotice.h"
#include "pxr/base/trace/eventNode.h"
#include "pxr/base/trace/eventTreeBuilder.h"
#include "pxr/base/trace/reporterDataSourceCollector.h"
#include "pxr/base/trace/threads.h"
#include "pxr/base/trace/trace.h"

#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/scoped.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/symbols.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/arch/timing.h"
#include "pxr/base/js/json.h"

#include <math.h>
#include <iostream>
#include <numeric>
#include <map>
#include <vector>

using std::map;
using std::multimap;
using std::ostream;
using std::pair;
using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(TraceReporterTokens, TRACE_REPORTER_TOKENS);

//
// TraceReporter
//

TraceReporter::TraceReporter(const string& label,
                             DataSourcePtr dataSource) :
    TraceReporterBase(std::move(dataSource)),
    _label(label),
    _groupByFunction(true),
    _foldRecursiveCalls(false),
    _buildEventTree(false)
{
    _rootAggregateNode = TraceAggregateNode::New();
    _eventTree = TraceEventTree::New();
    _eventTimes = _EventTimes();
}

TraceReporter::~TraceReporter()
{
}

void
TraceReporter::_ComputeInclusiveCounterValues()
{
    _rootAggregateNode->CalculateInclusiveCounterValues();
}

void
TraceReporter::OnBeginCollection() 
{
    _threadStacks.clear();
}

void
TraceReporter::OnEndCollection() 
{
    // Compute all the inclusive counter values in the graph
    _ComputeInclusiveCounterValues();
}

void
TraceReporter::OnBeginThread(const TraceThreadId& threadIndex) 
{
    // Node need a valid id to be printed in reports.
    TraceAggregateNode::Id id(threadIndex);

    // Note, that TraceGetThreadId() returns the id of the current thread,
    // i.e. the reporting thread. Since we always report from the main
    // thread, we label the current thread "Main Thread" in the trace.
    _PendingNodeStack& stack = _threadStacks[threadIndex];
    stack.emplace_back(
        id,
        TfToken(threadIndex.ToString()),
        0);
}

void
TraceReporter::OnEndThread(const TraceThreadId& threadIndex) 
{
    _ThreadStackMap::iterator it = _threadStacks.find(threadIndex);
    if (it != _threadStacks.end()) {
        _PendingNodeStack& stack = it->second;
        // Close any incomplete nodes.
        TraceAggregateNodeRefPtr threadRoot;
        while (!stack.empty()) {
            stack.back().start = 0;
            _PendingEventNode::Child child = stack.back().Close(0);
            threadRoot = child.node;
            stack.pop_back();
            if (!stack.empty()) {
                stack.back().children.push_back(child);
            }
        }
        _rootAggregateNode->Append(threadRoot);
    }
}

void
TraceReporter::OnEvent(
    const TraceThreadId& threadIndex, const TfToken& key, const TraceEvent& e) 
{
    switch(e.GetType()) {
        case TraceEvent::EventType::Begin:
            _OnBeginEvent(threadIndex, key, e);
            break;
        case TraceEvent::EventType::End:
            _OnEndEvent(threadIndex, key, e);
            break;
        case TraceEvent::EventType::CounterDelta:
        case TraceEvent::EventType::CounterValue:
            _OnCounterEvent(threadIndex, key, e);
            break;
        case TraceEvent::EventType::Timespan:
            _OnTimespanEvent(threadIndex, key, e);
            break;
        case TraceEvent::EventType::ScopeData:
            _OnDataEvent(threadIndex, key, e);
            break;
        case TraceEvent::EventType::Unknown:
            break;
    }
}

bool
TraceReporter::AcceptsCategory(TraceCategoryId) 
{
    return true;
}

void
TraceReporter::_OnBeginEvent(
    const TraceThreadId& threadIndex, const TfToken& key, const TraceEvent& e) 
{
     // For begin events, push a new node
    _ThreadStackMap::iterator it = _threadStacks.find(threadIndex);
    if (it != _threadStacks.end()) {
        // Node needs a valid id to be printed in reports.
        TraceAggregateNode::Id id(threadIndex);
        it->second.emplace_back(id, key, e.GetTimeStamp());
    }
}

void
TraceReporter::_OnEndEvent(
    const TraceThreadId& threadIndex, const TfToken& key, const TraceEvent& e) 
{
    // For end events, create the node and pop the stack
    _ThreadStackMap::iterator it = _threadStacks.find(threadIndex);
    if (it != _threadStacks.end()) {
        _PendingNodeStack& stack = it->second;
        _PendingEventNode::Child newChild;
        if (stack.back().key == key) {
            newChild = stack.back().Close(e.GetTimeStamp());
            stack.pop_back();
            if (!stack.empty()) {
                stack.back().children.push_back(newChild);
            }
        } else {
            // If we encounter an end event that does not match a begin 
            // event it means its from an incomplete scope. We need to 
            // insert a new node and take any pending children and 
            // counters from the top of the stack and parent them under 
            // this new node.

            // Node needs a valid id to be printed in reports.
            TraceAggregateNode::Id id(threadIndex);

            _PendingEventNode pending(id, key, 0);
            swap(pending.children, stack.back().children);
            swap(pending.counters, stack.back().counters);
            newChild = pending.Close(0);
            stack.back().children.push_back(newChild);
        }
        // While we're here, accumulate elapsed time by key.
        _AccumulateTime(newChild.node);
    }
}

void
TraceReporter::_OnCounterEvent( 
    const TraceThreadId& threadIndex, const TfToken& key, const TraceEvent& e)
{
    // For counter events, add the counter key to the map of counters
    // and create a new counter index, if necessary. Also, increment
    // the total, accumulated value stored in the counter map.
    // During this step, we merely store the inclusive and exclusive
    // counter values at the node. We will propagate these values to
    // the parents in a post-processing step in
    // _ComputeInclusiveCounterValues.

    bool isDelta = false;
    switch (e.GetType()) {
        case TraceEvent::EventType::CounterDelta: isDelta = true; break;
        case TraceEvent::EventType::CounterValue: break;
        default: return;
    }

    _ThreadStackMap::iterator it = _threadStacks.find(threadIndex);
    if (it != _threadStacks.end()) {
        _PendingNodeStack& stack = it->second;

        // Compute the total counter value
        CounterMap::iterator it =
            _counters.insert(
                std::make_pair(key, 0.0)).first;

        if (isDelta) {
            it->second += e.GetCounterValue();
        } else {
            it->second = e.GetCounterValue();
        }

        // Insert the counter index into the map, if one does not
        // already exist. If no counter index existed in the map, 
        // increment to the next available counter index.
        std::pair<_CounterIndexMap::iterator, bool> res =
            _counterIndexMap.insert(
                std::make_pair(key, _counterIndex));
        if (res.second) {
            ++_counterIndex;
        }

        // It only makes sense to store delta values in the specific nodes at 
        // the moment. This might need to be revisted in the future.
        if (isDelta) {
            // Set the counter value on the current node.
            stack.back().counters.push_back(
                {e.GetTimeStamp(), res.first->second, e.GetCounterValue()});
        }
    }
}

void
TraceReporter::_OnTimespanEvent(
    const TraceThreadId& threadIndex, const TfToken& key, const TraceEvent& e) 
{
    _ThreadStackMap::iterator it = _threadStacks.find(threadIndex);
    if (it != _threadStacks.end()) {
        _PendingNodeStack& stack = it->second;
        // If we encounter an end event that does not match a begin 
        // event it means its from an incomplete scope. We need to 
        // insert a new node and take any pending children and 
        // counters from the top of the stack and parent them under 
        // this new node.

        // Node needs a valid id to be printed in reports.
        TraceAggregateNode::Id id(threadIndex);

        const TimeStamp start = e.GetStartTimeStamp();
        const TimeStamp end = e.GetEndTimeStamp();
        const bool incompleteEvent = start == 0;

        _PendingEventNode pending(id, key, start);

        // Move the children that fall in the timespan to the new node
        using ChildList = std::vector<_PendingEventNode::Child>;
        ChildList& newChildren = pending.children;
        ChildList& currentChildren = stack.back().children;

        ChildList::iterator startChild = std::lower_bound(
            currentChildren.begin(),
            currentChildren.end(),
            start,
            [] (const _PendingEventNode::Child& lhs,
                const TimeStamp& rhs) -> bool {
                return lhs.start <= rhs;
            });

        newChildren.insert(
            newChildren.end(), startChild, currentChildren.end());
        currentChildren.erase(startChild, currentChildren.end());
        
        // Move the counters that fall in the timespan to the new node
        using CounterList = std::vector<_PendingEventNode::CounterData>;
        CounterList& newCounters = pending.counters;
        CounterList& currentCounters = stack.back().counters;
        CounterList::iterator startCounter = std::lower_bound(
            currentCounters.begin(),
            currentCounters.end(),
            start,
            [] (const _PendingEventNode::CounterData& lhs,
                const TimeStamp& rhs) {
                return lhs.time <= rhs;
            });
        
        newCounters.insert(
            newCounters.end(), startCounter, currentCounters.end());
        currentCounters.erase(startCounter, currentCounters.end());

        _PendingEventNode::Child newChild =
            pending.Close(incompleteEvent ? 0 : end);
        stack.back().children.push_back(newChild);

        // While we're here, accumulate elapsed time by key.
        _AccumulateTime(newChild.node);
    }
}

void
TraceReporter::_OnDataEvent(
    const TraceThreadId& threadIndex, const TfToken&, const TraceEvent& e) 
{}

void
TraceReporter::_AccumulateTime(const TraceAggregateNodeRefPtr& node)
{
    if (node->GetInclusiveTime() > 0) {
        _eventTimes[node->GetKey()] += node->GetInclusiveTime();
    }
}

static std::string
_IndentString(int indent)
{
    std::string s;
    s.resize(indent, ' ');

    // Insert '|' characters every 4 spaces.
    // The magic value of 2 makes it line up with the outer scope.
    for (int i=2; i<indent; i+=4) {
        s[i] = '|';
    }
    return s;
}

void
TraceReporter::_PrintLineTimes(ostream &s, double inclusive, double exclusive,
                               int count, const string& label, int indent,
                               bool recursive_node, int iterationCount)
{
    string inclusiveStr = TfStringPrintf("%9.3f ms ",
            ArchTicksToSeconds( uint64_t(inclusive * 1e3) / iterationCount ));
    if (inclusive <= 0)
        inclusiveStr = string(inclusiveStr.size(), ' ');

    string exclusiveStr = TfStringPrintf("%9.3f ms ",
            ArchTicksToSeconds( uint64_t(exclusive * 1e3) / iterationCount ));
    if (exclusive <= 0)
        exclusiveStr = string(exclusiveStr.size(), ' ');

    string countStr;
    if (iterationCount == 1)
        countStr = TfStringPrintf("%7.0f samples ", double(count));
    else
        countStr = TfStringPrintf("%10.3f samples ", 
                                  double(count)/iterationCount);

    if (count <= 0)
    {
        // CODE_COVERAGE_OFF -- shouldn't get a count of zero
        countStr = string(countStr.size(), ' ');
        // CODE_COVERAGE_ON
    }

    s << inclusiveStr << exclusiveStr << countStr << " ";

    s << _IndentString(indent);

    // Put a '*' before the label of recursive nodes so that we can easily 
    // identify them.
    if (recursive_node) 
        s << "*";
    s << label << "\n";
}

void
TraceReporter::_PrintRecursionMarker(ostream &s, const std::string &label, 
                                     int indent)
{
    string inclusiveStr(13, ' ');
    string exclusiveStr(13, ' ');
    string countStr(16, ' ');

    // Need -1 here in order to get '|' characters to line up.
    string indentStr( _IndentString(indent-1) );

    s << inclusiveStr << exclusiveStr << countStr << " " << indentStr << " ";
    s << "[" << label << "]\n";

}

#define _SORT 0

// Used by std::sort
static bool
_InclusiveGreater(const TraceAggregateNodeRefPtr &a, const TraceAggregateNodeRefPtr &b)
{
    return (a->GetInclusiveTime() > b->GetInclusiveTime());
}

void
TraceReporter::_PrintNodeTimes(ostream &s, TraceAggregateNodeRefPtr node, int indent, 
                               int iterationCount)
{
    // The root of the tree has id == -1, no useful stats there.

    if (node->GetId().IsValid()) {

        if (node->IsRecursionMarker()) {
            _PrintRecursionMarker(s, _GetKeyName(node->GetKey()), indent);
            return;
        }

        bool r = node->IsRecursionHead();
        _PrintLineTimes(s, node->GetInclusiveTime(), node->GetExclusiveTime(r),
                        node->GetCount(r), _GetKeyName(node->GetKey()),
                        indent, r, iterationCount);
    }

    // sort children by inclusive time on output
    std::vector<TraceAggregateNodeRefPtr> sortedKids;
    for (const TraceAggregateNodeRefPtr& it : node->GetChildrenRef()) {
        sortedKids.push_back(it);
    }
    
    if (_SORT) {
        std::sort(sortedKids.begin(), sortedKids.end(), _InclusiveGreater);
    }

    for (const TraceAggregateNodeRefPtr& it : sortedKids) {
        _PrintNodeTimes(s, it, indent+2, iterationCount);
    }
}

void
TraceReporter::_PrintLineCalls(ostream &s, int count, int exclusiveCount,
                               int totalCount, const string& label, int indent)
{
    string inclusiveStr =
        TfStringPrintf("%9d (%6.2f%%) ",
                       count,
                       100.0 * count / totalCount);

    string exclusiveStr =
        TfStringPrintf("%9d (%6.2f%%) ",
                       exclusiveCount,
                       100.0 * exclusiveCount / totalCount);

    s << inclusiveStr << exclusiveStr << " ";

    s << _IndentString(indent);

    s << label << "\n";
}

void
TraceReporter::_PrintTimes(ostream &s)
{
    using SortedTimes = multimap<TimeStamp, TfToken>;

    SortedTimes sortedTimes;
    for (const _EventTimes::value_type& it : _eventTimes) {
        sortedTimes.insert(SortedTimes::value_type(it.second, it.first));
    }
    for (const SortedTimes::value_type& it : sortedTimes) {
        s << TfStringPrintf("%9.3f ms ",
                            ArchTicksToSeconds((uint64_t)(it.first * 1e3)))
          << _GetKeyName(it.second) << "\n";
    }
}

std::string
TraceReporter::_GetKeyName(const TfToken& key) const
{
    return key.GetString();
}


void
TraceReporter::Report(
    std::ostream &s,
    int iterationCount)
{
    if (iterationCount < 1) {
        TF_CODING_ERROR("iterationCount %d is invalid; falling back to 1",
                        iterationCount);
        iterationCount = 1;
    }

    UpdateAggregateTree();

    // Fold recursive calls if we need to.
    if (GetFoldRecursiveCalls()) {
        _rootAggregateNode->MarkRecursiveChildren();
    }

    if (iterationCount > 1)
        s << "\nNumber of iterations: " << iterationCount << "\n";

    s << "\nTree view  ==============\n";
    if (iterationCount == 1)
        s << "   inclusive    exclusive        \n";
    else {
        s << "  incl./iter   excl./iter       samples/iter\n";
    }

    _PrintNodeTimes(s, _rootAggregateNode, 0, iterationCount);

    s << "\n";
}

void
TraceReporter::ReportTimes(std::ostream &s)
{
    UpdateAggregateTree();

    s << "\nTotal time for each key ==============\n";
    _PrintTimes(s);
    s << "\n";
}

void 
TraceReporter::ReportChromeTracing(std::ostream &s)
{
    UpdateEventTree();

    JsWriteToStream(JsValue(_eventTree->CreateChromeTraceObject()), s);

}


void 
TraceReporter::_UpdateTree(bool buildEventTree)
{
    // Get the latest from the collector and process the events.
    {
        TfScopedVar<bool> scope(_buildEventTree, buildEventTree);
        _Update();
    }

    // If MallocTags were enabled for the capture of this trace, add a dummy
    // warning node as an indicator that the trace may have been slowed down
    // by the memory tagging, unless there was nothing reported anyway.
    // XXX: add "WARNING" token that Spy can use.
    if (_rootAggregateNode && !_rootAggregateNode->GetChildrenRef().empty() && 
        TfMallocTag::IsInitialized()) {
        _rootAggregateNode->Append(TraceAggregateNode::Id(), 
                          TfToken(
                              TraceReporterTokens->warningString.GetString() +
                              " MallocTags enabled"),
                          0,
                          1   /* count */,
                          1   /* exclusive count */);
    }
}

void
TraceReporter::UpdateAggregateTree()
{
    static const bool buildEventTree = true;
    _UpdateTree(!buildEventTree);
}

void
TraceReporter::UpdateEventTree()
{
    static const bool buildEventTree = true;
    _UpdateTree(buildEventTree);
}


void 
TraceReporter::ClearTree() 
{ 
    _rootAggregateNode = TraceAggregateNode::New();
    _eventTree = TraceEventTree::New();
    _eventTimes.clear();
    _counters.clear();
    _counterIndexMap.clear();
    _counterIndex = 0;
    _Clear();
}

TraceAggregateNodePtr
TraceReporter::GetAggregateTreeRoot()
{
    return _rootAggregateNode;
}

TraceEventNodeRefPtr
TraceReporter::GetEventRoot()
{
    return _eventTree->GetRoot();
}

TraceEventTreeRefPtr
TraceReporter::GetEventTree()
{
    return _eventTree;
}


const TraceReporter::CounterMap &
TraceReporter::GetCounters() const
{
    return _counters;
}

int
TraceReporter::GetCounterIndex(const TfToken &key) const
{
    _CounterIndexMap::const_iterator it = _counterIndexMap.find(key);
    return it != _counterIndexMap.end() ? it->second : -1;
}

bool
TraceReporter::AddCounter(const TfToken &key, int index, double totalValue)
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
TraceReporter::SetGroupByFunction(bool groupByFunction)
{
    _groupByFunction = groupByFunction;
}

bool
TraceReporter::GetGroupByFunction() const
{
    return _groupByFunction;
}

void 
TraceReporter::SetFoldRecursiveCalls(bool foldRecursiveCalls)
{
    _foldRecursiveCalls = foldRecursiveCalls;
}

bool 
TraceReporter::GetFoldRecursiveCalls() const
{
    return _foldRecursiveCalls;
}

/* static */
TraceAggregateNode::Id
TraceReporter::CreateValidEventId() 
{
    return TraceAggregateNode::Id(TraceGetThreadId());
}

TraceReporter::_PendingEventNode::_PendingEventNode(
    TraceAggregateNode::Id id, const TfToken& pkey, TimeStamp start)
: id(id)
, key(pkey)
, start(start)
{
}

TraceReporter::_PendingEventNode::Child 
TraceReporter::_PendingEventNode::Close(TimeStamp end)
{
    // The new node should have a time that is at least as long as the time of 
    // its children.
    TimeStamp childTime = std::accumulate(
        children.begin(), children.end(), TimeStamp(0), 
        [](TimeStamp ts, const Child& b) -> TimeStamp {
            return ts + b.node->GetInclusiveTime();
        });

    TraceAggregateNodeRefPtr node = 
        TraceAggregateNode::New(id, key, std::max(end-start, childTime));
    for (Child& child : children) {
        node->Append(child.node);
    }
    for (CounterData& c : counters) {
        node->AppendExclusiveCounterValue(c.index, c.value);
        node->AppendInclusiveCounterValue(c.index, c.value);
    }
    return Child{start, node};
}

void
TraceReporter::_ProcessCollection(
    const TraceReporterBase::CollectionPtr& collection)
{
    if (collection) {
        collection->Iterate(*this);
        if (_buildEventTree) {
            _eventTree->Add(*collection);
        }
    }
}

namespace {
class _GlobalReporterHolder {
public:
    /// Returns the singleton instance.
    static _GlobalReporterHolder &GetInstance() {
        return TfSingleton<_GlobalReporterHolder>::GetInstance();
    }

    _GlobalReporterHolder() {
        _globalReporter =
            TraceReporter::New("Trace global reporter",
                TraceReporterDataSourceCollector::New());

    }

    TraceReporterRefPtr _globalReporter;
};
}

TF_INSTANTIATE_SINGLETON(_GlobalReporterHolder);

TraceReporterPtr 
TraceReporter::GetGlobalReporter()
{
    return _GlobalReporterHolder::GetInstance()._globalReporter;
}

PXR_NAMESPACE_CLOSE_SCOPE
