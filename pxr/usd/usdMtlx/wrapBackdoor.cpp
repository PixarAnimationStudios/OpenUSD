//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/usdMtlx/backdoor.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/base/tf/makePyConstructor.h"

#include "pxr/external/boost/python/def.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapUsdMtlxBackdoor()
{
    def("_TestString", UsdMtlx_TestString,
        (arg("buffer"), arg("nodeGraphs") = false),
        return_value_policy<TfPyRefPtrFactory<>>());
    def("_TestFile", UsdMtlx_TestFile,
        (arg("pathname"), arg("nodeGraphs") = false),
        return_value_policy<TfPyRefPtrFactory<>>());
}
