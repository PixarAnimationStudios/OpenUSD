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
#include "pxr/usd/usdGeom/xformCommonAPI.h"

#include "pxr/base/tf/pyEnum.h"
#include <boost/python.hpp>

using namespace boost::python;

static tuple 
_GetXformVectors(
    UsdGeomXformCommonAPI self, 
    const UsdTimeCode &time)
{
    GfVec3d translation;
    GfVec3f rotation, scale, pivot;
    UsdGeomXformCommonAPI::RotationOrder rotationOrder;

    bool result = self.GetXformVectors(&translation, &rotation, &scale, 
                                       &pivot, &rotationOrder, time);

    return result ? make_tuple(translation, rotation, scale, pivot, rotationOrder)
                  : tuple();
}

static tuple 
_GetXformVectorsByAccumulation(
    UsdGeomXformCommonAPI self, 
    const UsdTimeCode &time)
{
    GfVec3d translation;
    GfVec3f rotation, scale, pivot;
    UsdGeomXformCommonAPI::RotationOrder rotationOrder;

    bool result = self.GetXformVectorsByAccumulation(&translation, &rotation, &scale, 
                                                     &pivot, &rotationOrder, time);

    return result ? make_tuple(translation, rotation, scale, pivot, rotationOrder)
                  : tuple();
}


void wrapUsdGeomXformCommonAPI()
{
    typedef UsdGeomXformCommonAPI This;

    class_<This> cls("XformCommonAPI");

    {
        scope xformCommonAPIScope = cls;
        TfPyWrapEnum<This::RotationOrder>();
    }

    cls
        .def(init<UsdPrim>(arg("prim")))

        .def(init<UsdGeomXformable>(arg("xformable")))

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def(!self)

        .def("SetXformVectors", &This::SetXformVectors,
            (arg("translation"),
             arg("rotation"),
             arg("scale"),
             arg("pivot"),
             arg("rotationOrder"),
             arg("time")))

        .def("GetXformVectors", &_GetXformVectors, 
            arg("time"))

        .def("GetXformVectorsByAccumulation", &_GetXformVectorsByAccumulation, 
            arg("time"))

        .def("SetTranslate", &This::SetTranslate,
            (arg("translation"),
             arg("time")=UsdTimeCode::Default()))

        .def("SetPivot", &This::SetPivot,
            (arg("pivot"),
             arg("time")=UsdTimeCode::Default()))

        .def("SetRotate", &This::SetRotate,
            (arg("rotation"),
             arg("rotationOrder")=This::RotationOrderXYZ,
             arg("time")=UsdTimeCode::Default()))

        .def("SetScale", &This::SetScale,
            (arg("scale"),
             arg("time")=UsdTimeCode::Default()))

        .def("GetResetXformStack", &This::GetResetXformStack)

        .def("SetResetXformStack", &This::SetResetXformStack,
            arg("resetXformStack"))
        ;
}
