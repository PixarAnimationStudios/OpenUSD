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
#include "pxr/usd/usdGeom/pointBased.h"
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

namespace pxrUsdUsdGeomWrapPointBased {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreatePointsAttr(UsdGeomPointBased &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreatePointsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateVelocitiesAttr(UsdGeomPointBased &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateVelocitiesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateAccelerationsAttr(UsdGeomPointBased &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateAccelerationsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateNormalsAttr(UsdGeomPointBased &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateNormalsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Normal3fArray), writeSparsely);
}

static std::string
_Repr(const UsdGeomPointBased &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.PointBased(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdGeomPointBased()
{
    typedef UsdGeomPointBased This;

    boost::python::class_<This, boost::python::bases<UsdGeomGprim> >
        cls("PointBased");

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

        
        .def("GetPointsAttr",
             &This::GetPointsAttr)
        .def("CreatePointsAttr",
             &pxrUsdUsdGeomWrapPointBased::_CreatePointsAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetVelocitiesAttr",
             &This::GetVelocitiesAttr)
        .def("CreateVelocitiesAttr",
             &pxrUsdUsdGeomWrapPointBased::_CreateVelocitiesAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetAccelerationsAttr",
             &This::GetAccelerationsAttr)
        .def("CreateAccelerationsAttr",
             &pxrUsdUsdGeomWrapPointBased::_CreateAccelerationsAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetNormalsAttr",
             &This::GetNormalsAttr)
        .def("CreateNormalsAttr",
             &pxrUsdUsdGeomWrapPointBased::_CreateNormalsAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))

        .def("__repr__", pxrUsdUsdGeomWrapPointBased::_Repr)
    ;

    pxrUsdUsdGeomWrapPointBased::_CustomWrapCode(cls);
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
namespace pxrUsdUsdGeomWrapPointBased {

static TfPyObjWrapper 
_ComputeExtent(boost::python::object points) {

    // Convert from python objects to VtValue
    VtVec3fArray extent;
    VtValue pointsAsVtValue = UsdPythonToSdfType(points, 
        SdfValueTypeNames->Float3Array);

    // Check for proper conversion to VtVec3fArray
    if (!pointsAsVtValue.IsHolding<VtVec3fArray>()) {
        TF_CODING_ERROR("Improper value for 'points'");
        return boost::python::object();
    }

    // Convert from VtValue to VtVec3fArray
    VtVec3fArray pointsArray = pointsAsVtValue.UncheckedGet<VtVec3fArray>();
    if (UsdGeomPointBased::ComputeExtent(pointsArray, &extent)) {
        return UsdVtValueToPython(VtValue(extent));
    } else {
        return boost::python::object();
    }
}

static
VtVec3fArray
_ComputePointsAtTime(
    const UsdGeomPointBased& self,
    const UsdTimeCode time,
    const UsdTimeCode baseTime)
{
    VtVec3fArray points;

    // On error we'll be returning an empty array.
    self.ComputePointsAtTime(&points, time, baseTime);

    return points;
}

static
std::vector<VtVec3fArray>
_ComputePointsAtTimes(
    const UsdGeomPointBased& self,
    const std::vector<UsdTimeCode>& times,
    const UsdTimeCode baseTime)
{
    std::vector<VtVec3fArray> points;

    // On error we'll be returning an empty array.
    self.ComputePointsAtTimes(&points, times, baseTime);

    return points;
}

WRAP_CUSTOM {
    _class
        .def("GetNormalsInterpolation",
             &UsdGeomPointBased::GetNormalsInterpolation)
        .def("SetNormalsInterpolation",
             &UsdGeomPointBased::SetNormalsInterpolation,
             boost::python::arg("interpolation"))

        .def("ComputeExtent",
            &_ComputeExtent, 
            (boost::python::arg("points")))
        .staticmethod("ComputeExtent")

        .def("ComputePointsAtTime",
             &_ComputePointsAtTime,
             (boost::python::arg("time"), boost::python::arg("baseTime")))
        .def("ComputePointsAtTimes",
             &_ComputePointsAtTimes,
             (boost::python::arg("times"), boost::python::arg("baseTime")))

        ;
}

} // anonymous namespace
