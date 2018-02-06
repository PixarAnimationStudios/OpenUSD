//
// Copyright 2016 Pixar
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

#include "pxr/usd/usd/stagePopulationMask.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyResultConversions.h"

using std::string;

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static std::string _Str(UsdStagePopulationMask const &self)
{
    return boost::lexical_cast<std::string>(self);
}

static string __repr__(UsdStagePopulationMask const &self)
{
    return TF_PY_REPR_PREFIX + "StagePopulationMask(" +
        TfPyRepr(self.GetPaths()) + ")";
}

static std::pair<bool, std::vector<TfToken>> 
_GetIncludedChildNames(UsdStagePopulationMask const &self, SdfPath const& path)
{
    TfTokenVector names;
    const bool result = self.GetIncludedChildNames(path, &names);
    return std::make_pair(result, std::move(names));
}

} // anonymous namespace 

void wrapUsdStagePopulationMask()
{
    class_<UsdStagePopulationMask>("StagePopulationMask")
        .def(init<std::vector<SdfPath>>())
        
        .def("All", &UsdStagePopulationMask::All)
        .staticmethod("All")
        
        .def("Union", &UsdStagePopulationMask::Union)
        .staticmethod("Union")

        .def("GetUnion",
             (UsdStagePopulationMask (UsdStagePopulationMask::*)(
                 UsdStagePopulationMask const &) const)
             &UsdStagePopulationMask::GetUnion,
             arg("other"))
        .def("GetUnion",
             (UsdStagePopulationMask (UsdStagePopulationMask::*)(
                 SdfPath const &) const)
             &UsdStagePopulationMask::GetUnion,
             arg("path"))

        .def("Intersection", &UsdStagePopulationMask::Intersection)
        .staticmethod("Intersection")

        .def("GetIntersection",
             (UsdStagePopulationMask (UsdStagePopulationMask::*)(
                 UsdStagePopulationMask const &) const)
             &UsdStagePopulationMask::GetIntersection,
             arg("other"))

        .def("Includes",
             (bool (UsdStagePopulationMask::*)(
                 UsdStagePopulationMask const &) const)
             &UsdStagePopulationMask::Includes,
             arg("other"))
        .def("Includes",
             (bool (UsdStagePopulationMask::*)(SdfPath const &) const)
             &UsdStagePopulationMask::Includes,
             arg("path"))

        .def("IncludesSubtree", &UsdStagePopulationMask::IncludesSubtree,
             arg("path"))

        .def("IsEmpty", &UsdStagePopulationMask::IsEmpty)

        .def("Add",
             (UsdStagePopulationMask &(UsdStagePopulationMask::*)(
                 UsdStagePopulationMask const &other))
             &UsdStagePopulationMask::Add,
             return_self<>())
        
        .def("Add",
             (UsdStagePopulationMask &(UsdStagePopulationMask::*)(
                 SdfPath const &path))
             &UsdStagePopulationMask::Add,
             return_self<>())

        .def("GetIncludedChildNames", &_GetIncludedChildNames,
             arg("path"),
             return_value_policy<TfPyPairToTuple>())

        .def("GetPaths", &UsdStagePopulationMask::GetPaths)

        .def(self == self)
        .def(self != self)

//        .def(str(self))
        .def("__str__", _Str)
        .def("__repr__", __repr__)

        ;
}
