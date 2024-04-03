//
// Copyright 2023 Pixar
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
#include "pxr/base/ts/types.h"
#include "pxr/base/ts/wrapUtils.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"

#include <boost/python.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace boost::python;


static void wrapValueSample()
{
    typedef TsValueSample This;

    class_<This>
        ("ValueSample", 
         "An individual sample.  A sample is either a blur, "
                       "defining a rectangle, or linear, defining a line for "
                       "linear interpolation. In both cases the sample is "
                       "half-open on the right.", 
         no_init)

        .def_readonly(
            "isBlur", &This::isBlur, "True if a blur sample")
        .def_readonly(
            "leftTime", &This::leftTime, "Left side time (inclusive)")
        .def_readonly(
            "rightTime", &This::rightTime, "Right side time (exclusive)")

        // We need to specify a return_value_policy for VtValues, since
        // the default policy of add_property is return_internal_reference
        // but VtValues don't provide an lvalue.  Instead, we just copy
        // them on read.
        .add_property(
            "leftValue",
            make_getter(
                &This::leftValue, return_value_policy<return_by_value>()),
            "Value at left or, for blur, min value")
        .add_property(
            "rightValue",
            make_getter(
                &This::rightValue, return_value_policy<return_by_value>()),
            "Value at right or, for blur, max value")
        ;
}


void wrapTypes()
{
    TfPyWrapEnum<TsExtrapolationType>();

    TfPyContainerConversions::tuple_mapping_pair<
        std::pair<TsExtrapolationType, TsExtrapolationType> >();

    TfPyContainerConversions::from_python_sequence<
        std::set<double> , TfPyContainerConversions::set_policy >();

    wrapValueSample();

    Ts_AnnotatedBoolResult::Wrap<Ts_AnnotatedBoolResult>(
        "_AnnotatedBoolResult", "reasonWhyNot");
}
