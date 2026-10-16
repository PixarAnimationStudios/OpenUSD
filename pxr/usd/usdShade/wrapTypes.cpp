//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/usdShade/types.h"
#include "pxr/usd/usdShade/connectableAPI.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>

#include "pxr/base/tf/pyEnum.h"

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdShadeTypes(nanobind::module_ &m)
{
    nb::enum_<UsdShadeAttributeType>(m, "AttributeType")
        .value("Invalid", UsdShadeAttributeType::Invalid)
        .value("Input", UsdShadeAttributeType::Input)
        .value("Output", UsdShadeAttributeType::Output)
        ;

    nb::enum_<UsdShadeConnectionModification>(m, "ConnectionModification")
        .value("Replace", UsdShadeConnectionModification::Replace)
        .value("Prepend", UsdShadeConnectionModification::Prepend)
        .value("Append", UsdShadeConnectionModification::Append)
        ;

    TfPyContainerConversions::from_python_sequence<
        UsdShadeAttributeVector,
        TfPyContainerConversions::variable_capacity_policy>();

    TfPyContainerConversions::from_python_sequence<
        UsdShadeSourceInfoVector,
        TfPyContainerConversions::variable_capacity_policy>();
}
