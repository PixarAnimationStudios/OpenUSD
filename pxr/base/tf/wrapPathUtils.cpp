//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file tf/wrapPathUtils.cpp
#include "pxr/pxr.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/diagnostic.h"
#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE


#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/stl/string.h>

namespace {

static string
_RealPath(string const &path, bool allowInaccessibleSuffix, bool raiseOnError)
{
    string error;
    string realPath = TfRealPath(path, allowInaccessibleSuffix, &error);
    if (raiseOnError && !error.empty()) {
        TF_RUNTIME_ERROR(error);
    }
    return realPath;
}

static string::size_type
_FindLongestAccessiblePrefix(string const &path)
{
    // For python, we convert npos into path's length, which makes the return
    // value correct to use in slices.
    string error;
    string::size_type result = TfFindLongestAccessiblePrefix(path, &error);

    if (!error.empty()) {
        PyErr_SetString(PyExc_OSError, error.c_str());
        throw nanobind::python_error();
    }

    return result;
}

} // anonymous namespace 

void wrapPathUtils(nanobind::module_& m)
{
    namespace nb = nanobind;
    m.def("RealPath", _RealPath,
          nb::arg("path"),
          nb::arg("allowInaccessibleSuffix") = false,
          nb::arg("raiseOnError") = false);

    m.def("FindLongestAccessiblePrefix", _FindLongestAccessiblePrefix);
}
