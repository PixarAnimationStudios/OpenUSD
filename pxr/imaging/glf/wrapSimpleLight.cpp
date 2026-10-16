//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/glf/simpleLight.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void wrapSimpleLight(nanobind::module_ &m)
{
    typedef GlfSimpleLight This;

    nb::class_<This>(m, "SimpleLight")
        .def(nb::init<>())
        .def_prop_rw("transform",
                     &This::GetTransform,
                     &This::SetTransform,
                     nb::rv_policy::copy)
        .def_prop_rw("ambient",
                     &This::GetAmbient,
                     &This::SetAmbient,
                     nb::rv_policy::copy)
        .def_prop_rw("diffuse",
                     &This::GetDiffuse,
                     &This::SetDiffuse,
                     nb::rv_policy::copy)
        .def_prop_rw("specular",
                     &This::GetSpecular,
                     &This::SetSpecular,
                     nb::rv_policy::copy)
        .def_prop_rw("position",
                     &This::GetPosition,
                     &This::SetPosition,
                     nb::rv_policy::copy)
        .def_prop_rw("spotDirection",
                     &This::GetSpotDirection,
                     &This::SetSpotDirection,
                     nb::rv_policy::copy)
        .def_prop_rw("spotCutoff",
                     &This::GetSpotCutoff,
                     &This::SetSpotCutoff,
                     nb::rv_policy::copy)
        .def_prop_rw("spotFalloff",
                     &This::GetSpotFalloff,
                     &This::SetSpotFalloff,
                     nb::rv_policy::copy)
        .def_prop_rw("attenuation",
                     &This::GetAttenuation,
                     &This::SetAttenuation,
                     nb::rv_policy::copy)
        .def_prop_rw("shadowMatrices",
                     &This::GetShadowMatrices,
                     &This::SetShadowMatrices,
                     nb::rv_policy::copy)
        .def_prop_rw("shadowResolution",
                     &This::GetShadowResolution,
                     &This::SetShadowResolution,
                     nb::rv_policy::copy)
        .def_prop_rw("shadowBias",
                     &This::GetShadowBias,
                     &This::SetShadowBias,
                     nb::rv_policy::copy)
        .def_prop_rw("shadowBlur",
                     &This::GetShadowBlur,
                     &This::SetShadowBlur,
                     nb::rv_policy::copy)
        .def_prop_rw("shadowIndexStart",
                     &This::GetShadowIndexStart,
                     &This::SetShadowIndexStart,
                     nb::rv_policy::copy)
        .def_prop_rw("shadowIndexEnd",
                     &This::GetShadowIndexEnd,
                     &This::SetShadowIndexEnd,
                     nb::rv_policy::copy)
        .def_prop_rw("hasShadow",
                     &This::HasShadow,
                     &This::SetHasShadow,
                     nb::rv_policy::copy)
        .def_prop_rw("isCameraSpaceLight",
                     &This::IsCameraSpaceLight,
                     &This::SetIsCameraSpaceLight,
                     nb::rv_policy::copy)
        .def_prop_rw("id",
                     &This::GetID,
                     &This::SetID,
                     nb::rv_policy::copy)
        .def_prop_rw("isDomeLight",
                     &This::IsDomeLight,
                     &This::SetIsDomeLight,
                     nb::rv_policy::copy)
        ;
}
