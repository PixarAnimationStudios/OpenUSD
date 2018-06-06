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
#include "pxr/base/trace/aggregateTree.h"
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
#include <stack>
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
    _aggregateTree = TraceAggregateTree::New();
    _eventTree = TraceEventTree::New();
}

TraceReporter::~TraceReporter()
{
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
    for (const TraceAggregateTree::EventTimes::value_type& it
            : _aggregateTree->GetEventTimes() ) {
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
        _aggregateTree->GetRoot()->MarkRecursiveChildren();
    }

    if (iterationCount > 1)
        s << "\nNumber of iterations: " << iterationCount << "\n";

    s << "\nTree view  ==============\n";
    if (iterationCount == 1)
        s << "   inclusive    exclusive        \n";
    else {
        s << "  incl./iter   excl./iter       samples/iter\n";
    }

    _PrintNodeTimes(s, _aggregateTree->GetRoot(), 0, iterationCount);

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
    TraceAggregateNodePtr root = _aggregateTree->GetRoot();
    if (root && !root->GetChildrenRef().empty() && 
        TfMallocTag::IsInitialized()) {
        root->Append(TraceAggregateNode::Id(), 
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
    _aggregateTree->Clear();
    _eventTree = TraceEventTree::New();
    _Clear();
}

TraceAggregateNodePtr
TraceReporter::GetAggregateTreeRoot()
{
    return _aggregateTree->GetRoot();
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
    return _aggregateTree->GetCounters();
}

int
TraceReporter::GetCounterIndex(const TfToken &key) const
{
    return _aggregateTree->GetCounterIndex(key);
}

bool
TraceReporter::AddCounter(const TfToken &key, int index, double totalValue)
{
    return _aggregateTree->AddCounter(key, index, totalValue);
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

void
TraceReporter::_ProcessCollection(
    const TraceReporterBase::CollectionPtr& collection)
{
    if (collection) {
        _aggregateTree->Append(*collection);
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
