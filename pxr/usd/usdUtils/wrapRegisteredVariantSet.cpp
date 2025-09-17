//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python.hpp"

#include "pxr/usd/usdUtils/registeredVariantSet.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyEnum.h"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapRegisteredVariantSet()
{
    scope registeredVariantSet =
        class_<UsdUtilsRegisteredVariantSet>(
                        "RegisteredVariantSet", 
                        "Info for registered variant set",
                        no_init)
            .def_readonly("name", &UsdUtilsRegisteredVariantSet::name)
            .def_readonly("selectionExportPolicy", &UsdUtilsRegisteredVariantSet::selectionExportPolicy)
    ;
    
    typedef UsdUtilsRegisteredVariantSet::SelectionExportPolicy SelectionExportPolicy;
    enum_<SelectionExportPolicy>("SelectionExportPolicy")
        .value("IfAuthored", SelectionExportPolicy::IfAuthored)
        .value("Always", SelectionExportPolicy::Always)
        .value("Never", SelectionExportPolicy::Never)
    ;
    
}
