//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/animMapper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/usd/usd/pyConversions.h"

#include "pxr/external/boost/python.hpp"


PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;


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


void wrapUsdSkelAnimMapper()
{
    using This = UsdSkelAnimMapper;

    class_<This,UsdSkelAnimMapperRefPtr>("AnimMapper", no_init)

        .def(init<>())

        .def(init<size_t>())
        
        .def(init<VtTokenArray,VtTokenArray>(
                 (arg("sourceOrder"), arg("targetOrder"))))
        .def("Remap", &_Remap,
             (arg("source"),
              arg("target")=VtValue(),
              arg("elementSize")=1,
              arg("defaultValue")=VtValue()))

        .def("RemapTransforms", &_RemapTransforms<GfMatrix4d>,
             (arg("source"),
              arg("target"),
              arg("elementSize")=1))

        .def("RemapTransforms", &_RemapTransforms<GfMatrix4f>,
             (arg("source"),
              arg("target"),
              arg("elementSize")=1))

        .def("IsIdentity", &This::IsIdentity)

        .def("IsSparse", &This::IsSparse)

        .def("IsNull", &This::IsNull)

        .def("__len__", &This::size)
        ;
}            
