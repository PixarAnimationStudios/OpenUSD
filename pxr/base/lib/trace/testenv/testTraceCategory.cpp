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
#include "pxr/base/trace/collectionNotice.h"
#include "pxr/base/trace/reporter.h"

PXR_NAMESPACE_USING_DIRECTIVE

// Declare a custom Trace category
constexpr TraceCategoryId PerfCategory 
    = TraceCategory::CreateTraceCategoryId("CustomPerfCounter");

// Record a scope and counter with the new category
void TestCounters()
{
    constexpr static TraceStaticKeyData scopeKey("TestScope");
    constexpr static TraceStaticKeyData counterKey1("Test Counter 1");
    TraceCollector::GetInstance().BeginScope(scopeKey, PerfCategory);
    TraceCollector::GetInstance().RecordCounterValue(counterKey1, 1, PerfCategory);
    TraceCollector::GetInstance().EndScope(scopeKey, PerfCategory);
    TRACE_COUNTER("Default Category counter", 1);
}

// Simple Reporter that accumulates all the counters in a custom category

TF_DECLARE_WEAK_AND_REF_PTRS(PerfReporter);
class PerfReporter : 
    public TraceCollection::Visitor, public TfRefBase, public TfWeakBase  {
public:
    using This = PerfReporter;

    PerfReporter() {
        TfNotice::Register(TfCreateWeakPtr(this), &This::_OnCollection);
    }
    virtual bool AcceptsCategory(TraceCategoryId id) {
        return id == PerfCategory;
    }
    
    virtual void OnEvent(
        const TraceThreadId&, const TfToken& k, const TraceEvent& e) override {
        if (e.GetType() != TraceEvent::EventType::Counter) {
            return;
        }

        std::string key = k.GetString();
        CounterTable::iterator it = counters.find(key);
        if (it == counters.end()) {
            counters.insert({key, e.GetCounterValue()});
        } else {
            it->second += e.GetCounterValue();
        }
        printf("Perf counter event: %s %f\n", key.c_str(), e.GetCounterValue());
    }

    // Callbacks that are not used
    virtual void OnBeginCollection() override {}
    virtual void OnEndCollection() override {}
    virtual void OnBeginThread(const TraceThreadId&) override {}
    virtual void OnEndThread(const TraceThreadId&) override {}

    bool HasCounter(const std::string& key) const {
        return counters.find(key) != counters.end();
    }
    double GetCounterValue(const std::string& key) {
        return counters[key];
    }

private:
    void _OnCollection(const TraceCollectionAvailable& notice) {
        notice.GetCollection()->Iterate(*this);
    }

    using CounterTable = std::map<std::string, double>;
    CounterTable counters;
};

int
main(int argc, char *argv[])
{
    PerfReporterRefPtr perfReporter =
        TfCreateRefPtr(new PerfReporter());
    TraceCategory::GetInstance().RegisterCategory(
        PerfCategory, "CustomPerfCounter");

    TraceCollector* collector = &TraceCollector::GetInstance();
    TraceReporterPtr reporter = TraceReporter::GetGlobalReporter();
    collector->SetEnabled(true);
    for (int i = 0; i < 3; i++) {
        TestCounters();
    }
    collector->SetEnabled(false);
    // This will trigger processing by the custom reporter
    collector->CreateCollection();

    // Make sure we found events for the custom counter
    TF_AXIOM(perfReporter->HasCounter("Test Counter 1"));
    TF_AXIOM(perfReporter->GetCounterValue("Test Counter 1") == 3.0);

    // Make sure default category events were filtered out
    TF_AXIOM(!perfReporter->HasCounter("Default Category counter"));
}