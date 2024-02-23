//
// Copyright 2023 Pixar
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
#include "pxr/base/ts/loopParams.h"
#include "pxr/base/tf/pyUtils.h"

#include <boost/python.hpp>

#include <string>
#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace boost::python;


static std::string
_GetRepr(const TsLoopParams & params)
{
    // This takes advantage of our operator<<, which produces a string that
    // makes a valid Python tuple, or a parenthesized set of args.
    std::ostringstream result;
    result << TF_PY_REPR_PREFIX
           << "LoopParams"
           << params;
    return result.str();
}


void wrapLoopParams()
{
    typedef TsLoopParams This;

    class_<This>("LoopParams", init<>())
        .def(init<bool, TsTime, TsTime, TsTime, TsTime, double>())

        .add_property("looping",
                      &This::GetLooping,
                      &This::SetLooping)

        .add_property("start",
                      &This::GetStart)

        .add_property("period",
                      &This::GetPeriod)

        .add_property("preRepeatFrames",
                      &This::GetPreRepeatFrames)

        .add_property("repeatFrames",
                      &This::GetRepeatFrames)

        .def("GetMasterInterval", &This::GetMasterInterval,
                return_value_policy<return_by_value>())

        .def("GetLoopedInterval", &This::GetLoopedInterval,
                return_value_policy<return_by_value>())

        .def("IsValid", &This::IsValid)

        .add_property("valueOffset",
                      &This::GetValueOffset,
                      &This::SetValueOffset)

        .def("__repr__", &::_GetRepr)

        .def(self == self)
        .def(self != self)
        ;
}
