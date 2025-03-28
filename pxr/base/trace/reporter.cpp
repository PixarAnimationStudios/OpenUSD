//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/trace/reporter.h"

#include "pxr/pxr.h"
#include "pxr/base/trace/aggregateTree.h"
#include "pxr/base/trace/collector.h"
#include "pxr/base/trace/eventTree.h"
#include "pxr/base/trace/reporterDataSourceCollector.h"
#include "pxr/base/trace/threads.h"

#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/arch/timing.h"
#include "pxr/base/js/json.h"

#include <algorithm>
#include <map>
#include <ostream>
#include <regex>
#include <string>
#include <vector>
#include <stack>

using std::ostream;
using std::string;

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
    _shouldAdjustForOverheadAndNoise(true)
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

static std::string
_GetKeyName(const TfToken& key)
{
    return key.GetString();
}

static void
_PrintLineTimes(ostream &s, double inclusive, double exclusive,
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

static void
_PrintRecursionMarker(
    ostream &s,
    const std::string &label, 
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

static void
_PrintNodeTimes(
    ostream &s,
    TraceAggregateNodeRefPtr node,
    int indent, 
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
    
    for (const TraceAggregateNodeRefPtr& it : sortedKids) {
        _PrintNodeTimes(s, it, indent+2, iterationCount);
    }
}

void
TraceReporter::_PrintTimes(ostream &s)
{
    using SortedTimes = std::multimap<TimeStamp, TfToken>;

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

    UpdateTraceTrees();

    // Adjust for overhead.
    if (ShouldAdjustForOverheadAndNoise()) {
        _aggregateTree->GetRoot()->AdjustForOverheadAndNoise(
            TraceCollector::GetInstance().GetScopeOverhead(),
            ArchGetTickQuantum());
    }

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

/* static */ std::vector<TraceReporter::ParsedTree> 
TraceReporter::LoadReport(
    std::istream &stream)
{
    // Regular expression for the reported number of iterations.
    static const std::regex itCountRE(R"(Number of iterations: (\d+))");

    // Every report has this header.
    static const std::string treeHeader("Tree view  ==============");

    // Regular expression for each trace line in a report.
    static const auto traceRowRE = std::invoke([]() -> std::regex {
        // Note that the expression we build will have exactly 5 capture groups:
        // 
        // 1. The inclusive time entry (may be empty)
        // 
        // 2. The exclusive time entry (may also be empty)
        //
        // 3. The sample count (required)
        //
        // 4. The indentation string (e.g., "| | ", may be empty)
        //
        // 5. The tag

        // Match time entries:
        //
        // - Time entries are in milliseconds and always rounded to 1000ths 
        //   place, so expect exactly 3 digits after the decimal.
        //
        // - Trace Reporter will either output 0, 1, or 2 time entries.
        //
        // - The first entry, if present, is always the inclusive time entry.
        //
        // Note: This is structured this way to maintain compatibility with 
        // Windows. For some reason, if we just have two optional time entry 
        // patterns, and if only one has match, Linux and Windows will disagree
        // on whether the matched entry belongs to the first or second capture
        // group. To work around this, we nest the expression to match either:
        //
        // - Required time entry followed by an optional time entry
        //
        // - or an empty group.
        //
        const std::string msEntryPattern = R"((?:(\d+\.\d{3}) ms))";
        const std::string msPattern = TfStringPrintf(R"(%s\s+%s?\s+)", 
            msEntryPattern.c_str(), msEntryPattern.c_str());
        const std::string msPatternOptional = TfStringPrintf(
            R"((?:%s|(?:)\s+))", msPattern.c_str());

        // Match sample entry
        // - Can be either an integer or a floating point number (for traces
        //   with iterations)
        const std::string samplePattern = R"((\d+|\d+\.\d{3}) samples\s+)";

        // Match indentation string
        const std::string indentPattern = R"(([ |]+))";

        // Match tag
        const std::string tagPattern = R"((.*))";

        return std::regex(R"(\s*)" + msPatternOptional + samplePattern 
            + indentPattern + tagPattern);
    });

    // Current state of the parser. 
    enum class State {
        // Tree view header not yet found
        FindingTree,
        // Found Tree view header, searching for others
        ReadingTree
    } state = State::FindingTree;

    std::cmatch match;

    TraceAggregateTreeRefPtr currentTree;

    // By default assume 1 iteration. Only trees with non-1 iteration counts
    // have the the iteration count line.
    int currentIters = 1;

    std::vector<ParsedTree> result;
    std::stack<TraceAggregateNodePtr> stack;
    for (std::string line; std::getline(stream, line);) {
        // When finding the tree, only parse for the tree header and the 
        // iteration count.
        if (state == State::FindingTree) {
            if (line == treeHeader) {
                state = State::ReadingTree;
                currentTree = TraceAggregateTree::New();
                stack.push(currentTree->GetRoot());

                // By this point we've already seen the iteration count for this
                // tree.
                result.push_back({currentTree, currentIters});
                continue;
            }

            if (std::regex_match(line.c_str(), match, itCountRE)) {
                currentIters = std::stoi(match[1]);
            }

            continue;
        }

        if (!TF_VERIFY(state == State::ReadingTree)) {
            // If we're not finding a tree, we should be reading a tree.
            break;
        }

        // When we see an empty line, that means we've gotten a full tree. Clear
        // the stack and switch back to tree finding.
        if (TfStringTrim(line).empty()) {
            state = State::FindingTree;
            stack = {};
            currentIters = 1;
            continue;
        }

        if (!std::regex_match(line.c_str(), match, traceRowRE)) {
            continue;
        }

        // The indentation string always has a size of 2x the depth.
        //
        // Determine the depth and then pop the stack until we have the parent
        // node.
        const size_t depth = match[4].length() / 2;
        while (stack.size() > depth+1) {
            stack.pop();
        }
        TraceAggregateNodePtr &parent = stack.top();

        // Add a new node.
        // Sample count may be a double if there's >1 iterations.
        const int samples = std::round(currentIters*TfStringToDouble(match[3]));
        stack.push(parent->Append(
            TraceReporter::CreateValidEventId(),
            /* key */ TfToken(match[5].str()),
            /* timestamp */ ArchSecondsToTicks(
                currentIters*TfStringToDouble(match[1])/1000.0),
            /* count */ samples,
            /* exclusiveCount */ samples));
    }

    return result;
}

void
TraceReporter::ReportTimes(std::ostream &s)
{
    UpdateTraceTrees();

    s << "\nTotal time for each key ==============\n";
    _PrintTimes(s);
    s << "\n";
}

void 
TraceReporter::ReportChromeTracing(std::ostream &s)
{
    UpdateTraceTrees();

    JsWriter w(s);
    _eventTree->WriteChromeTraceObject(w);
}


void 
TraceReporter::_RebuildEventAndAggregateTrees()
{
    // Get the latest from the collector and process the events.
    _Update();

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
TraceReporter::UpdateTraceTrees()
{
    _RebuildEventAndAggregateTrees();
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

void
TraceReporter::SetShouldAdjustForOverheadAndNoise(bool shouldAdjust)
{
    _shouldAdjustForOverheadAndNoise = shouldAdjust;
}

bool
TraceReporter::ShouldAdjustForOverheadAndNoise() const
{
    return _shouldAdjustForOverheadAndNoise;
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

        // We just always build the single (additional) event tree for the 
        // (additional) new collection given and pass it on to the aggregate 
        // tree. Note that the call to Add() merges in the newGraph to 
        // _eventTree which thus represents the merged sum of all collections 
        // seen here whereas newGraph is just the graph for the new collection. 

        TraceEventTreeRefPtr newGraph = _eventTree->Add(*collection);
        _aggregateTree->Append(newGraph, *collection);
    }
}

TraceReporterPtr 
TraceReporter::GetGlobalReporter()
{
    // Note that, like TfSingleton, the global reporter instance is not freed
    // at shutdown.
    static const TraceReporterPtr globalReporter(
        new TraceReporter(
            "Trace global reporter",
            TraceReporterDataSourceCollector::New()));
    return globalReporter;
}

PXR_NAMESPACE_CLOSE_SCOPE
