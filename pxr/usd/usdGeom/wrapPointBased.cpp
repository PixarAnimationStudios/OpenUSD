//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/pointBased.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/external/boost/python.hpp"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreatePointsAttr(UsdGeomPointBased &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePointsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateVelocitiesAttr(UsdGeomPointBased &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateVelocitiesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateAccelerationsAttr(UsdGeomPointBased &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAccelerationsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateNormalsAttr(UsdGeomPointBased &self,
                                      object defaultVal, bool writeSparsely) {
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

    class_<This, bases<UsdGeomGprim> >
        cls("PointBased");

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

        
        .def("GetPointsAttr",
             &This::GetPointsAttr)
        .def("CreatePointsAttr",
             &_CreatePointsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetVelocitiesAttr",
             &This::GetVelocitiesAttr)
        .def("CreateVelocitiesAttr",
             &_CreateVelocitiesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAccelerationsAttr",
             &This::GetAccelerationsAttr)
        .def("CreateAccelerationsAttr",
             &_CreateAccelerationsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetNormalsAttr",
             &This::GetNormalsAttr)
        .def("CreateNormalsAttr",
             &_CreateNormalsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

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
namespace {

static TfPyObjWrapper 
_ComputeExtent(object points) {

    // Convert from python objects to VtValue
    VtVec3fArray extent;
    VtValue pointsAsVtValue = UsdPythonToSdfType(points, 
        SdfValueTypeNames->Float3Array);

    // Check for proper conversion to VtVec3fArray
    if (!pointsAsVtValue.IsHolding<VtVec3fArray>()) {
        TF_CODING_ERROR("Improper value for 'points'");
        return object();
    }

    // Convert from VtValue to VtVec3fArray
    VtVec3fArray pointsArray = pointsAsVtValue.UncheckedGet<VtVec3fArray>();
    if (UsdGeomPointBased::ComputeExtent(pointsArray, &extent)) {
        return UsdVtValueToPython(VtValue(extent));
    } else {
        return object();
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
             arg("interpolation"))

        .def("ComputeExtent",
            &_ComputeExtent, 
            (arg("points")))
        .staticmethod("ComputeExtent")

        .def("ComputePointsAtTime",
             &_ComputePointsAtTime,
             (arg("time"), arg("baseTime")))
        .def("ComputePointsAtTimes",
             &_ComputePointsAtTimes,
             (arg("times"), arg("baseTime")))

        ;
}

} // anonymous namespace
