//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file Sdf/wrapNotice.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/notice.h"
#include "pxr/base/tf/pyNoticeWrapper.h"
#include "pxr/base/tf/pyResultConversions.h"
#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

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

} // anonymous namespace 

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
