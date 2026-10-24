//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/color.h"
#include "pxr/base/gf/colorSpace.h"
#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/pair.h>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static std::string __repr__(GfColorSpace const &self)
{
    return TF_PY_REPR_PREFIX + 
        TfStringPrintf("ColorSpace(%s)", TfPyRepr(self.GetName()).c_str());
}

} // anon

nanobind::tuple ConvertPrimariesAndWhitePoint_nb(const GfColorSpace& self) {
    auto result = self.GetPrimariesAndWhitePoint();
    return nanobind::make_tuple(
        std::get<0>(result),
        std::get<1>(result),
        std::get<2>(result),
        std::get<3>(result)
    );
}

void wrapColorSpace(nanobind::module_ &m)
{

    nanobind::class_<GfColorSpace>(m, "ColorSpace")
        .def(nanobind::init<const TfToken&>())
        .def(nanobind::init<const TfToken&, const GfVec2f&, const GfVec2f&, 
                  const GfVec2f&, const GfVec2f&, float, float>(),
             nanobind::arg("name"), nanobind::arg("redChroma"),
             nanobind::arg("greenChroma"), nanobind::arg("blueChroma"),
             nanobind::arg("whitePoint"), nanobind::arg("gamma"), 
             nanobind::arg("linearBias"))
        .def(nanobind::init<const TfToken&, const GfMatrix3f&, float, float>(),
             nanobind::arg("name"), nanobind::arg("rgbToXYZ"),
             nanobind::arg("gamma"), nanobind::arg("linearBias"))
        .def("__repr__", &__repr__)
        .def("GetName", &GfColorSpace::GetName)
        .def("ConvertRGBSpan", &GfColorSpace::ConvertRGBSpan)
        .def("ConvertRGBASpan", &GfColorSpace::ConvertRGBASpan)
        .def("Convert", &GfColorSpace::Convert)
        .def("IsData", &GfColorSpace::IsData)
        .def("GetRGBToXYZ", &GfColorSpace::GetRGBToXYZ)
        .def("GetGamma", &GfColorSpace::GetGamma)
        .def("GetLinearBias", &GfColorSpace::GetLinearBias)
        .def("GetTransferFunctionParams", &GfColorSpace::GetTransferFunctionParams)
        .def("GetPrimariesAndWhitePoint", &ConvertPrimariesAndWhitePoint_nb)
        .def_static("IsValid", &GfColorSpace::IsValid)
        .def(nanobind::self == nanobind::self)
        .def(nanobind::self != nanobind::self);

    TF_NB_WRAP_PUBLIC_TOKENS(m, "ColorSpaceNames", GfColorSpaceNames, 
                             GF_COLORSPACE_NAME_TOKENS);
}
