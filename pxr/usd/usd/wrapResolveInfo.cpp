//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/resolveInfo.h"
#include "pxr/base/tf/pyEnum.h"

#include "pxr/external/boost/python/class.hpp"

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapUsdResolveInfo()
{
    class_<UsdResolveInfo>("ResolveInfo")
        .def("GetSource", &UsdResolveInfo::GetSource)
        .def("GetNode", &UsdResolveInfo::GetNode)
        .def("ValueIsBlocked", &UsdResolveInfo::ValueIsBlocked)
        ;

    TfPyWrapEnum<UsdResolveInfoSource>();
}
