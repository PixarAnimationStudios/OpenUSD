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


PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrUsdUsdGeomWrapXformCommonAPI {

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

    boost::python::class_<This, boost::python::bases<UsdAPISchemaBase> >
        cls("XformCommonAPI");

    cls
        .def(boost::python::init<UsdPrim>(boost::python::arg("prim")))
        .def(boost::python::init<UsdSchemaBase const&>(boost::python::arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Get")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             boost::python::arg("includeInherited")=true,
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!boost::python::self)


        .def("__repr__", pxrUsdUsdGeomWrapXformCommonAPI::_Repr)
    ;

    pxrUsdUsdGeomWrapXformCommonAPI::_CustomWrapCode(cls);
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

namespace pxrUsdUsdGeomWrapXformCommonAPI {

static boost::python::tuple 
_GetXformVectors(
    UsdGeomXformCommonAPI self, 
    const UsdTimeCode &time)
{
    GfVec3d translation;
    GfVec3f rotation, scale, pivot;
    UsdGeomXformCommonAPI::RotationOrder rotationOrder;

    bool result = self.GetXformVectors(&translation, &rotation, &scale, 
                                       &pivot, &rotationOrder, time);

    return result ? boost::python::make_tuple(translation, rotation, scale, pivot, rotationOrder)
                  : boost::python::tuple();
}

static boost::python::tuple 
_GetXformVectorsByAccumulation(
    UsdGeomXformCommonAPI self, 
    const UsdTimeCode &time)
{
    GfVec3d translation;
    GfVec3f rotation, scale, pivot;
    UsdGeomXformCommonAPI::RotationOrder rotationOrder;

    bool result = self.GetXformVectorsByAccumulation(&translation, &rotation, &scale, 
                                                     &pivot, &rotationOrder, time);

    return result ? boost::python::make_tuple(translation, rotation, scale, pivot, rotationOrder)
                  : boost::python::tuple();
}

static boost::python::tuple
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
    return boost::python::make_tuple(
        ops.translateOp,
        ops.pivotOp,
        ops.rotateOp,
        ops.scaleOp,
        ops.inversePivotOp);
}

static boost::python::tuple
_CreateXformOps2(
    UsdGeomXformCommonAPI self,
    UsdGeomXformCommonAPI::OpFlags op1,
    UsdGeomXformCommonAPI::OpFlags op2,
    UsdGeomXformCommonAPI::OpFlags op3,
    UsdGeomXformCommonAPI::OpFlags op4)
{
    UsdGeomXformCommonAPI::Ops ops = self.CreateXformOps(op1, op2, op3, op4);
    return boost::python::make_tuple(
        ops.translateOp,
        ops.pivotOp,
        ops.rotateOp,
        ops.scaleOp,
        ops.inversePivotOp);
}

WRAP_CUSTOM {
    using This = UsdGeomXformCommonAPI;

    {
        boost::python::scope xformCommonAPIScope = _class;
        TfPyWrapEnum<This::RotationOrder>();
        TfPyWrapEnum<This::OpFlags>();
    }

    _class
        .def("SetXformVectors", &This::SetXformVectors,
            (boost::python::arg("translation"),
             boost::python::arg("rotation"),
             boost::python::arg("scale"),
             boost::python::arg("pivot"),
             boost::python::arg("rotationOrder"),
             boost::python::arg("time")))

        .def("GetXformVectors", &_GetXformVectors, 
            boost::python::arg("time"))

        .def("GetXformVectorsByAccumulation", &_GetXformVectorsByAccumulation, 
            boost::python::arg("time"))

        .def("SetTranslate", &This::SetTranslate,
            (boost::python::arg("translation"),
             boost::python::arg("time")=UsdTimeCode::Default()))

        .def("SetPivot", &This::SetPivot,
            (boost::python::arg("pivot"),
             boost::python::arg("time")=UsdTimeCode::Default()))

        .def("SetRotate", &This::SetRotate,
            (boost::python::arg("rotation"),
             boost::python::arg("rotationOrder")=This::RotationOrderXYZ,
             boost::python::arg("time")=UsdTimeCode::Default()))

        .def("SetScale", &This::SetScale,
            (boost::python::arg("scale"),
             boost::python::arg("time")=UsdTimeCode::Default()))

        .def("GetResetXformStack", &This::GetResetXformStack)

        .def("SetResetXformStack", &This::SetResetXformStack,
            boost::python::arg("resetXformStack"))

        .def("CreateXformOps", _CreateXformOps1,
            (boost::python::arg("rotationOrder"),
             boost::python::arg("op1")=UsdGeomXformCommonAPI::OpNone,
             boost::python::arg("op2")=UsdGeomXformCommonAPI::OpNone,
             boost::python::arg("op3")=UsdGeomXformCommonAPI::OpNone,
             boost::python::arg("op4")=UsdGeomXformCommonAPI::OpNone))

        .def("CreateXformOps", _CreateXformOps2,
            (boost::python::arg("op1")=UsdGeomXformCommonAPI::OpNone,
             boost::python::arg("op2")=UsdGeomXformCommonAPI::OpNone,
             boost::python::arg("op3")=UsdGeomXformCommonAPI::OpNone,
             boost::python::arg("op4")=UsdGeomXformCommonAPI::OpNone))

        .def("GetRotationTransform", &This::GetRotationTransform,
            (boost::python::arg("rotation"), boost::python::arg("rotationOrder")))
        .staticmethod("GetRotationTransform")

        .def("ConvertRotationOrderToOpType",
            &This::ConvertRotationOrderToOpType,
            boost::python::arg("rotationOrder"))
        .staticmethod("ConvertRotationOrderToOpType")

        .def("ConvertOpTypeToRotationOrder",
            &This::ConvertOpTypeToRotationOrder,
            boost::python::arg("opType"))
        .staticmethod("ConvertOpTypeToRotationOrder")

        .def("CanConvertOpTypeToRotationOrder",
            &This::CanConvertOpTypeToRotationOrder,
            boost::python::arg("opType"))
        .staticmethod("CanConvertOpTypeToRotationOrder")
        ;
}

}
