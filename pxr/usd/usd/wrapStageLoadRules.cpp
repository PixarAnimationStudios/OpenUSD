//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/stageLoadRules.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/set.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;
using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static std::string __str__(UsdStageLoadRules const &self)
{
    return TfStringify(self);
}

static string __repr__(UsdStageLoadRules const &self)
{
    return TF_PY_REPR_PREFIX + "StageLoadRules(" +
        TfPyRepr(self.GetRules()) + ")";
}

static size_t __hash__(UsdStageLoadRules const &self)
{
    return hash_value(self);
}

} // anonymous namespace 

void wrapUsdStageLoadRules(nb::module_ &m)
{

    TfPyContainerConversions::tuple_mapping_pair<
        std::pair<SdfPath, UsdStageLoadRules::Rule>>();

    nb::class_<UsdStageLoadRules> thisClass(m, "StageLoadRules");
    TfNbWrapEnum<UsdStageLoadRules::Rule>(thisClass);
    thisClass
        .def(nb::init<>())
        .def(nb::init<UsdStageLoadRules>())

        .def_static("LoadAll", &UsdStageLoadRules::LoadAll)
        .def_static("LoadNone", &UsdStageLoadRules::LoadNone)

        .def("LoadWithDescendants",
             &UsdStageLoadRules::LoadWithDescendants, nb::arg("path"))

        .def("LoadWithoutDescendants",
             &UsdStageLoadRules::LoadWithoutDescendants, nb::arg("path"))

        .def("Unload",
             &UsdStageLoadRules::Unload, nb::arg("path"))

        .def("LoadAndUnload",
             &UsdStageLoadRules::LoadAndUnload,
             nb::arg("loadSet"), nb::arg("unloadSet"), nb::arg("policy"))
        
        .def("AddRule", &UsdStageLoadRules::AddRule,
             nb::arg("path"), nb::arg("rule"))

        .def("SetRules",
             (void (UsdStageLoadRules::*)(
                 std::vector<std::pair<
                     SdfPath, UsdStageLoadRules::Rule>> const &))
             &UsdStageLoadRules::SetRules, nb::arg("rules"))

        .def("Minimize", &UsdStageLoadRules::Minimize)

        .def("IsLoaded", &UsdStageLoadRules::IsLoaded, nb::arg("path"))

        .def("IsLoadedWithAllDescendants",
             &UsdStageLoadRules::IsLoadedWithAllDescendants, nb::arg("path"))

        .def("IsLoadedWithNoDescendants",
             &UsdStageLoadRules::IsLoadedWithNoDescendants, nb::arg("path"))

        .def("GetEffectiveRuleForPath",
             &UsdStageLoadRules::GetEffectiveRuleForPath, nb::arg("path"))

        .def("GetRules", &UsdStageLoadRules::GetRules)

        .def("swap", &UsdStageLoadRules::swap, nb::arg("other"))

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)

        .def("__str__", __str__)
        .def("__repr__", __repr__)
        .def("__hash__", __hash__)
        
        ;

    TfPyContainerConversions::from_python_sequence<
        std::vector<std::pair<SdfPath, UsdStageLoadRules::Rule> >,
        TfPyContainerConversions::
        variable_capacity_all_items_convertible_policy>();
}
