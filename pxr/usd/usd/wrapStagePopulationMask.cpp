//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/stagePopulationMask.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/stringUtils.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;
using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static std::string __str__(UsdStagePopulationMask const &self)
{
    return TfStringify(self);
}

static string __repr__(UsdStagePopulationMask const &self)
{
    return TF_PY_REPR_PREFIX + "StagePopulationMask(" +
        TfPyRepr(self.GetPaths()) + ")";
}

static size_t __hash__(UsdStagePopulationMask const &self)
{
    return hash_value(self);
}

static std::pair<bool, std::vector<TfToken>> 
_GetIncludedChildNames(UsdStagePopulationMask const &self, SdfPath const& path)
{
    TfTokenVector names;
    const bool result = self.GetIncludedChildNames(path, &names);
    return std::make_pair(result, std::move(names));
}

} // anonymous namespace 

void wrapUsdStagePopulationMask(nb::module_ &m)
{
    nb::class_<UsdStagePopulationMask>(m, "StagePopulationMask")
        .def(nb::init<>())
        .def(nb::init<std::vector<SdfPath>>())
        
        .def_static("All", &UsdStagePopulationMask::All)
        
        .def_static("Union", &UsdStagePopulationMask::Union)

        .def("GetUnion",
             (UsdStagePopulationMask (UsdStagePopulationMask::*)(
                 UsdStagePopulationMask const &) const)
             &UsdStagePopulationMask::GetUnion,
             nb::arg("other"))
        .def("GetUnion",
             (UsdStagePopulationMask (UsdStagePopulationMask::*)(
                 SdfPath const &) const)
             &UsdStagePopulationMask::GetUnion,
             nb::arg("path"))

        .def_static("Intersection", &UsdStagePopulationMask::Intersection)

        .def("GetIntersection",
             (UsdStagePopulationMask (UsdStagePopulationMask::*)(
                 UsdStagePopulationMask const &) const)
             &UsdStagePopulationMask::GetIntersection,
             nb::arg("other"))

        .def("Includes",
             (bool (UsdStagePopulationMask::*)(
                 UsdStagePopulationMask const &) const)
             &UsdStagePopulationMask::Includes,
             nb::arg("other"))
        .def("Includes",
             (bool (UsdStagePopulationMask::*)(SdfPath const &) const)
             &UsdStagePopulationMask::Includes,
             nb::arg("path"))

        .def("IncludesSubtree", &UsdStagePopulationMask::IncludesSubtree,
             nb::arg("path"))

        .def("IsEmpty", &UsdStagePopulationMask::IsEmpty)

        .def("Add",
             [](UsdStagePopulationMask &self, UsdStagePopulationMask const &other) -> UsdStagePopulationMask & {
                 return self.Add(other);
             },
             nb::rv_policy::reference)
        
        .def("Add",
             [](UsdStagePopulationMask &self, SdfPath const &path) -> UsdStagePopulationMask & {
                 return self.Add(path);
             },
             nb::rv_policy::reference)

        .def("GetIncludedChildNames", &_GetIncludedChildNames,
             nb::arg("path"))

        .def("GetPaths", &UsdStagePopulationMask::GetPaths)

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)

        .def("__str__", __str__)
        .def("__repr__", __repr__)
        .def("__hash__", __hash__)

        ;

}
