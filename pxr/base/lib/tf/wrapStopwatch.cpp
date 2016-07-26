//
// Copyright 2016 Pixar
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
#include "pxr/base/tf/stopwatch.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python/class.hpp>

#include <string>

using std::string;

using namespace boost::python;

void wrapStopwatch() {

    typedef TfStopwatch This;

    class_<This>("Stopwatch",
                 init<optional<string const &, bool> >())

        .def("GetNamedStopwatch", This::GetNamedStopwatch)
        .staticmethod("GetNamedStopwatch")

        .def("GetStopwatchNames", This::GetStopwatchNames,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetStopwatchNames")
        
        .def("Start", &This::Start)
        .def("Stop", &This::Stop)
        .def("Reset", &This::Reset)
        .def("AddFrom", &This::AddFrom)
        .add_property("name",
                      make_function(&This::GetName,
                                    return_value_policy<return_by_value>()))

        .add_property("nanoseconds", &This::GetNanoseconds)
        .add_property("microseconds", &This::GetMicroseconds)
        .add_property("milliseconds", &This::GetMilliseconds)
        .add_property("sampleCount", &This::GetSampleCount)
        .add_property("seconds", &This::GetSeconds)
        .add_property("isShared", &This::IsShared)
        ;
}
                 
