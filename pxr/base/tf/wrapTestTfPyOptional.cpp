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
///
/// \file tf/wrapTestTfPython.cpp

#include "pxr/pxr.h"

#include "pxr/base/tf/pyOptional.h"

#include <boost/python/class.hpp>
#include <boost/python/tuple.hpp>

#include <string>
#include <vector>

using namespace boost::python;
using std::string;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// ////////////////////////////////
// // optional

static tuple
_TakesOptional(
    const boost::optional<string>& optString,
    const boost::optional<vector<string> >& optStrvec)
{
    object strObj;
    if (optString) {
        strObj = object(*optString);
    }
    object vecObj;
    if (optStrvec) {
        vecObj = object(TfPyCopySequenceToList(*optStrvec));
    }
    return make_tuple(strObj, vecObj);
}

template <typename T>
static boost::optional<T>
_TestOptional(
    const boost::optional<T>& opt)
{
    fprintf(stderr, "TestOptional<%s>\n", ArchGetDemangled<T>().c_str());
    return opt;
}

struct Tf_TestPyOptional { };

} // anonymous namespace 

void wrapTf_TestTfPyOptional()
{
    class_<Tf_TestPyOptional, boost::noncopyable>("Tf_TestPyOptional")
        .def("TakesOptional", _TakesOptional,
            ( arg("optString") = boost::optional<string>(),
              arg("optStrvec") = boost::optional<vector<string> >() ))
        .staticmethod("TakesOptional")

        .def("TestOptionalStringVector", _TestOptional<std::vector<std::string> >)
        .staticmethod("TestOptionalStringVector")
        .def("TestOptionalString",       _TestOptional<std::string>)
        .staticmethod("TestOptionalString")
        .def("TestOptionalDouble",       _TestOptional<double>)
        .staticmethod("TestOptionalDouble")
        .def("TestOptionalFloat",        _TestOptional<float>)
        .staticmethod("TestOptionalFloat")
        .def("TestOptionalLong",         _TestOptional<long>)
        .staticmethod("TestOptionalLong")
        .def("TestOptionalULong",        _TestOptional<unsigned long>)
        .staticmethod("TestOptionalULong")
        .def("TestOptionalInt",          _TestOptional<int>)
        .staticmethod("TestOptionalInt")
        .def("TestOptionalUInt",         _TestOptional<unsigned int>)
        .staticmethod("TestOptionalUInt")
        .def("TestOptionalShort",        _TestOptional<short>)
        .staticmethod("TestOptionalShort")
        .def("TestOptionalUShort",       _TestOptional<unsigned short>)
        .staticmethod("TestOptionalUShort")
        .def("TestOptionalChar",         _TestOptional<char>)
        .staticmethod("TestOptionalChar")
        .def("TestOptionalUChar",        _TestOptional<unsigned char>)
        .staticmethod("TestOptionalUChar")
        ;
}
