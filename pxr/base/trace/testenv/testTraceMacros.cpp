//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/trace/trace.h"
#include "pxr/base/trace/reporter.h"
#include "pxr/base/trace/eventNode.h"
#include "pxr/base/trace/eventTree.h"
#include "pxr/base/tf/stringUtils.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static void
TestMacros() {
    TRACE_FUNCTION();
    {
        TRACE_SCOPE("Test Scope");
        {
            TRACE_FUNCTION_SCOPE("Inner Scope");
            TRACE_COUNTER_DELTA("Counter A", 1);
            TRACE_MARKER("Marker A");
        }
        TRACE_COUNTER_VALUE("Counter B", 2);
        TRACE_MARKER_DYNAMIC(TfStringPrintf("Dynamic Marker %d", 1));
    }
}

int main(int argc, char* argv[]) {
    TraceCollector* collector = &TraceCollector::GetInstance();
    TraceReporterPtr reporter = TraceReporter::GetGlobalReporter();
    collector->SetEnabled(true);
    TestMacros();
    collector->SetEnabled(false);
    reporter->ReportChromeTracing(std::cout);

    TraceAggregateNodeRefPtr threadNode =
        reporter->GetAggregateTreeRoot()->GetChild("Main Thread");
    TF_AXIOM(threadNode);

    TraceAggregateNodeRefPtr funcNode = threadNode->GetChild("TestMacros");
    TF_AXIOM(funcNode);

    TraceAggregateNodeRefPtr scopeNode = funcNode->GetChild("Test Scope");
    TF_AXIOM(scopeNode);

    TraceAggregateNodeRefPtr innerScopeNode = 
        scopeNode->GetChild("TestMacros (Inner Scope)");
    TF_AXIOM(innerScopeNode);

    const TraceReporter::CounterMap& counters = reporter->GetCounters();
    TraceReporter::CounterMap::const_iterator it =
        counters.find(TfToken("Counter A"));
    TF_AXIOM(it != counters.end());
    TF_AXIOM(it->second == 1.0);

    it = counters.find(TfToken("Counter B"));
    TF_AXIOM(it != counters.end());
    TF_AXIOM(it->second == 2.0);

    // Test Markers
    TraceEventTreeRefPtr timeline = reporter->GetEventTree();
    TF_AXIOM(timeline);

    const TraceEventTree::MarkerValuesMap& markers =
        timeline->GetMarkers();
    TraceEventTree::MarkerValuesMap::const_iterator it2 =
        markers.find(TfToken("Marker A"));
    TF_AXIOM(it2 != markers.end());
    TraceEventTree::MarkerValuesMap::const_iterator it3 =
        markers.find(TfToken("Dynamic Marker 1"));
    TF_AXIOM(it3 != markers.end());

    return 0;
}