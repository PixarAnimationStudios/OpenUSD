//
// Copyright 2020 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
