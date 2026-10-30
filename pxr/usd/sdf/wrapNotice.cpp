//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file Sdf/wrapNotice.cpp

#include "pxr/pxr.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/notice.h"
#include "pxr/base/tf/pyNoticeWrapper.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

void
wrapNotice(nanobind::module_ &m)
{
    namespace nb = nanobind;
    nb::class_<SdfNotice> s(m, "Notice");

    TfNbNoticeWrapper<SdfNotice::Base, TfNotice>::Wrap(s);

    TfNbNoticeWrapper<
        SdfNotice::LayersDidChange, SdfNotice::Base>::Wrap(s)
        .def("GetLayers", &SdfNotice::LayersDidChange::GetLayers)
        .def("GetSerialNumber",
             &SdfNotice::LayersDidChange::GetSerialNumber)
        ;

    TfNbNoticeWrapper<
        SdfNotice::LayersDidChangeSentPerLayer, SdfNotice::Base>::Wrap(s)
        .def("GetLayers", &SdfNotice::LayersDidChangeSentPerLayer::GetLayers)
        .def("GetSerialNumber",
             &SdfNotice::LayersDidChangeSentPerLayer::GetSerialNumber)
        ;

    TfNbNoticeWrapper<
        SdfNotice::LayerDidReplaceContent, SdfNotice::Base>::Wrap(s);

    TfNbNoticeWrapper<
        SdfNotice::LayerDidReloadContent,
        SdfNotice::LayerDidReplaceContent>::Wrap(s);

    TfNbNoticeWrapper<
        SdfNotice::LayerInfoDidChange, SdfNotice::Base>::Wrap(s)
        .def("key", &SdfNotice::LayerInfoDidChange::key,
             nb::rv_policy::copy)
        ;

    TfNbNoticeWrapper<
        SdfNotice::LayerIdentifierDidChange, 
        SdfNotice::Base>::Wrap(s)
        .def_prop_ro("oldIdentifier",
                     &SdfNotice::LayerIdentifierDidChange::GetOldIdentifier,
                     nb::rv_policy::copy)
        .def_prop_ro("newIdentifier",
                     &SdfNotice::LayerIdentifierDidChange::GetNewIdentifier,
                     nb::rv_policy::copy)
        ;

    TfNbNoticeWrapper<
        SdfNotice::LayerDirtinessChanged,
        SdfNotice::Base>::Wrap(s)
        ;

    TfNbNoticeWrapper<
        SdfNotice::LayerMutenessChanged,
        SdfNotice::Base>::Wrap(s)
        .def_prop_ro("layerPath",
                     &SdfNotice::LayerMutenessChanged::GetLayerPath,
                     nb::rv_policy::copy)
        .def_prop_ro("wasMuted", &SdfNotice::LayerMutenessChanged::WasMuted)
        ;
}
