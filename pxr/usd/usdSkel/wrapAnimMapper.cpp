//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/animMapper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/usd/usd/pyConversions.h"

#include <nanobind/nanobind.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

TfPyObjWrapper
_Remap(const UsdSkelAnimMapper& self,
       const VtValue& source,
       const VtValue& target,
       int elementSize,
       const VtValue& defaultValue)
{
    VtValue output(target);
    self.Remap(source, &output, elementSize, defaultValue);
    return UsdVtValueToPython(output);
}

template <typename Matrix4>
VtArray<Matrix4>
_RemapTransforms(const UsdSkelAnimMapper& self,
                 const VtArray<Matrix4>& source,
                 const VtArray<Matrix4>& target,
                 int elementSize)
{
    VtArray<Matrix4> output(target);
    self.RemapTransforms(source, &output, elementSize);
    return output;
}

} // namespace

void wrapUsdSkelAnimMapper(nb::module_ &m)
{
    using This = UsdSkelAnimMapper;

    nb::class_<This>(m, "AnimMapper")

        .def(nb::init<>())

        .def(nb::init<size_t>())
        
        .def(nb::init<VtTokenArray, VtTokenArray>(),
             nb::arg("sourceOrder"), nb::arg("targetOrder"))
        .def("Remap", &_Remap,
             nb::arg("source"),
             nb::arg("target").none()=VtValue(),
             nb::arg("elementSize")=1,
             nb::arg("defaultValue").none()=VtValue())

        .def("RemapTransforms", &_RemapTransforms<GfMatrix4d>,
             nb::arg("source"),
             nb::arg("target"),
             nb::arg("elementSize")=1)

        .def("RemapTransforms", &_RemapTransforms<GfMatrix4f>,
             nb::arg("source"),
             nb::arg("target"),
             nb::arg("elementSize")=1)

        .def("IsIdentity", &This::IsIdentity)

        .def("IsSparse", &This::IsSparse)

        .def("IsNull", &This::IsNull)

        .def("__len__", &This::size)
        ;

}
