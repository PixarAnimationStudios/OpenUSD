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
#include "pxr/base/trace/singleEventGraph.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static void
TestCounters() {
    // All deltas
    TRACE_COUNTER_DELTA("Counter A", 1);
    TRACE_COUNTER_DELTA("Counter A", 2);
    TRACE_COUNTER_DELTA("Counter A", 3);

    // All values
    TRACE_COUNTER_VALUE("Counter B", 1);
    TRACE_COUNTER_VALUE("Counter B", 2);
    TRACE_COUNTER_VALUE("Counter B", 3);

    // value then delta
    TRACE_COUNTER_VALUE("Counter C", 5);
    TRACE_COUNTER_DELTA("Counter C", -1);
    TRACE_COUNTER_DELTA("Counter C", -2);

    // deltas then value
    TRACE_COUNTER_DELTA("Counter D", 1);
    TRACE_COUNTER_DELTA("Counter D", 2);
    TRACE_COUNTER_VALUE("Counter D", -5);
}

static void TestTimelineCounterValues(
    const TfToken& counterName, const std::vector<double> desiredValues)
{
    TraceReporterPtr reporter = TraceReporter::GetGlobalReporter();
    TraceSingleEventGraphRefPtr timeline = reporter->GetSingleEventGraph();
    TF_AXIOM(timeline);

    const TraceSingleEventGraph::CounterValuesMap& counters =
        timeline->GetCounters();
    TraceSingleEventGraph::CounterValuesMap::const_iterator it =
        counters.find(counterName);
    TF_AXIOM(it != counters.end());

    // The number and order of values should match.
    const TraceSingleEventGraph::CounterValues& values = it->second;
    TF_AXIOM(desiredValues.size() == values.size());

    std::vector<double>::const_iterator desiredIt = desiredValues.begin();
    for (auto& v : values) {
        TF_AXIOM(v.second == *desiredIt);
        ++desiredIt;
    }
}

static void TestAggregateCounterValue(
    const TfToken& counterName, const double desiredValue)
{
    TraceReporterPtr reporter = TraceReporter::GetGlobalReporter();
    const TraceReporter::CounterMap& counters = reporter->GetCounters();
    TraceReporter::CounterMap::const_iterator it =
        counters.find(counterName);
    TF_AXIOM(it != counters.end());
    TF_AXIOM(it->second == desiredValue);
}

int main(int argc, char* argv[]) {
    TraceCollector* collector = &TraceCollector::GetInstance();
    TraceReporterPtr reporter = TraceReporter::GetGlobalReporter();
    collector->SetEnabled(true);
    TestCounters();
    collector->SetEnabled(false);
    reporter->ReportChromeTracing(std::cout);

    // Test that the aggregate reporter works correctly.
    TestAggregateCounterValue(TfToken("Counter A"), 6.0);
    TestAggregateCounterValue(TfToken("Counter B"), 3.0);
    TestAggregateCounterValue(TfToken("Counter C"), 2.0);
    TestAggregateCounterValue(TfToken("Counter D"), -5.0);

    // Test that the timeline reporter works correctly
    TestTimelineCounterValues(TfToken("Counter A"), {1.0,3.0,6.0});
    TestTimelineCounterValues(TfToken("Counter B"), {1.0,2.0,3.0});
    TestTimelineCounterValues(TfToken("Counter C"), {5.0,4.0,2.0});
    TestTimelineCounterValues(TfToken("Counter D"), {1.0,3.0,-5.0});

    collector->SetEnabled(true);
    TestCounters();
    collector->SetEnabled(false);
    reporter->ReportChromeTracing(std::cout);

    // Test that the aggregate reporter works correctly.
    TestAggregateCounterValue(TfToken("Counter A"), 12.0);
    TestAggregateCounterValue(TfToken("Counter B"), 3.0);
    TestAggregateCounterValue(TfToken("Counter C"), 2.0);
    TestAggregateCounterValue(TfToken("Counter D"), -5.0);

    // Test that the timeline reporter works correctly
    TestTimelineCounterValues(TfToken("Counter A"), {1.0,3.0,6.0,7.0,9.0,12.0});
    TestTimelineCounterValues(TfToken("Counter B"), {1.0,2.0,3.0,1.0,2.0,3.0});
    TestTimelineCounterValues(TfToken("Counter C"), {5.0,4.0,2.0,5.0,4.0,2.0});
    TestTimelineCounterValues(TfToken("Counter D"), {1.0,3.0,-5.0,-4.0,-2.0,-5.0});

    return 0;
}