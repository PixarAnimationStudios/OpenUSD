//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/return_arg.hpp"
#include "pxr/external/boost/python/scope.hpp"

#include "pxr/usd/usd/stageLoadRules.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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

void wrapUsdStageLoadRules()
{

    TfPyContainerConversions::tuple_mapping_pair<
        std::pair<SdfPath, UsdStageLoadRules::Rule>>();

    class_<UsdStageLoadRules> thisClass("StageLoadRules");
    scope s = thisClass;
    TfPyWrapEnum<UsdStageLoadRules::Rule>();
    thisClass
        .def(init<UsdStageLoadRules>())

        .def("LoadAll", &UsdStageLoadRules::LoadAll).staticmethod("LoadAll")
        .def("LoadNone", &UsdStageLoadRules::LoadNone).staticmethod("LoadNone")

        .def("LoadWithDescendants",
             &UsdStageLoadRules::LoadWithDescendants, arg("path"))

        .def("LoadWithoutDescendants",
             &UsdStageLoadRules::LoadWithoutDescendants, arg("path"))

        .def("Unload",
             &UsdStageLoadRules::Unload, arg("path"))

        .def("LoadAndUnload",
             &UsdStageLoadRules::LoadAndUnload,
             (arg("loadSet"), arg("unloadSet"), arg("policy")))
        
        .def("AddRule", &UsdStageLoadRules::AddRule,
             (arg("path"), arg("rule")))

        .def("SetRules",
             (void (UsdStageLoadRules::*)(
                 std::vector<std::pair<
                     SdfPath, UsdStageLoadRules::Rule>> const &))
             &UsdStageLoadRules::SetRules, arg("rules"))

        .def("Minimize", &UsdStageLoadRules::Minimize)

        .def("IsLoaded", &UsdStageLoadRules::IsLoaded, arg("path"))

        .def("IsLoadedWithAllDescendants",
             &UsdStageLoadRules::IsLoadedWithAllDescendants, arg("path"))

        .def("IsLoadedWithNoDescendants",
             &UsdStageLoadRules::IsLoadedWithNoDescendants, arg("path"))

        .def("GetEffectiveRuleForPath",
             &UsdStageLoadRules::GetEffectiveRuleForPath, arg("path"))

        .def("GetRules", &UsdStageLoadRules::GetRules,
             return_value_policy<TfPySequenceToList>())

        .def("swap", &UsdStageLoadRules::swap, arg("other"))

        .def(self == self)
        .def(self != self)

        .def("__str__", __str__)
        .def("__repr__", __repr__)
        .def("__hash__", __hash__)
        
        ;

    TfPyContainerConversions::from_python_sequence<
        std::vector<std::pair<SdfPath, UsdStageLoadRules::Rule> >,
        TfPyContainerConversions::
        variable_capacity_all_items_convertible_policy>();
}
