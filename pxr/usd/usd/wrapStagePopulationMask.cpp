//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/return_arg.hpp"

#include "pxr/usd/usd/stagePopulationMask.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/stringUtils.h"

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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

        .def("__str__", __str__)
        .def("__repr__", __repr__)
        .def("__hash__", __hash__)

        ;
}
