//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "pxr/base/tf/pyFunction.h"

#include "pxr/usd/usdUtils/userProcessingFunc.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void wrapUserProcessingFunc(nanobind::module_ &m)
{
    TfPyFunctionFromPython<UsdUtilsProcessingFunc>();
    
    typedef UsdUtilsDependencyInfo This;

    nb::class_<This>(m, "DependencyInfo")
        // Note: we purposely do not expose the constructor which allows for
        // providing expression variables.  These values are currently only
        // specified by the asset localization system during dependency
        // traversal.
        .def(nb::init<>())
        .def(nb::init<const This&>())
        .def(nb::init<const std::string &>())
        .def(nb::init<const std::string &, const std::vector<std::string>>())
        .def_prop_ro("assetPath", &This::GetAssetPath,
            nb::rv_policy::copy)
        .def_prop_ro("dependencies", [](const This& self) {
            return TfNbSequenceToList(self.GetDependencies());
        })
        .def_prop_ro("rawAssetPath", &This::GetRawAssetPath,
            nb::rv_policy::copy)
        .def_prop_ro("expressionVariables", &This::GetExpressionVariables,
            nb::rv_policy::copy)
    ;
}
