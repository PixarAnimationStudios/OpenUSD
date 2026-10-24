//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/pointInstancer.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/wrapTypeHelpers.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/tuple.h>

namespace nb = nanobind;

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateProtoIndicesAttr(UsdGeomPointInstancer &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateProtoIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateIdsAttr(UsdGeomPointInstancer &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateIdsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int64Array), writeSparsely);
}
        
static UsdAttribute
_CreatePositionsAttr(UsdGeomPointInstancer &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreatePositionsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateOrientationsAttr(UsdGeomPointInstancer &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateOrientationsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->QuathArray), writeSparsely);
}
        
static UsdAttribute
_CreateOrientationsfAttr(UsdGeomPointInstancer &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateOrientationsfAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->QuatfArray), writeSparsely);
}
        
static UsdAttribute
_CreateScalesAttr(UsdGeomPointInstancer &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateScalesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3Array), writeSparsely);
}
        
static UsdAttribute
_CreateVelocitiesAttr(UsdGeomPointInstancer &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateVelocitiesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateAccelerationsAttr(UsdGeomPointInstancer &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateAccelerationsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateAngularVelocitiesAttr(UsdGeomPointInstancer &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateAngularVelocitiesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateInvisibleIdsAttr(UsdGeomPointInstancer &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateInvisibleIdsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int64Array), writeSparsely);
}

static std::string
_Repr(const UsdGeomPointInstancer &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.PointInstancer(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdGeomPointInstancer(nb::module_ &m)
{
    typedef UsdGeomPointInstancer This;

    nb::class_<This, UsdGeomBoundable> cls(m, "PointInstancer");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

        .def_static("Define", &This::Define, nb::arg("stage"), nb::arg("path"))

        .def_static("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        
        .def("GetProtoIndicesAttr",
             &This::GetProtoIndicesAttr)
        .def("CreateProtoIndicesAttr",
             &_CreateProtoIndicesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetIdsAttr",
             &This::GetIdsAttr)
        .def("CreateIdsAttr",
             &_CreateIdsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetPositionsAttr",
             &This::GetPositionsAttr)
        .def("CreatePositionsAttr",
             &_CreatePositionsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetOrientationsAttr",
             &This::GetOrientationsAttr)
        .def("CreateOrientationsAttr",
             &_CreateOrientationsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetOrientationsfAttr",
             &This::GetOrientationsfAttr)
        .def("CreateOrientationsfAttr",
             &_CreateOrientationsfAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetScalesAttr",
             &This::GetScalesAttr)
        .def("CreateScalesAttr",
             &_CreateScalesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetVelocitiesAttr",
             &This::GetVelocitiesAttr)
        .def("CreateVelocitiesAttr",
             &_CreateVelocitiesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetAccelerationsAttr",
             &This::GetAccelerationsAttr)
        .def("CreateAccelerationsAttr",
             &_CreateAccelerationsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetAngularVelocitiesAttr",
             &This::GetAngularVelocitiesAttr)
        .def("CreateAngularVelocitiesAttr",
             &_CreateAngularVelocitiesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetInvisibleIdsAttr",
             &This::GetInvisibleIdsAttr)
        .def("CreateInvisibleIdsAttr",
             &_CreateInvisibleIdsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        
        .def("GetPrototypesRel",
             &This::GetPrototypesRel)
        .def("CreatePrototypesRel",
             &This::CreatePrototypesRel)
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

static
nb::list
_ComputeMaskAtTime(const UsdGeomPointInstancer& self,
                   const UsdTimeCode time)
{
    nb::list items;
    for (const auto& b : self.ComputeMaskAtTime(time)) {
        items.append(static_cast<bool>(b));
    }

    return items;
}

static
VtMatrix4dArray
_ComputeInstanceTransformsAtTime(
    const UsdGeomPointInstancer& self,
    const UsdTimeCode time,
    const UsdTimeCode baseTime,
    const UsdGeomPointInstancer::ProtoXformInclusion doProtoXforms,
    const UsdGeomPointInstancer::MaskApplication applyMask)
{
    VtMatrix4dArray xforms;

    // On error we'll be returning an empty array.
    self.ComputeInstanceTransformsAtTime(&xforms, time, baseTime,
                                         doProtoXforms, applyMask);

    return xforms;
}

static
std::vector<VtMatrix4dArray>
_ComputeInstanceTransformsAtTimes(
    const UsdGeomPointInstancer& self,
    const std::vector<UsdTimeCode>& times,
    const UsdTimeCode baseTime,
    const UsdGeomPointInstancer::ProtoXformInclusion doProtoXforms,
    const UsdGeomPointInstancer::MaskApplication applyMask)
{
    std::vector<VtMatrix4dArray> xforms;

    // On error we'll be returning an empty array.
    self.ComputeInstanceTransformsAtTimes(&xforms, times, baseTime,
                                         doProtoXforms, applyMask);

    return xforms;
}

static
VtVec3fArray
_ComputeExtentAtTime(
    const UsdGeomPointInstancer& self,
    const UsdTimeCode time,
    const UsdTimeCode baseTime)
{
    VtVec3fArray extent;

    // On error we'll be returning an empty array.
    self.ComputeExtentAtTime(&extent, time, baseTime);

    return extent;
}

static
std::vector<VtVec3fArray>
_ComputeExtentAtTimes(
    const UsdGeomPointInstancer& self,
    const std::vector<UsdTimeCode>& times,
    const UsdTimeCode baseTime)
{
    std::vector<VtVec3fArray> extents;

    // On error we'll be returning an empty array.
    self.ComputeExtentAtTimes(&extents, times, baseTime);

    return extents;
}

WRAP_CUSTOM {

    typedef UsdGeomPointInstancer This;

    // class needs to be in-scope for enums to get wrapped properly
    TfNbWrapEnum<This::MaskApplication>(_class);

    TfNbWrapEnum<This::ProtoXformInclusion>(_class);

    _class
        .def("ActivateId", 
             &This::ActivateId,
             nb::arg("id"))
        .def("ActivateIds", 
             &This::ActivateIds,
             nb::arg("ids"))
        .def("ActivateAllIds", 
             &This::ActivateAllIds)
        .def("DeactivateId", 
             &This::DeactivateId,
             nb::arg("id"))
        .def("DeactivateIds", 
             &This::DeactivateIds,
             nb::arg("ids"))

        .def("VisId", 
             &This::VisId,
             nb::arg("id"), nb::arg("time"))
        .def("VisIds", 
             &This::VisIds,
             nb::arg("ids"), nb::arg("time"))
        .def("VisAllIds", 
             &This::VisAllIds,
             nb::arg("time"))
        .def("InvisId", 
             &This::InvisId,
             nb::arg("id"), nb::arg("time"))
        .def("InvisIds", 
             &This::InvisIds,
             nb::arg("ids"), nb::arg("time"))
        
        // The cost to fetch 'ids' is likely dwarfed by python marshalling
        // costs, so let's not worry about the 'ids' optional arg
        .def("ComputeMaskAtTime",
             &_ComputeMaskAtTime,
             nb::arg("time"))

        .def("ComputeInstanceTransformsAtTime",
             &_ComputeInstanceTransformsAtTime,
             nb::arg("time"), nb::arg("baseTime"),
              nb::arg("doProtoXforms")=This::IncludeProtoXform,
              nb::arg("applyMask")=This::ApplyMask)
        .def("ComputeInstanceTransformsAtTimes",
             &_ComputeInstanceTransformsAtTimes,
             nb::arg("times"), nb::arg("baseTime"),
              nb::arg("doProtoXforms")=This::IncludeProtoXform,
              nb::arg("applyMask")=This::ApplyMask)

        .def("ComputeExtentAtTime",
             &_ComputeExtentAtTime,
             nb::arg("time"), nb::arg("baseTime"))
        .def("ComputeExtentAtTimes",
             &_ComputeExtentAtTimes,
             nb::arg("times"), nb::arg("baseTime"))

        .def("GetInstanceCount", &UsdGeomPointInstancer::GetInstanceCount,
            nb::arg("timeCode")=UsdTimeCode::Default()) 
        ;
    TfPyRegisterStlSequencesFromPython<UsdTimeCode>();
}

} // anonymous namespace
