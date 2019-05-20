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
#include "pxr/pxr.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/trace/reporter.h"
#include "pxr/base/trace/eventTree.h"
#include "pxr/base/tf/stringUtils.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static void
TestMarkerMacro() {
    TRACE_MARKER("Static Marker A")
    TRACE_MARKER("Static Marker B")
    TRACE_MARKER("Static Marker C")

    TRACE_MARKER_DYNAMIC(TfStringPrintf("Dynamic Marker %s", "A"));
    TRACE_MARKER_DYNAMIC(TfStringPrintf("Dynamic Marker %s", "B"));
    TRACE_MARKER_DYNAMIC(TfStringPrintf("Dynamic Marker %s", "C"));
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
