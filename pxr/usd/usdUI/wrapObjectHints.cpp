//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdUI/objectHints.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyStaticTokens.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/operators.hpp"

#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapUsdUIObjectHints()
{
    TF_PY_WRAP_PUBLIC_TOKENS(
        "HintKeys", UsdUIHintKeys, USDUI_HINT_KEYS);

    using This = UsdUIObjectHints;

    class_<This> clsObj("ObjectHints");
    clsObj
        .def(init<UsdObject>(arg("obj")))

        .def(self == self)
        .def(self != self)
        .def(!self)

        .def("GetObject", &This::GetObject,
             return_value_policy<return_by_value>())

        .def("GetDisplayName", &This::GetDisplayName)
        .def("SetDisplayName", &This::SetDisplayName,
             arg("name"))

        .def("GetHidden", &This::GetHidden)
        .def("SetHidden", &This::SetHidden,
             arg("hidden"))
        ;

    TfPyRegisterStlSequencesFromPython<This>();
    to_python_converter<
        std::vector<This>,
        TfPySequenceToPython<std::vector<This>>>();
}
