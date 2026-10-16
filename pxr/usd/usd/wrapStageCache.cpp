//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/stageCache.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/ar/resolverContext.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/usd/ar/pyResolverContext.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <vector>

namespace nb = nanobind;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static size_t __hash__(UsdStageCache::Id id) { return hash_value(id); }

static bool
Contains(const UsdStageCache &self, const UsdStagePtr &stage) {
    return self.Contains(stage);
}

static UsdStageCache::Id
GetId(const UsdStageCache &self, const UsdStagePtr &stage) {
    return self.GetId(stage);
}

} // anonymous namespace 

void wrapUsdStageCache(nb::module_ &m)
{
    nb::class_<UsdStageCache> cls(m, "StageCache", nb::is_weak_referenceable());
    cls
        .def(nb::init<>())
        .def(nb::init<const UsdStageCache &>())
        .def("swap", &UsdStageCache::swap)

        .def("GetAllStages", &UsdStageCache::GetAllStages) 
        .def("Size", &UsdStageCache::Size)
        .def("IsEmpty", &UsdStageCache::IsEmpty)

        .def("Find", (UsdStageRefPtr
                      (UsdStageCache::*)(UsdStageCache::Id) const)
             &UsdStageCache::Find, nb::arg("id"))

        .def("FindOneMatching",
             (UsdStageRefPtr (UsdStageCache::*)(
                 const SdfLayerHandle &) const)
             &UsdStageCache::FindOneMatching, nb::arg("rootLayer"))
        .def("FindOneMatching",
             (UsdStageRefPtr (UsdStageCache::*)(
                 const SdfLayerHandle &, const SdfLayerHandle &) const)
             &UsdStageCache::FindOneMatching, nb::arg("rootLayer"),
                                              nb::arg("sessionLayer"))
        .def("FindOneMatching",
             (UsdStageRefPtr (UsdStageCache::*)(
                 const SdfLayerHandle &, const ArResolverContext &) const)
             &UsdStageCache::FindOneMatching, nb::arg("rootLayer"),
                                              nb::arg("pathResolverContext"))
        .def("FindOneMatching",
             (UsdStageRefPtr (UsdStageCache::*)(
                 const SdfLayerHandle &, const SdfLayerHandle &,
                 const ArResolverContext &) const)
             &UsdStageCache::FindOneMatching, nb::arg("rootLayer"),
                                              nb::arg("sessionLayer"),
                                              nb::arg("pathResolverContext"))

        .def("FindAllMatching",
             (vector<UsdStageRefPtr> (UsdStageCache::*)(
                 const SdfLayerHandle &) const)
             &UsdStageCache::FindAllMatching, nb::arg("rootLayer"))
        .def("FindAllMatching",
             (vector<UsdStageRefPtr> (UsdStageCache::*)(
                 const SdfLayerHandle &, const SdfLayerHandle &) const)
             &UsdStageCache::FindAllMatching, nb::arg("rootLayer"),
                                              nb::arg("sessionLayer"))
        .def("FindAllMatching",
             (vector<UsdStageRefPtr> (UsdStageCache::*)(
                 const SdfLayerHandle &, const ArResolverContext &) const)
             &UsdStageCache::FindAllMatching, nb::arg("rootLayer"),
                                              nb::arg("pathResolverContext"))
        .def("FindAllMatching",
             (vector<UsdStageRefPtr> (UsdStageCache::*)(
                 const SdfLayerHandle &, const SdfLayerHandle &,
                 const ArResolverContext &) const)
             &UsdStageCache::FindAllMatching, nb::arg("rootLayer"),
                                              nb::arg("sessionLayer"),
                                              nb::arg("pathResolverContext"))

        .def("Contains", Contains, nb::arg("stage"))
        .def("Contains", (bool (UsdStageCache::*)(UsdStageCache::Id) const)
             &UsdStageCache::Contains, nb::arg("id"))
        .def("GetId", GetId, nb::arg("stage"))
        .def("Insert", &UsdStageCache::Insert, nb::arg("stage"))

        .def("Erase", (bool (UsdStageCache::*)(UsdStageCache::Id))
             &UsdStageCache::Erase, nb::arg("id"))
        .def("Erase", (bool (UsdStageCache::*)(const UsdStageRefPtr &))
             &UsdStageCache::Erase, nb::arg("stage"))

        .def("EraseAll", (size_t (UsdStageCache::*)(const SdfLayerHandle &))
             &UsdStageCache::EraseAll, nb::arg("rootLayer"))
        .def("EraseAll", (size_t (UsdStageCache::*)(const SdfLayerHandle &,
                                                     const SdfLayerHandle &))
             &UsdStageCache::EraseAll,
             nb::arg("rootLayer"), nb::arg("sessionLayer"))
        .def("EraseAll", (size_t (UsdStageCache::*)(
                              const SdfLayerHandle &,
                              const SdfLayerHandle &,
                              const ArResolverContext &))
             &UsdStageCache::EraseAll,
             nb::arg("rootLayer"),
             nb::arg("sessionLayer"),
             nb::arg("pathResolverContext"))

        .def("Clear", &UsdStageCache::Clear)

        .def("SetDebugName", &UsdStageCache::SetDebugName)
        .def("GetDebugName", &UsdStageCache::GetDebugName)
        ;

    nb::class_<UsdStageCache::Id>(cls, "Id")
        .def(nb::init<>())
        .def_static("FromLongInt", &UsdStageCache::Id::FromLongInt, nb::arg("val"))
        .def_static("FromString", &UsdStageCache::Id::FromString, nb::arg("s"))
        .def("ToLongInt", &UsdStageCache::Id::ToLongInt)
        .def("ToString", &UsdStageCache::Id::ToString)
        .def("IsValid", &UsdStageCache::Id::IsValid)
        .def("__bool__", [](const UsdStageCache::Id &self) { return bool(self); })
        .def(nb::self < nb::self)
        .def(nb::self <= nb::self)
        .def(nb::self > nb::self)
        .def(nb::self >= nb::self)
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def("__hash__", __hash__)
        ;

}
