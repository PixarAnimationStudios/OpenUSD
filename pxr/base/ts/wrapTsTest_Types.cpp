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
#include "pxr/base/ts/tsTest_Types.h"
#include "pxr/base/tf/pyContainerConversions.h"

#include <boost/python.hpp>
#include <sstream>
#include <string>
#include <cstdio>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace boost::python;


// Return a full-precision python repr for a double value.
static std::string
_HexFloatRepr(const double num)
{
    // XXX: work around std::hexfloat apparently not working in our libstdc++ as
    // of this writing.
    char buf[100];
    sprintf(buf, "float.fromhex('%a')", num);
    return std::string(buf);
}

static std::string
_SampleRepr(const TsTest_Sample &sample)
{
    std::ostringstream result;

    result << "Ts.TsTest_Sample("
           << _HexFloatRepr(sample.time)
           << ", " << _HexFloatRepr(sample.value)
           << ")";

    return result.str();
}


void wrapTsTest_Types()
{
    class_<TsTest_Sample>("TsTest_Sample")
        // Default init is not suppressed, so automatically created.
        .def(init<double, double>())
        .def(init<const TsTest_Sample&>())
        .def("__repr__", &_SampleRepr)
        .def_readwrite("time", &TsTest_Sample::time)
        .def_readwrite("value", &TsTest_Sample::value)
        ;

    to_python_converter<
        TsTest_SampleVec,
        TfPySequenceToPython<TsTest_SampleVec>>();
    TfPyContainerConversions::from_python_sequence<
        TsTest_SampleVec,
        TfPyContainerConversions::variable_capacity_policy>();
}
