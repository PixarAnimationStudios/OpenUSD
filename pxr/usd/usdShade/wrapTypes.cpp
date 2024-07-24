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

#include <boost/python/enum.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdShadeTypes()
{
    enum_<UsdShadeAttributeType>("AttributeType")
        .value("Invalid", UsdShadeAttributeType::Invalid)
        .value("Input", UsdShadeAttributeType::Input)
        .value("Output", UsdShadeAttributeType::Output)
        ;

    enum_<UsdShadeConnectionModification>("ConnectionModification")
        .value("Replace", UsdShadeConnectionModification::Replace)
        .value("Prepend", UsdShadeConnectionModification::Prepend)
        .value("Append", UsdShadeConnectionModification::Append)
        ;

    to_python_converter<
        UsdShadeAttributeVector,
        TfPySequenceToPython<UsdShadeAttributeVector>>();
    TfPyContainerConversions::from_python_sequence<
        UsdShadeAttributeVector,
        TfPyContainerConversions::variable_capacity_policy>();

    to_python_converter<
        UsdShadeSourceInfoVector,
        TfPySequenceToPython<UsdShadeSourceInfoVector>>();
    TfPyContainerConversions::from_python_sequence<
        UsdShadeSourceInfoVector,
        TfPyContainerConversions::variable_capacity_policy>();
}
