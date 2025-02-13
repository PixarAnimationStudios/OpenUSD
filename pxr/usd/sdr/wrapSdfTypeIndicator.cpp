//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/usd/sdr/sdfTypeIndicator.h"

#include "pxr/external/boost/python.hpp"
#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapSdfTypeIndicator()
{
    typedef SdrSdfTypeIndicator This;
    scope().attr("SdfTypeIndicator") =
        TfPyGetClassObject<NdrSdfTypeIndicator>();

    // NOTE: Once Ndr is removed, the above alias will be removed and
    // replaced with the below class registration. This is necessary
    // because SdrSdfTypeIndicator is a direct c++ typedef of
    // NdrSdfTypeIndicator, and we want to avoid double-registering
    // the same class.
    // class_<This>("SdfTypeIndicator", no_init)
    //     .def("GetSdfType", &This::GetSdfType)
    //     .def("HasSdfType", &This::HasSdfType)
    //     .def("GetSdrType", &This::GetSdrType)
    // ;
}
