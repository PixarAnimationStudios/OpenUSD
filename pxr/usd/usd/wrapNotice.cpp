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

