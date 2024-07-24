//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

template <template <typename> typename Optional>
static tuple
_TakesOptional(
    const Optional<string>& optString,
    const Optional<vector<string> >& optStrvec)
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

template <template <typename> typename Optional, typename T>
static Optional<T>
_TestOptional(
    const Optional<T>& opt)
{
    fprintf(stderr, "TestOptional<%s>\n", 
        ArchGetDemangled<Optional<T>>().c_str());
    return opt;
}

struct Tf_TestPyOptionalBoost { };
struct Tf_TestPyOptionalStd { };

} // anonymous namespace 

void wrapTf_TestTfPyOptional()
{
    class_<Tf_TestPyOptionalBoost, boost::noncopyable>("Tf_TestPyOptionalBoost")
        .def("TakesOptional", _TakesOptional<boost::optional>,
            ( arg("optString") = boost::optional<string>(),
              arg("optStrvec") = boost::optional<vector<string> >() ))
        .staticmethod("TakesOptional")

        .def("TestOptionalStringVector",
            _TestOptional<boost::optional, std::vector<std::string> >)
        .staticmethod("TestOptionalStringVector")
        .def("TestOptionalString",
            _TestOptional<boost::optional, std::string>)
        .staticmethod("TestOptionalString")
        .def("TestOptionalDouble",
            _TestOptional<boost::optional, double>)
        .staticmethod("TestOptionalDouble")
        .def("TestOptionalFloat",
            _TestOptional<boost::optional, float>)
        .staticmethod("TestOptionalFloat")
        .def("TestOptionalLong",
            _TestOptional<boost::optional, long>)
        .staticmethod("TestOptionalLong")
        .def("TestOptionalULong",
            _TestOptional<boost::optional, unsigned long>)
        .staticmethod("TestOptionalULong")
        .def("TestOptionalInt",
            _TestOptional<boost::optional, int>)
        .staticmethod("TestOptionalInt")
        .def("TestOptionalUInt",
            _TestOptional<boost::optional, unsigned int>)
        .staticmethod("TestOptionalUInt")
        .def("TestOptionalShort",
            _TestOptional<boost::optional, short>)
        .staticmethod("TestOptionalShort")
        .def("TestOptionalUShort",
            _TestOptional<boost::optional, unsigned short>)
        .staticmethod("TestOptionalUShort")
        .def("TestOptionalChar",
            _TestOptional<boost::optional, char>)
        .staticmethod("TestOptionalChar")
        .def("TestOptionalUChar",
            _TestOptional<boost::optional, unsigned char>)
        .staticmethod("TestOptionalUChar")
        ;

    class_<Tf_TestPyOptionalStd, boost::noncopyable>("Tf_TestPyOptionalStd")
        .def("TakesOptional", _TakesOptional<std::optional>,
            ( arg("optString") = std::optional<string>(),
              arg("optStrvec") = std::optional<vector<string> >() ))
        .staticmethod("TakesOptional")

        .def("TestOptionalStringVector",
            _TestOptional<std::optional, std::vector<std::string> >)
        .staticmethod("TestOptionalStringVector")
        .def("TestOptionalString",
            _TestOptional<std::optional, std::string>)
        .staticmethod("TestOptionalString")
        .def("TestOptionalDouble",
            _TestOptional<std::optional, double>)
        .staticmethod("TestOptionalDouble")
        .def("TestOptionalFloat",
            _TestOptional<std::optional, float>)
        .staticmethod("TestOptionalFloat")
        .def("TestOptionalLong",
            _TestOptional<std::optional, long>)
        .staticmethod("TestOptionalLong")
        .def("TestOptionalULong",
            _TestOptional<std::optional, unsigned long>)
        .staticmethod("TestOptionalULong")
        .def("TestOptionalInt",
            _TestOptional<std::optional, int>)
        .staticmethod("TestOptionalInt")
        .def("TestOptionalUInt",
            _TestOptional<std::optional, unsigned int>)
        .staticmethod("TestOptionalUInt")
        .def("TestOptionalShort",
            _TestOptional<std::optional, short>)
        .staticmethod("TestOptionalShort")
        .def("TestOptionalUShort",
            _TestOptional<std::optional, unsigned short>)
        .staticmethod("TestOptionalUShort")
        .def("TestOptionalChar",
            _TestOptional<std::optional, char>)
        .staticmethod("TestOptionalChar")
        .def("TestOptionalUChar",
            _TestOptional<std::optional, unsigned char>)
        .staticmethod("TestOptionalUChar")
        ;
}
