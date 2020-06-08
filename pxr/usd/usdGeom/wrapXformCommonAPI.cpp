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
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;


static std::string
_Repr(const UsdGeomXformCommonAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.XformCommonAPI(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdGeomXformCommonAPI()
{
    typedef UsdGeomXformCommonAPI This;

    class_<This, bases<UsdAPISchemaBase> >
        cls("XformCommonAPI");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)


        .def("__repr__", ::_Repr)
    ;

    _CustomWrapCode(cls);
}

// ===================================================================== //
// Feel free to add custom code below this line, it will be preserved by 
// the code generator.  The entry point for your custom code should look
// minimally like the following:
//
// WRAP_CUSTOM {
//     _class
//         .def("MyCustomMethod", ...)
//     ;
// }
//
// Of course any other ancillary or support code may be provided.
// 
// Just remember to wrap code in the appropriate delimiters:
// 'namespace {', '}'.
//
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/base/tf/pyEnum.h"

namespace {

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

static tuple
_CreateXformOps1(
    UsdGeomXformCommonAPI self,
    UsdGeomXformCommonAPI::RotationOrder rotOrder,
    UsdGeomXformCommonAPI::OpFlags op1,
    UsdGeomXformCommonAPI::OpFlags op2,
    UsdGeomXformCommonAPI::OpFlags op3,
    UsdGeomXformCommonAPI::OpFlags op4)
{
    UsdGeomXformCommonAPI::Ops ops = self.CreateXformOps(
        rotOrder, op1, op2, op3, op4);
    return make_tuple(
        ops.translateOp,
        ops.pivotOp,
        ops.rotateOp,
        ops.scaleOp,
        ops.inversePivotOp);
}

static tuple
_CreateXformOps2(
    UsdGeomXformCommonAPI self,
    UsdGeomXformCommonAPI::OpFlags op1,
    UsdGeomXformCommonAPI::OpFlags op2,
    UsdGeomXformCommonAPI::OpFlags op3,
    UsdGeomXformCommonAPI::OpFlags op4)
{
    UsdGeomXformCommonAPI::Ops ops = self.CreateXformOps(op1, op2, op3, op4);
    return make_tuple(
        ops.translateOp,
        ops.pivotOp,
        ops.rotateOp,
        ops.scaleOp,
        ops.inversePivotOp);
}

WRAP_CUSTOM {
    using This = UsdGeomXformCommonAPI;

    {
        scope xformCommonAPIScope = _class;
        TfPyWrapEnum<This::RotationOrder>();
        TfPyWrapEnum<This::OpFlags>();
    }

    _class
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

        .def("CreateXformOps", _CreateXformOps1,
            (arg("rotationOrder"),
             arg("op1")=UsdGeomXformCommonAPI::OpNone,
             arg("op2")=UsdGeomXformCommonAPI::OpNone,
             arg("op3")=UsdGeomXformCommonAPI::OpNone,
             arg("op4")=UsdGeomXformCommonAPI::OpNone))

        .def("CreateXformOps", _CreateXformOps2,
            (arg("op1")=UsdGeomXformCommonAPI::OpNone,
             arg("op2")=UsdGeomXformCommonAPI::OpNone,
             arg("op3")=UsdGeomXformCommonAPI::OpNone,
             arg("op4")=UsdGeomXformCommonAPI::OpNone))

        .def("GetRotationTransform", &This::GetRotationTransform,
            (arg("rotation"), arg("rotationOrder")))
        .staticmethod("GetRotationTransform")

        .def("ConvertRotationOrderToOpType",
            &This::ConvertRotationOrderToOpType,
            arg("rotationOrder"))
        .staticmethod("ConvertRotationOrderToOpType")

        .def("ConvertOpTypeToRotationOrder",
            &This::ConvertOpTypeToRotationOrder,
            arg("opType"))
        .staticmethod("ConvertOpTypeToRotationOrder")

        .def("CanConvertOpTypeToRotationOrder",
            &This::CanConvertOpTypeToRotationOrder,
            arg("opType"))
        .staticmethod("CanConvertOpTypeToRotationOrder")
        ;
}

}
