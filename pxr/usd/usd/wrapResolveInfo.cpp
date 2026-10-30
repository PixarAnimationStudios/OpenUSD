//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/resolveInfo.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void wrapUsdResolveInfo(nanobind::module_ &m)
{
    nb::class_<UsdResolveInfo>(m, "ResolveInfo")
        .def(nb::init<>())
        .def("GetSource", &UsdResolveInfo::GetSource)
        .def("HasAuthoredValueOpinion",
             &UsdResolveInfo::HasAuthoredValueOpinion)
        .def("HasAuthoredValue",
             &UsdResolveInfo::HasAuthoredValue)
        .def("GetNode", &UsdResolveInfo::GetNode)
        .def("ValueIsBlocked", &UsdResolveInfo::ValueIsBlocked)
        .def("ValueSourceMightBeTimeVarying",
             &UsdResolveInfo::ValueSourceMightBeTimeVarying)
        .def("HasNextWeakerInfo",
             &UsdResolveInfo::HasNextWeakerInfo)
        .def("GetNextWeakerInfo",
             &UsdResolveInfo::GetNextWeakerInfo,
             nb::rv_policy::reference_internal)
        ;

    TfNbWrapEnum<UsdResolveInfoSource>(m);
}

