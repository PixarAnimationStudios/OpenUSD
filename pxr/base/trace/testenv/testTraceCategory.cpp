//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/trace/trace.h"
#include "pxr/base/trace/collectionNotice.h"
#include "pxr/base/trace/reporter.h"

PXR_NAMESPACE_USING_DIRECTIVE

// Declare a custom Trace category
struct PerfCategory {
    static constexpr TraceCategoryId GetId() { 
        return TraceCategory::CreateTraceCategoryId("CustomPerfCounter"); 
    }
    static bool IsEnabled() { return TraceCollector::IsEnabled(); }
};

// Record a scope and counter with the new category
void TestCounters()
{
    constexpr static TraceStaticKeyData scopeKey("TestScope");
    constexpr static TraceStaticKeyData counterKey1("Test Counter 1");
    TraceCollector::GetInstance().BeginScope<PerfCategory>(scopeKey);
    TraceCollector::GetInstance().RecordCounterDelta<PerfCategory>(counterKey1, 1);
    TraceCollector::GetInstance().EndScope<PerfCategory>(scopeKey);
    TRACE_COUNTER_DELTA("Default Category counter", 1);
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
    bool AcceptsCategory(TraceCategoryId id) override {
        return id == PerfCategory::GetId();
    }
    
    void OnEvent(
        const TraceThreadId&, const TfToken& k, const TraceEvent& e) override {
        if (e.GetType() != TraceEvent::EventType::CounterDelta) {
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
    void OnBeginCollection() override {}
    void OnEndCollection() override {}
    void OnBeginThread(const TraceThreadId&) override {}
    void OnEndThread(const TraceThreadId&) override {}

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
        PerfCategory::GetId(), "CustomPerfCounter");

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
