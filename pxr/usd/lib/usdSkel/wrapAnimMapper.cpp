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
#include "pxr/usd/usdSkel/animMapper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/usd/usd/pyConversions.h"

#include <boost/python.hpp>


using namespace boost::python;

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


void wrapUsdSkelAnimMapper()
{
    using This = UsdSkelAnimMapper;

    class_<This,UsdSkelAnimMapperRefPtr>("AnimMapper", no_init)

        .def(init<>())
        
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
