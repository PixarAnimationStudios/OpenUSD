//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdUI/attributeHints.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/external/boost/python/class.hpp"

#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapUsdUIAttributeHints()
{
    using This = UsdUIAttributeHints;

    class_<This, bases<UsdUIPropertyHints>>
        clsObj("AttributeHints");
    clsObj
        .def(init<UsdAttribute>(arg("attr")))

        .def("GetAttribute",
             &This::GetAttribute,
             return_value_policy<return_by_value>())

        .def("GetValueLabels",
             &This::GetValueLabels)
        .def("SetValueLabels",
             &This::SetValueLabels,
             arg("labels"))

        .def("GetValueLabelsOrder",
             &This::GetValueLabelsOrder)
        .def("SetValueLabelsOrder",
             &This::SetValueLabelsOrder,
             arg("order"))

        .def("ApplyValueLabel",
             &This::ApplyValueLabel,
             arg("label"))
        ;

    TfPyRegisterStlSequencesFromPython<This>();
    to_python_converter<
        std::vector<This>,
        TfPySequenceToPython<std::vector<This>>>();
}
