//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/notice.h"
#include "pxr/base/tf/pyNoticeWrapper.h"
#include "pxr/base/tf/pyResultConversions.h"
#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

TF_INSTANTIATE_NOTICE_WRAPPER(UsdNotice::StageNotice, 
                                TfNotice);

TF_INSTANTIATE_NOTICE_WRAPPER(UsdNotice::StageContentsChanged,
                                UsdNotice::StageNotice);
TF_INSTANTIATE_NOTICE_WRAPPER(UsdNotice::ObjectsChanged,
                                UsdNotice::StageNotice);
TF_INSTANTIATE_NOTICE_WRAPPER(UsdNotice::StageEditTargetChanged,
                                UsdNotice::StageNotice);
TF_INSTANTIATE_NOTICE_WRAPPER(UsdNotice::LayerMutingChanged,
                                UsdNotice::StageNotice);

} // anonymous namespace 

void wrapUsdNotice()
{
    scope s = class_<UsdNotice>("Notice", no_init);

    TfPyNoticeWrapper<UsdNotice::StageNotice, TfNotice>::Wrap()
        .def("GetStage", &UsdNotice::StageNotice::GetStage,
             return_value_policy<return_by_value>())
        ;

    TfPyNoticeWrapper<
        UsdNotice::StageContentsChanged, UsdNotice::StageNotice>::Wrap()
        ;

    TfPyNoticeWrapper<
        UsdNotice::ObjectsChanged, UsdNotice::StageNotice>::Wrap()
            .def("AffectedObject", &UsdNotice::ObjectsChanged::AffectedObject)
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
                 &UsdNotice::ObjectsChanged::GetChangedFields,
                 return_value_policy<return_by_value>())
            .def("GetChangedFields", 
                 (TfTokenVector (UsdNotice::ObjectsChanged::*)
                     (const SdfPath&) const)
                 &UsdNotice::ObjectsChanged::GetChangedFields,
                 return_value_policy<return_by_value>())
            .def("HasChangedFields",
                 (bool (UsdNotice::ObjectsChanged::*)(const UsdObject&) const)
                 &UsdNotice::ObjectsChanged::HasChangedFields)
            .def("HasChangedFields",
                 (bool (UsdNotice::ObjectsChanged::*)(const SdfPath&) const)
                 &UsdNotice::ObjectsChanged::HasChangedFields)
        ;

    TfPyNoticeWrapper<
        UsdNotice::StageEditTargetChanged, UsdNotice::StageNotice>::Wrap()
        ;

    TfPyNoticeWrapper<
        UsdNotice::LayerMutingChanged, UsdNotice::StageNotice>::Wrap()
        .def("GetMutedLayers", 
             &UsdNotice::LayerMutingChanged::GetMutedLayers,
             return_value_policy<TfPySequenceToList>())
        .def("GetUnmutedLayers", 
             &UsdNotice::LayerMutingChanged::GetUnmutedLayers,
             return_value_policy<TfPySequenceToList>())
        ;
}

