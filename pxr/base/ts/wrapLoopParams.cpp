//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
