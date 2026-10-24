//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/vector.h>

#include "pxr/usd/usdShade/utils.h"
#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/output.h"

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static nb::object 
_GetBaseNameAndType(const TfToken &fullName)
{
    const auto &result = UsdShadeUtils::GetBaseNameAndType(fullName);
    return nb::make_tuple(result.first, result.second);
}

} // anonymous namespace 

void wrapUsdShadeUtils(nanobind::module_ &m)
{
    typedef UsdShadeUtils This;

    UsdShadeAttributeVector (*GetValueProducingAttributes_Input)(
        const UsdShadeInput &input, bool includeAuthoredValues) = 
            &UsdShadeUtils::GetValueProducingAttributes;
    UsdShadeAttributeVector (*GetValueProducingAttributes_Output)(
        const UsdShadeOutput &output, bool includeAuthoredValues) = 
            &UsdShadeUtils::GetValueProducingAttributes;

    nb::class_<This>(m, "Utils")
        .def_static("GetPrefixForAttributeType", 
            &UsdShadeUtils::GetPrefixForAttributeType)

        .def_static("GetConnectedSourcePath", 
             &UsdShadeUtils::GetConnectedSourcePath, 
             nb::arg("connectionSourceInfo"))

        .def_static("GetBaseNameAndType", _GetBaseNameAndType)

        .def_static("GetType", &UsdShadeUtils::GetType)

        .def_static("GetFullName", &UsdShadeUtils::GetFullName)

        .def_static("GetValueProducingAttributes", GetValueProducingAttributes_Input,
            nb::arg("input"), nb::arg("shaderOutputsOnly")=false)
        .def_static("GetValueProducingAttributes", GetValueProducingAttributes_Output,
            nb::arg("output"), nb::arg("shaderOutputsOnly")=false)
        ;

}
