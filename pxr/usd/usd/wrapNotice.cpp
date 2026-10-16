//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/notice.h"
#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyNoticeWrapper.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

static nb::tuple _ObjectsChangedGetResyncNotice(
    const UsdNotice::ObjectsChanged &self, const SdfPath &path)
{
    SdfPath associatedPath;
    UsdNotice::ObjectsChanged::PrimResyncType result =
        self.GetPrimResyncType(path, &associatedPath);
    return nb::make_tuple(result, associatedPath);
}

void wrapUsdNotice(nb::module_ &m)
{
    nb::class_<UsdNotice> cls(m, "Notice");
    cls.def("__init__", [](nb::args, nb::kwargs) {
        TfPyThrowRuntimeError("This class cannot be instantiated from Python");
    });

    TfNbNoticeWrapper<UsdNotice::StageNotice, TfNotice>::Wrap(cls)
        .def("GetStage", &UsdNotice::StageNotice::GetStage,
             nb::rv_policy::copy)
        ;

    TfNbNoticeWrapper<
        UsdNotice::StageContentsChanged, UsdNotice::StageNotice>::Wrap(cls)
        ;

    {
        auto s = TfNbNoticeWrapper<
            UsdNotice::ObjectsChanged, UsdNotice::StageNotice>::Wrap(cls);
        s.def("AffectedObject", &UsdNotice::ObjectsChanged::AffectedObject)
            .def("ResyncedObject", &UsdNotice::ObjectsChanged::ResyncedObject)
            .def("ResolvedAssetPathsResynced",
                 &UsdNotice::ObjectsChanged::ResolvedAssetPathsResynced)
            .def("ChangedInfoOnly", &UsdNotice::ObjectsChanged::ChangedInfoOnly)
            .def("GetResyncedPaths",
                +[](const UsdNotice::ObjectsChanged& n) {
                    return SdfPathVector(n.GetResyncedPaths());
                })
            .def("GetChangedInfoOnlyPaths",
                +[](const UsdNotice::ObjectsChanged& n) {
                    return SdfPathVector(n.GetChangedInfoOnlyPaths());
                })
            .def("GetResolvedAssetPathsResyncedPaths", 
                +[](const UsdNotice::ObjectsChanged& n) {
                    return SdfPathVector(
                        n.GetResolvedAssetPathsResyncedPaths());
                })
            .def("GetChangedFields", 
                 (TfTokenVector (UsdNotice::ObjectsChanged::*)
                     (const UsdObject&) const)
                 &UsdNotice::ObjectsChanged::GetChangedFields)
            .def("GetChangedFields", 
                 (TfTokenVector (UsdNotice::ObjectsChanged::*)
                     (const SdfPath&) const)
                 &UsdNotice::ObjectsChanged::GetChangedFields)
            .def("HasChangedFields",
                 (bool (UsdNotice::ObjectsChanged::*)(const UsdObject&) const)
                 &UsdNotice::ObjectsChanged::HasChangedFields)
            .def("HasChangedFields",
                 (bool (UsdNotice::ObjectsChanged::*)(const SdfPath&) const)
                 &UsdNotice::ObjectsChanged::HasChangedFields)
            .def("GetPrimResyncType", &_ObjectsChangedGetResyncNotice)
            .def("GetRenamedProperties",
                &UsdNotice::ObjectsChanged::GetRenamedProperties,
                nb::rv_policy::copy)
            ;
            
        TfNbWrapEnum<UsdNotice::ObjectsChanged::PrimResyncType>(s);
    }
    TfNbNoticeWrapper<
        UsdNotice::StageEditTargetChanged, UsdNotice::StageNotice>::Wrap(cls)
        ;

    TfNbNoticeWrapper<
        UsdNotice::LayerMutingChanged, UsdNotice::StageNotice>::Wrap(cls)
        .def("GetMutedLayers", 
             &UsdNotice::LayerMutingChanged::GetMutedLayers,
             nb::rv_policy::copy)
        .def("GetUnmutedLayers", 
             &UsdNotice::LayerMutingChanged::GetUnmutedLayers,
             nb::rv_policy::copy)
        ;
}

