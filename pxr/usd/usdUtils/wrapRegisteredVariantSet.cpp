//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

#include "pxr/usd/usdUtils/registeredVariantSet.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void wrapRegisteredVariantSet(nanobind::module_ &m)
{
    nb::class_<UsdUtilsRegisteredVariantSet> registeredVariantSet(
                    m,
                    "RegisteredVariantSet", 
                    "Info for registered variant set");
    registeredVariantSet
        .def_ro("name", &UsdUtilsRegisteredVariantSet::name)
        .def_ro("selectionExportPolicy", &UsdUtilsRegisteredVariantSet::selectionExportPolicy)
    ;
    
    typedef UsdUtilsRegisteredVariantSet::SelectionExportPolicy SelectionExportPolicy;
    nb::enum_<SelectionExportPolicy>(registeredVariantSet, "SelectionExportPolicy")
        .value("IfAuthored", SelectionExportPolicy::IfAuthored)
        .value("Always", SelectionExportPolicy::Always)
        .value("Never", SelectionExportPolicy::Never)
        .export_values()
    ;
    
}
