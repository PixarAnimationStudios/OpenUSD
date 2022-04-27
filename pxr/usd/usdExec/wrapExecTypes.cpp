//
// unlicensed 2022 benmalartre
//
#include <iostream>
#include "pxr/pxr.h"

#include "pxr/usd/usdExec/execTypes.h"
#include "pxr/usd/usdExec/execConnectableAPI.h"

#include "pxr/base/tf/pyContainerConversions.h"

#include <boost/python/enum.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdExecTypes()
{
    enum_<UsdExecAttributeType>("AttributeType")
        .value("Invalid", UsdExecAttributeType::Invalid)
        .value("Input", UsdExecAttributeType::Input)
        .value("Output", UsdExecAttributeType::Output)
        ;

    enum_<UsdExecConnectionModification>("ConnectionModification")
        .value("Replace", UsdExecConnectionModification::Replace)
        .value("Prepend", UsdExecConnectionModification::Prepend)
        .value("Append", UsdExecConnectionModification::Append)
        ;

    to_python_converter<
        UsdExecAttributeVector,
        TfPySequenceToPython<UsdExecAttributeVector>>();
    TfPyContainerConversions::from_python_sequence<
        UsdExecAttributeVector,
        TfPyContainerConversions::variable_capacity_policy>();

    to_python_converter<
        UsdExecSourceInfoVector,
        TfPySequenceToPython<UsdExecSourceInfoVector>>();
    TfPyContainerConversions::from_python_sequence<
        UsdExecSourceInfoVector,
        TfPyContainerConversions::variable_capacity_policy>();
}
