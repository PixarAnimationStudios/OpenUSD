//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"


#include "pxr/base/tf/fileUtils.h"

#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/stl/string.h>

void wrapFileUtils(nanobind::module_& m) {
    namespace nb = nanobind;
    m.def("TouchFile", &TfTouchFile, nb::arg("fileName"), nb::arg("create") = false);
}
