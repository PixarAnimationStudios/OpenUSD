//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/trace/reporter.h"
#include "pxr/base/trace/eventTree.h"
#include "pxr/base/tf/stringUtils.h"

#include <chrono>
#include <iostream>
#include <thread>

using namespace std::chrono_literals;

PXR_NAMESPACE_USING_DIRECTIVE

static void
TestMarkerMacro() {
    TRACE_MARKER("Static Marker A");
    std::this_thread::sleep_for(1us);
    TRACE_MARKER("Static Marker B");
    std::this_thread::sleep_for(1us);
    TRACE_MARKER("Static Marker C");
    std::this_thread::sleep_for(1us);

    TRACE_MARKER_DYNAMIC(TfStringPrintf("Dynamic Marker %s", "A"));
    std::this_thread::sleep_for(1us);
    TRACE_MARKER_DYNAMIC(TfStringPrintf("Dynamic Marker %s", "B"));
    std::this_thread::sleep_for(1us);
    TRACE_MARKER_DYNAMIC(TfStringPrintf("Dynamic Marker %s", "C"));
    std::this_thread::sleep_for(1us);
}


static TraceEventTree::MarkerValues
GetTimeOfMarker(const std::string& MarkerName, const TraceEventTree::MarkerValuesMap& Markers) {
    TraceEventTree::MarkerValuesMap::const_iterator it = Markers.find(TfToken(MarkerName));
    TF_AXIOM(it!= Markers.end());
    return it->second;
}

int main(int argc, char* argv[]) {
    TraceCollector* collector = &TraceCollector::GetInstance();
    TraceReporterPtr reporter = TraceReporter::GetGlobalReporter();
    collector->SetEnabled(true);
    TestMarkerMacro();
    collector->SetEnabled(false);
    reporter->ReportChromeTracing(std::cout);

    TraceEventTreeRefPtr timeline = reporter->GetEventTree();
    TF_AXIOM(timeline);
    const TraceEventTree::MarkerValuesMap& Markers =
        timeline->GetMarkers();

    // Test that the Markers are recorded in order
    TraceEvent::TimeStamp asTime = GetTimeOfMarker("Static Marker A", Markers)[0].first;
    TF_AXIOM(GetTimeOfMarker("Static Marker A", Markers).size() == 1);
    TraceEvent::TimeStamp bsTime = GetTimeOfMarker("Static Marker B", Markers)[0].first;
    TraceEvent::TimeStamp csTime = GetTimeOfMarker("Static Marker C", Markers)[0].first;
    TF_AXIOM(asTime < bsTime && bsTime < csTime);

    TraceEvent::TimeStamp adTime = GetTimeOfMarker("Dynamic Marker A", Markers)[0].first;
    TraceEvent::TimeStamp bdTime = GetTimeOfMarker("Dynamic Marker B", Markers)[0].first;
    TraceEvent::TimeStamp cdTime = GetTimeOfMarker("Dynamic Marker C", Markers)[0].first;
    TF_AXIOM(csTime < adTime && adTime < bdTime && bdTime < cdTime);

    // Run a second time to test merging
    collector->SetEnabled(true);
    TestMarkerMacro();
    collector->SetEnabled(false);
    reporter->ReportChromeTracing(std::cout);
    TraceEventTreeRefPtr timeline2 = reporter->GetEventTree();
    const TraceEventTree::MarkerValuesMap& Markers2 =
        timeline2->GetMarkers();

    size_t numSA = GetTimeOfMarker("Static Marker A", Markers2).size();
    TF_AXIOM(numSA == 2);
    size_t numSB = GetTimeOfMarker("Static Marker B", Markers2).size();
    TF_AXIOM(numSB == 2);
    size_t numSC = GetTimeOfMarker("Static Marker C", Markers2).size();
    TF_AXIOM(numSC == 2);

    size_t numDA = GetTimeOfMarker("Dynamic Marker A", Markers2).size();
    TF_AXIOM(numDA == 2);
    size_t numDB = GetTimeOfMarker("Dynamic Marker B", Markers2).size();
    TF_AXIOM(numDB == 2);
    size_t numDC = GetTimeOfMarker("Dynamic Marker C", Markers2).size();
    TF_AXIOM(numDC == 2);

    // Test clearing
    reporter->ClearTree();
    collector->SetEnabled(true);
    TestMarkerMacro();
    collector->SetEnabled(false);
    reporter->ReportChromeTracing(std::cout);
    TraceEventTreeRefPtr timeline3 = reporter->GetEventTree();
    const TraceEventTree::MarkerValuesMap& Markers3 =
        timeline3->GetMarkers();

    numSA = GetTimeOfMarker("Static Marker A", Markers3).size();
    TF_AXIOM(numSA == 1);
    numSB = GetTimeOfMarker("Static Marker B", Markers3).size();
    TF_AXIOM(numSB == 1);
    numSC = GetTimeOfMarker("Static Marker C", Markers3).size();
    TF_AXIOM(numSC == 1);

    numDA = GetTimeOfMarker("Dynamic Marker A", Markers3).size();
    TF_AXIOM(numDA == 1);
    numDB = GetTimeOfMarker("Dynamic Marker B", Markers3).size();
    TF_AXIOM(numDB == 1);
    numDC = GetTimeOfMarker("Dynamic Marker C", Markers3).size();
    TF_AXIOM(numDC == 1);

}
