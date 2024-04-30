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

#include "pxr/base/trace/collector.h"

#include "pxr/base/tf/pySingleton.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/return_value_policy.hpp>

#include <string>

using namespace boost::python;

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
    const std::string& phase, const object& info)
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

    const std::size_t generation = extract<std::size_t>(info["generation"]);
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

void wrapCollector()
{
    using This = TraceCollector;
    using ThisPtr = TfWeakPtr<TraceCollector>;

    class_<This, ThisPtr, boost::noncopyable>("Collector", no_init)
        .def(TfPySingleton())

        .def("BeginEvent", BeginEventHelper)
        .def("EndEvent", EndEventHelper)

        .def("BeginEventAtTime", BeginEventAtTimeHelper)
        .def("EndEventAtTime", EndEventAtTimeHelper)

        .def("GetLabel", &This::GetLabel,
             return_value_policy<return_by_value>())
        
        .def("Clear", &This::Clear)

        .add_property("enabled", IsEnabledHelper, &This::SetEnabled)
        .add_property("pythonTracingEnabled",
                      &This::IsPythonTracingEnabled,
                      &This::SetPythonTracingEnabled)
        ;
    
    def("GetElapsedSeconds", GetElapsedSeconds);
    def("PythonGarbageCollectionCallback", PythonGarbageCollectionCallback);
};


