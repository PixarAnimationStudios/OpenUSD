//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/work/threadLimits.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>

PXR_NAMESPACE_USING_DIRECTIVE

void wrapThreadLimits(nanobind::module_ &m)
{
    m.def("GetConcurrencyLimit", &WorkGetConcurrencyLimit);
    m.def("HasConcurrency", &WorkHasConcurrency);
    m.def("GetPhysicalConcurrencyLimit", &WorkGetPhysicalConcurrencyLimit);

    m.def("SetConcurrencyLimit", &WorkSetConcurrencyLimit);
    m.def("SetConcurrencyLimitArgument", &WorkSetConcurrencyLimitArgument);
    m.def("SetMaximumConcurrencyLimit", &WorkSetMaximumConcurrencyLimit);
}
