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
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/external/boost/python.hpp"

#include <vector>

using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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

void wrapUsdStageCache()
{
    scope s = class_<UsdStageCache>("StageCache")
        .def(init<const UsdStageCache &>())
        .def("swap", &UsdStageCache::swap)

        .def("GetAllStages", &UsdStageCache::GetAllStages,
             return_value_policy<TfPySequenceToList>()) 
        .def("Size", &UsdStageCache::Size)
        .def("IsEmpty", &UsdStageCache::IsEmpty)

        .def("Find", (UsdStageRefPtr
                      (UsdStageCache::*)(UsdStageCache::Id) const)
             &UsdStageCache::Find, arg("id"))

        .def("FindOneMatching",
             (UsdStageRefPtr (UsdStageCache::*)(
                 const SdfLayerHandle &) const)
             &UsdStageCache::FindOneMatching, (arg("rootLayer")))
        .def("FindOneMatching",
             (UsdStageRefPtr (UsdStageCache::*)(
                 const SdfLayerHandle &, const SdfLayerHandle &) const)
             &UsdStageCache::FindOneMatching, (arg("rootLayer"),
                                                arg("sessionLayer")))
        .def("FindOneMatching",
             (UsdStageRefPtr (UsdStageCache::*)(
                 const SdfLayerHandle &, const ArResolverContext &) const)
             &UsdStageCache::FindOneMatching, (arg("rootLayer"),
                                                arg("pathResolverContext")))
        .def("FindOneMatching",
             (UsdStageRefPtr (UsdStageCache::*)(
                 const SdfLayerHandle &, const SdfLayerHandle &,
                 const ArResolverContext &) const)
             &UsdStageCache::FindOneMatching, (arg("rootLayer"),
                                                arg("sessionLayer"),
                                                arg("pathResolverContext")))

        .def("FindAllMatching",
             (vector<UsdStageRefPtr> (UsdStageCache::*)(
                 const SdfLayerHandle &) const)
             &UsdStageCache::FindAllMatching, (arg("rootLayer")),
             return_value_policy<TfPySequenceToList>())
        .def("FindAllMatching",
             (vector<UsdStageRefPtr> (UsdStageCache::*)(
                 const SdfLayerHandle &, const SdfLayerHandle &) const)
             &UsdStageCache::FindAllMatching, (arg("rootLayer"),
                                                arg("sessionLayer")),
             return_value_policy<TfPySequenceToList>())
        .def("FindAllMatching",
             (vector<UsdStageRefPtr> (UsdStageCache::*)(
                 const SdfLayerHandle &, const ArResolverContext &) const)
             &UsdStageCache::FindAllMatching, (arg("rootLayer"),
                                                arg("pathResolverContext")),
             return_value_policy<TfPySequenceToList>())
        .def("FindAllMatching",
             (vector<UsdStageRefPtr> (UsdStageCache::*)(
                 const SdfLayerHandle &, const SdfLayerHandle &,
                 const ArResolverContext &) const)
             &UsdStageCache::FindAllMatching, (arg("rootLayer"),
                                                arg("sessionLayer"),
                                                arg("pathResolverContext")),
             return_value_policy<TfPySequenceToList>())

        .def("Contains", Contains, arg("stage"))
        .def("Contains", (bool (UsdStageCache::*)(UsdStageCache::Id) const)
             &UsdStageCache::Contains, arg("id"))
        .def("GetId", GetId, arg("stage"))
        .def("Insert", &UsdStageCache::Insert, arg("stage"))

        .def("Erase", (bool (UsdStageCache::*)(UsdStageCache::Id))
             &UsdStageCache::Erase, arg("id"))
        .def("Erase", (bool (UsdStageCache::*)(const UsdStageRefPtr &))
             &UsdStageCache::Erase, arg("stage"))

        .def("EraseAll", (size_t (UsdStageCache::*)(const SdfLayerHandle &))
             &UsdStageCache::EraseAll, arg("rootLayer"))
        .def("EraseAll", (size_t (UsdStageCache::*)(const SdfLayerHandle &,
                                                     const SdfLayerHandle &))
             &UsdStageCache::EraseAll,
             (arg("rootLayer"), arg("sessionLayer")))
        .def("EraseAll", (size_t (UsdStageCache::*)(
                              const SdfLayerHandle &,
                              const SdfLayerHandle &,
                              const ArResolverContext &))
             &UsdStageCache::EraseAll,
             (arg("rootLayer"),
              arg("sessionLayer"),
              arg("pathResolverContext")))

        .def("Clear", &UsdStageCache::Clear)

        .def("SetDebugName", &UsdStageCache::SetDebugName)
        .def("GetDebugName", &UsdStageCache::GetDebugName)
        ;

    class_<UsdStageCache::Id>("Id")
        .def("FromLongInt", &UsdStageCache::Id::FromLongInt, arg("val"))
            .staticmethod("FromLongInt")
        .def("FromString", &UsdStageCache::Id::FromString, arg("s"))
            .staticmethod("FromString")
        .def("ToLongInt", &UsdStageCache::Id::ToLongInt)
        .def("ToString", &UsdStageCache::Id::ToString)
        .def("IsValid", &UsdStageCache::Id::IsValid)
        .def(!self)
        .def(self < self)
        .def(self <= self)
        .def(self > self)
        .def(self >= self)
        .def(self == self)
        .def(self != self)
        .def("__hash__", __hash__)
        ;
}
