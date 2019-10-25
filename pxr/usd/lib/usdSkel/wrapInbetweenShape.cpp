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
#include "pxr/usd/usdSkel/inbetweenShape.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>
#include <boost/python/extract.hpp>


using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE


namespace {


VtVec3fArray
_GetOffsets(const UsdSkelInbetweenShape& self)
{
    VtVec3fArray points;
    self.GetOffsets(&points);
    return points;
}


VtVec3fArray
_GetNormalOffsets(const UsdSkelInbetweenShape& self)
{
    VtVec3fArray points;
    self.GetNormalOffsets(&points);
    return points;
}


object
_GetWeight(const UsdSkelInbetweenShape& self)
{
    float w = 0;
    return self.GetWeight(&w) ? object(w) : object();
}


bool
_SetOffsets(const UsdSkelInbetweenShape& self, const object& val)
{
    const VtValue vtVal =
        UsdPythonToSdfType(val, SdfValueTypeNames->Vector3fArray);
    return vtVal.IsHolding<VtVec3fArray>() ?
        self.SetOffsets(vtVal.UncheckedGet<VtVec3fArray>()) : false;
}


bool
_SetNormalOffsets(const UsdSkelInbetweenShape& self, const object& val)
{
    const VtValue vtVal =
        UsdPythonToSdfType(val, SdfValueTypeNames->Vector3fArray);
    return vtVal.IsHolding<VtVec3fArray>() ?
        self.SetNormalOffsets(vtVal.UncheckedGet<VtVec3fArray>()) : false;
}


UsdAttribute
_CreateNormalOffsetsAttr(const UsdSkelInbetweenShape& self,
                         const object& defaultValue)
{
    return self.CreateNormalOffsetsAttr(
        UsdPythonToSdfType(defaultValue, SdfValueTypeNames->Vector3fArray));
}            


} // namespace


void wrapUsdSkelInbetweenShape()
{
    using This = UsdSkelInbetweenShape;

    class_<This>("InbetweenShape")

        .def(init<UsdAttribute>(arg("attr")))
        .def(!self)
        .def(self == self)

        .def("GetWeight", &_GetWeight)
        .def("SetWeight", &This::SetWeight, arg("weight"))
        .def("HasAuthoredWeight", &This::HasAuthoredWeight)

        .def("GetOffsets", &_GetOffsets)
        .def("SetOffsets", &_SetOffsets, arg("offsets"))

        .def("GetNormalOffsetsAttr", &This::GetNormalOffsetsAttr)
        .def("CreateNormalOffsetsAttr", &_CreateNormalOffsetsAttr)

        .def("GetNormalOffsets", &_GetNormalOffsets)
        .def("SetNormalOffsets", &_SetNormalOffsets, arg("offsets"))

        .def("IsInbetween", &This::IsInbetween, arg("attr"))
        .staticmethod("IsInbetween")

        .def("GetAttr", &This::GetAttr,
             return_value_policy<return_by_value>())
        .def("IsDefined", &This::IsDefined)
        ;
}            
