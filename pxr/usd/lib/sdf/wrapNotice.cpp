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
///
/// \file Sdf/wrapNotice.cpp
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/notice.h"
#include "pxr/base/tf/pyNoticeWrapper.h"
#include "pxr/base/tf/pyResultConversions.h"
#include <boost/python.hpp>

using namespace boost::python;

TF_INSTANTIATE_NOTICE_WRAPPER(
    SdfNotice::Base, TfNotice);
TF_INSTANTIATE_NOTICE_WRAPPER(
    SdfNotice::LayersDidChange, SdfNotice::Base);
TF_INSTANTIATE_NOTICE_WRAPPER(
    SdfNotice::LayersDidChangeSentPerLayer, SdfNotice::Base);
TF_INSTANTIATE_NOTICE_WRAPPER(
    SdfNotice::LayerDidReplaceContent, SdfNotice::Base);
TF_INSTANTIATE_NOTICE_WRAPPER(
    SdfNotice::LayerDidReloadContent, SdfNotice::LayerDidReplaceContent);
TF_INSTANTIATE_NOTICE_WRAPPER(
    SdfNotice::LayerInfoDidChange, SdfNotice::Base);
TF_INSTANTIATE_NOTICE_WRAPPER(
    SdfNotice::LayerIdentifierDidChange, SdfNotice::Base);
TF_INSTANTIATE_NOTICE_WRAPPER(
    SdfNotice::LayerDirtinessChanged, SdfNotice::Base);
TF_INSTANTIATE_NOTICE_WRAPPER(
    SdfNotice::LayerMutenessChanged, SdfNotice::Base);

void
wrapNotice()
{
    scope s = class_<SdfNotice>("Notice", no_init);

    TfPyNoticeWrapper<SdfNotice::Base, TfNotice>::Wrap();

    TfPyNoticeWrapper<
        SdfNotice::LayersDidChange, SdfNotice::Base>::Wrap()
        .def("GetLayers",
            &SdfNotice::LayersDidChange::GetLayers,
            return_value_policy<TfPySequenceToList>())
        .def("GetSerialNumber",
             &SdfNotice::LayersDidChange::GetSerialNumber)
        ;

    TfPyNoticeWrapper<
        SdfNotice::LayersDidChangeSentPerLayer, SdfNotice::Base>::Wrap()
        .def("GetLayers",
            &SdfNotice::LayersDidChangeSentPerLayer::GetLayers,
            return_value_policy<TfPySequenceToList>())
        .def("GetSerialNumber",
             &SdfNotice::LayersDidChangeSentPerLayer::GetSerialNumber)
        ;

    TfPyNoticeWrapper<
        SdfNotice::LayerDidReplaceContent, SdfNotice::Base>::Wrap();

    TfPyNoticeWrapper<
        SdfNotice::LayerDidReloadContent,
        SdfNotice::LayerDidReplaceContent>::Wrap();

    TfPyNoticeWrapper<
        SdfNotice::LayerInfoDidChange, SdfNotice::Base>::Wrap()
        .def("key", &SdfNotice::LayerInfoDidChange::key,
             return_value_policy<return_by_value>())
        ;

    TfPyNoticeWrapper<
        SdfNotice::LayerIdentifierDidChange, 
        SdfNotice::Base>::Wrap()
        .add_property("oldIdentifier", 
             make_function(
                 &SdfNotice::LayerIdentifierDidChange::GetOldIdentifier,
                 return_value_policy<return_by_value>()))
        .add_property("newIdentifier", 
             make_function(
                 &SdfNotice::LayerIdentifierDidChange::GetNewIdentifier,
                 return_value_policy<return_by_value>()))
        ;

    TfPyNoticeWrapper<
        SdfNotice::LayerDirtinessChanged,
        SdfNotice::Base>::Wrap()
        ;

    TfPyNoticeWrapper<
        SdfNotice::LayerMutenessChanged,
        SdfNotice::Base>::Wrap()
        .add_property("layerPath",
             make_function(
                 &SdfNotice::LayerMutenessChanged::GetLayerPath,
                 return_value_policy<return_by_value>()))
        .add_property("wasMuted", &SdfNotice::LayerMutenessChanged::WasMuted)
        ;
}
