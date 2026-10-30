//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/callContext.h"
#include <string>

using std::string;
PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static string
_GetFileString(TfCallContext const& cc) {
    return string(cc.GetFile());
}

static string
_GetFunctionString(TfCallContext const& cc) {
    return string(cc.GetFunction());
}

static string
_GetPrettyFunctionString(TfCallContext const& cc) {
    return string(cc.GetPrettyFunction());
}

} // anonymous namespace 

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/stl/string.h>

void wrapCallContext(nanobind::module_& m) {
    typedef TfCallContext This;

    nanobind::class_<This>(m, "CallContext")
        .def_prop_ro("file", &_GetFileString)
        .def_prop_ro("function", &_GetFunctionString)
        .def_prop_ro("line", &This::GetLine)
        .def_prop_ro("prettyFunction", &_GetPrettyFunctionString)
    ;
}
