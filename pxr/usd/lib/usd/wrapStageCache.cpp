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
#include "pxr/usd/usd/stageCache.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/ar/resolverContext.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python.hpp>

#include <vector>

using std::vector;

using namespace boost::python;

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
