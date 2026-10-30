//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/usd/sdr/sdfTypeIndicator.h"

#include <nanobind/nanobind.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void wrapSdfTypeIndicator(nanobind::module_ &m)
{
    typedef SdrSdfTypeIndicator This;
    nb::class_<This>(m, "SdfTypeIndicator")
        .def("GetSdfType", &This::GetSdfType)
        .def("HasSdfType", &This::HasSdfType)
        .def("GetSdrType", &This::GetSdrType)
    ;
}
