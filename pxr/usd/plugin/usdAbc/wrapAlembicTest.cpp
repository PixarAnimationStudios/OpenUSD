//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/external/boost/python/def.hpp"

#include "pxr/usd/plugin/usdAbc/alembicTest.h"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapUsdAbcAlembicTest()
{
    def("_TestAlembic", UsdAbc_TestAlembic, arg("pathname"));
    def("_WriteAlembic", UsdAbc_WriteAlembic,
        (arg("srcPathname"), arg("dstPathname")));
}
