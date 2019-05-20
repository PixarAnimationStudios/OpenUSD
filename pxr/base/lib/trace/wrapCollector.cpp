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

#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pySingleton.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/object.hpp>

#include <string>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

using PythonKey = std::string;
static double
GetElapsedSeconds(TraceEvent::TimeStamp begin, TraceEvent::TimeStamp end)
{
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
};


