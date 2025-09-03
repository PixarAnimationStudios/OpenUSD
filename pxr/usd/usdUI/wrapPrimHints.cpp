//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdUI/primHints.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/implicit.hpp"

#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapUsdUIPrimHints()
{
    using This = UsdUIPrimHints;

    class_<This, bases<UsdUIObjectHints>> clsObj("PrimHints");
    clsObj
        .def(init<UsdPrim>(arg("prim")))

        .def("GetPrim", &This::GetPrim,
             return_value_policy<return_by_value>())

        .def("GetDisplayGroupsExpanded",
             &This::GetDisplayGroupsExpanded)
        .def("SetDisplayGroupsExpanded",
             &This::SetDisplayGroupsExpanded,
             arg("expanded"))

        .def("GetDisplayGroupExpanded",
             &This::GetDisplayGroupExpanded)
        .def("SetDisplayGroupExpanded",
             &This::SetDisplayGroupExpanded,
             (arg("group"),
              arg("expanded")))

        .def("GetDisplayGroupsShownIf",
             &This::GetDisplayGroupsShownIf)
        .def("SetDisplayGroupsShownIf",
             &This::SetDisplayGroupsShownIf,
             arg("expanded"))

        .def("GetDisplayGroupShownIf",
             &This::GetDisplayGroupShownIf)
        .def("SetDisplayGroupShownIf",
             &This::SetDisplayGroupShownIf,
             (arg("group"),
              arg("shownIf")))
        ;

    TfPyRegisterStlSequencesFromPython<This>();
    to_python_converter<
        std::vector<This>,
        TfPySequenceToPython<std::vector<This>>>();
}
