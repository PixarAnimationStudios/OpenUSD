//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/glf/simpleMaterial.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void wrapSimpleMaterial(nanobind::module_ &m)
{
    typedef GlfSimpleMaterial This;

    nb::class_<This>(m, "SimpleMaterial")
        .def(nb::init<>())
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
        .def_prop_rw("emission",
                     &This::GetEmission,
                     &This::SetEmission,
                     nb::rv_policy::copy)
        .def_prop_rw("shininess",
                     &This::GetShininess,
                     &This::SetShininess,
                     nb::rv_policy::copy)
        ;
}
