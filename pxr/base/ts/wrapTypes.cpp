//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
