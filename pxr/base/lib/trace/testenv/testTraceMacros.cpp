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