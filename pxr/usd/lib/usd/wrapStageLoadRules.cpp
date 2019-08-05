//
// Copyright 2019 Pixar
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
#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/return_arg.hpp>
#include <boost/python/scope.hpp>

#include "pxr/usd/usd/stageLoadRules.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"

using std::string;

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static std::string __str__(UsdStageLoadRules const &self)
{
    return boost::lexical_cast<std::string>(self);
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
