//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/pySafePython.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

#include "pxr/usd/plugin/usdAbc/alembicTest.h"

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdAbcAlembicTest(nb::module_ &m)
{
    m.def("_TestAlembic", UsdAbc_TestAlembic, nb::arg("pathname"));
    m.def("_WriteAlembic", UsdAbc_WriteAlembic,
          nb::arg("srcPathname"), nb::arg("dstPathname"));
}
