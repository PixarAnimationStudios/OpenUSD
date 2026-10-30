//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyModuleNotice.h"
#include "pxr/base/tf/pyNoticeWrapper.h"

PXR_NAMESPACE_USING_DIRECTIVE

void wrapPyModuleNotice(nanobind::module_ &m) {

    TfNbNoticeWrapper<TfPyModuleWasLoaded, TfNotice>::Wrap(
        m, "PyModuleWasLoaded")
        .def("name", &TfPyModuleWasLoaded::GetName)
        ;
}
