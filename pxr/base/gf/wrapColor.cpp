///
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/color.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/colorSpace.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

std::string __repr__(GfColor const &self)
{
    return TF_PY_REPR_PREFIX + 
        TfStringPrintf("Color(%s, %s)", 
                             TfPyRepr(self.GetRGB()).c_str(),
			     TfPyRepr(self.GetColorSpace()).c_str());
}

}

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>

void wrapColor(nanobind::module_ &m)
{

    nanobind::class_<GfColor>(m, "Color", 
                    "A class representing a color, supporting different "
                    "color spaces.")
        .def(nanobind::init<>()) 
        .def(nanobind::init<const GfColor&>()) 
        .def(nanobind::init<const GfColorSpace&>()) 
        .def(nanobind::init<const GfVec3f&, const GfColorSpace&>()) 
        .def(nanobind::init<const GfColor&, const GfColorSpace&>()) 
        .def("__repr__", &__repr__) 
        .def("SetFromPlanckianLocus", &GfColor::SetFromPlanckianLocus, 
             nanobind::arg("kelvin"), nanobind::arg("luminance"))
        .def("GetRGB", &GfColor::GetRGB) 
        .def("GetColorSpace", &GfColor::GetColorSpace) 
        .def(nanobind::self == nanobind::self) 
        .def(nanobind::self != nanobind::self) 
        ;

    m.def("IsClose", 
        (bool (*)(const GfColor &v1, const GfColor &v2, double)) GfIsClose,
        nanobind::arg("v1"), nanobind::arg("v2"), nanobind::arg("tolerance"));
}

