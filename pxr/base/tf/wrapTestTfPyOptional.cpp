//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file tf/wrapTestTfPython.cpp

#include "pxr/pxr.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyOptional.h"

#include <nanobind/nanobind.h>

#include <optional>
#include <string>
#include <vector>

using std::string;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// ////////////////////////////////
// // optional

template <template <typename> typename Optional>
static nanobind::tuple
_TakesOptional(
    const Optional<string>& optString,
    const Optional<vector<string> >& optStrvec)
{
    nanobind::object strObj = optString
        ? nanobind::cast(*optString)
        : nanobind::none();
    nanobind::object vecObj = optStrvec
        ? nanobind::object(TfNbSequenceToList(*optStrvec))
        : nanobind::none();
    return nanobind::make_tuple(strObj, vecObj);
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

struct Tf_TestPyOptionalStd { };

} // anonymous namespace 

void wrapTf_TestTfPyOptional(nanobind::module_ &m)
{
    namespace nb = nanobind;

    nb::class_<Tf_TestPyOptionalStd>(m, "Tf_TestPyOptionalStd")
        .def_static("TakesOptional", _TakesOptional<std::optional>,
            nb::arg("optString") = std::optional<string>(),
            nb::arg("optStrvec") = std::optional<vector<string> >())

        .def_static("TestOptionalStringVector",
            _TestOptional<std::optional, std::vector<std::string> >)
        .def_static("TestOptionalString",
            _TestOptional<std::optional, std::string>)
        .def_static("TestOptionalDouble",
            _TestOptional<std::optional, double>)
        .def_static("TestOptionalFloat",
            _TestOptional<std::optional, float>)
        .def_static("TestOptionalLong",
            _TestOptional<std::optional, long>)
        .def_static("TestOptionalULong",
            _TestOptional<std::optional, unsigned long>)
        .def_static("TestOptionalInt",
            _TestOptional<std::optional, int>)
        .def_static("TestOptionalUInt",
            _TestOptional<std::optional, unsigned int>)
        .def_static("TestOptionalShort",
            _TestOptional<std::optional, short>)
        .def_static("TestOptionalUShort",
            _TestOptional<std::optional, unsigned short>)
        .def_static("TestOptionalChar",
            _TestOptional<std::optional, char>)
        .def_static("TestOptionalUChar",
            _TestOptional<std::optional, unsigned char>)
        ;
}
