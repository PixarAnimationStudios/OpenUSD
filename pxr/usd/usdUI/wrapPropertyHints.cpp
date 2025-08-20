//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdUI/propertyHints.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/implicit.hpp"

#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapUsdUIPropertyHints()
{
    using This = UsdUIPropertyHints;

    class_<This, bases<UsdUIObjectHints>>
        clsObj("PropertyHints");
    clsObj
        .def(init<UsdProperty>(arg("prop")))

        .def("GetProperty", &This::GetProperty,
             return_value_policy<return_by_value>())

        .def("GetDisplayGroup", &This::GetDisplayGroup)
        .def("SetDisplayGroup", &This::SetDisplayGroup,
             arg("group"))

        .def("GetShownIf", &This::GetShownIf)
        .def("SetShownIf", &This::SetShownIf,
             arg("shownIf"))
        ;

    TfPyRegisterStlSequencesFromPython<This>();
    to_python_converter<
        std::vector<This>,
        TfPySequenceToPython<std::vector<This>>>();
}
