//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/trace/collector.h"

#include "pxr/base/tf/pySingleton.h"


#include <string>

PXR_NAMESPACE_USING_DIRECTIVE


using PythonKey = std::string;

static double
GetElapsedSeconds(TraceEvent::TimeStamp begin, TraceEvent::TimeStamp end)
{
    if (begin > end) {
        TF_CODING_ERROR("Invalid interval: begin=%zu, end=%zu", begin, end);
        return 0.0;
    }
    return ArchTicksToSeconds(end-begin);
}

static TraceEvent::TimeStamp
BeginEventHelper(const TraceCollectorPtr& self, const PythonKey& key)
{
    return self->BeginEvent(key);
}

static TraceEvent::TimeStamp
EndEventHelper(const TraceCollectorPtr& self, const PythonKey& key)
{
    return self->EndEvent(key);
}

static void
BeginEventAtTimeHelper(
    const TraceCollectorPtr& self, const PythonKey& key, double ms)
{
    self->BeginEventAtTime(key, ms);
}

static void
EndEventAtTimeHelper(
    const TraceCollectorPtr& self, const PythonKey& key, double ms)
{
    self->EndEventAtTime(key, ms);
}

static bool
IsEnabledHelper(const TraceCollectorPtr& self) {
    return TraceCollector::IsEnabled();
}

// Generate Trace scopes for Python garbage collection passes.
//
// This can be added and removed dynamically in Python using:
//
//     import gc
//     gc.callbacks.append(Trace.PythonGarbageCollectionCallback)
//     # ...
//     gc.callbacks.remove(Trace.PythonGarbageCollectionCallback)
//
static void
PythonGarbageCollectionCallback(
    const std::string& phase, nanobind::dict info)
{
    if (!TraceCollector::IsEnabled()) {
        return;
    }

    // Python's default garbage collector organizes objects into three
    // generations so we provide a unique trace key for each one.  There
    // doesn't appear to be public API to query the number of generations but
    // this hasn't changed since the generational collector was introduced.
    // The collector used in the free-threaded build slated for release in
    // 3.13 is not generational but still reports a generation in [0, 2] when
    // invoking callbacks.
    static const TraceStaticKeyData keys[] = {
        {"Python Garbage Collection (generation: 0)"},
        {"Python Garbage Collection (generation: 1)"},
        {"Python Garbage Collection (generation: 2)"},
    };

    const std::size_t generation = nanobind::cast<std::size_t>(info["generation"]);
    if (generation >= std::size(keys)) {
        TF_WARN("'generation' %zu is out of range", generation);
        return;
    }

    TraceCollector& collector = TraceCollector::GetInstance();
    const TraceStaticKeyData& key = keys[generation];
    if (phase == "start") {
        collector.BeginScope(key);
    }
    else if (phase == "stop") {
        collector.EndScope(key);
    }
}

void wrapCollector(nanobind::module_& m)
{
    namespace nb = nanobind;
    using This = TraceCollector;

    nb::class_<This> cls(m, "Collector",
                         nb::is_weak_referenceable(), nb::dynamic_attr());
    TfNbSingleton(cls);
    cls.def("BeginEvent", BeginEventHelper)
        .def("EndEvent", EndEventHelper)

        .def("BeginEventAtTime", BeginEventAtTimeHelper)
        .def("EndEventAtTime", EndEventAtTimeHelper)

        .def("GetLabel", &This::GetLabel)
        
        .def("Clear", &This::Clear)

        .def_prop_rw("enabled", IsEnabledHelper, &This::SetEnabled)
        .def_prop_rw("pythonTracingEnabled",
                     &This::IsPythonTracingEnabled,
                     &This::SetPythonTracingEnabled)
        ;
    
    m.def("GetElapsedSeconds", GetElapsedSeconds);
    m.def("PythonGarbageCollectionCallback", PythonGarbageCollectionCallback);
};


