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
#include "pxr/usd/usdGeom/pointInstancer.h"
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

namespace pxrUsdUsdGeomWrapPointInstancer {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateProtoIndicesAttr(UsdGeomPointInstancer &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateProtoIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateIdsAttr(UsdGeomPointInstancer &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateIdsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int64Array), writeSparsely);
}
        
static UsdAttribute
_CreatePositionsAttr(UsdGeomPointInstancer &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreatePositionsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateOrientationsAttr(UsdGeomPointInstancer &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateOrientationsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->QuathArray), writeSparsely);
}
        
static UsdAttribute
_CreateScalesAttr(UsdGeomPointInstancer &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateScalesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3Array), writeSparsely);
}
        
static UsdAttribute
_CreateVelocitiesAttr(UsdGeomPointInstancer &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateVelocitiesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateAccelerationsAttr(UsdGeomPointInstancer &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateAccelerationsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateAngularVelocitiesAttr(UsdGeomPointInstancer &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateAngularVelocitiesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateInvisibleIdsAttr(UsdGeomPointInstancer &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
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

void wrapUsdGeomPointInstancer()
{
    typedef UsdGeomPointInstancer This;

    boost::python::class_<This, boost::python::bases<UsdGeomBoundable> >
        cls("PointInstancer");

    cls
        .def(boost::python::init<UsdPrim>(boost::python::arg("prim")))
        .def(boost::python::init<UsdSchemaBase const&>(boost::python::arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Get")

        .def("Define", &This::Define, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Define")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             boost::python::arg("includeInherited")=true,
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!boost::python::self)

        
        .def("GetProtoIndicesAttr",
             &This::GetProtoIndicesAttr)
        .def("CreateProtoIndicesAttr",
             &pxrUsdUsdGeomWrapPointInstancer::_CreateProtoIndicesAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetIdsAttr",
             &This::GetIdsAttr)
        .def("CreateIdsAttr",
             &pxrUsdUsdGeomWrapPointInstancer::_CreateIdsAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetPositionsAttr",
             &This::GetPositionsAttr)
        .def("CreatePositionsAttr",
             &pxrUsdUsdGeomWrapPointInstancer::_CreatePositionsAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetOrientationsAttr",
             &This::GetOrientationsAttr)
        .def("CreateOrientationsAttr",
             &pxrUsdUsdGeomWrapPointInstancer::_CreateOrientationsAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetScalesAttr",
             &This::GetScalesAttr)
        .def("CreateScalesAttr",
             &pxrUsdUsdGeomWrapPointInstancer::_CreateScalesAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetVelocitiesAttr",
             &This::GetVelocitiesAttr)
        .def("CreateVelocitiesAttr",
             &pxrUsdUsdGeomWrapPointInstancer::_CreateVelocitiesAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetAccelerationsAttr",
             &This::GetAccelerationsAttr)
        .def("CreateAccelerationsAttr",
             &pxrUsdUsdGeomWrapPointInstancer::_CreateAccelerationsAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetAngularVelocitiesAttr",
             &This::GetAngularVelocitiesAttr)
        .def("CreateAngularVelocitiesAttr",
             &pxrUsdUsdGeomWrapPointInstancer::_CreateAngularVelocitiesAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetInvisibleIdsAttr",
             &This::GetInvisibleIdsAttr)
        .def("CreateInvisibleIdsAttr",
             &pxrUsdUsdGeomWrapPointInstancer::_CreateInvisibleIdsAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))

        
        .def("GetPrototypesRel",
             &This::GetPrototypesRel)
        .def("CreatePrototypesRel",
             &This::CreatePrototypesRel)
        .def("__repr__", pxrUsdUsdGeomWrapPointInstancer::_Repr)
    ;

    pxrUsdUsdGeomWrapPointInstancer::_CustomWrapCode(cls);
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

namespace pxrUsdUsdGeomWrapPointInstancer {

static
boost::python::list
_ComputeMaskAtTime(const UsdGeomPointInstancer& self,
                   const UsdTimeCode time)
{
    boost::python::list items;
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
    boost::python::scope obj = _class;

    TfPyWrapEnum<This::MaskApplication>();

    TfPyWrapEnum<This::ProtoXformInclusion>();

    _class
        .def("ActivateId", 
             &This::ActivateId,
             (boost::python::arg("id")))
        .def("ActivateIds", 
             &This::ActivateIds,
             (boost::python::arg("ids")))
        .def("ActivateAllIds", 
             &This::ActivateAllIds)
        .def("DeactivateId", 
             &This::DeactivateId,
             (boost::python::arg("id")))
        .def("DeactivateIds", 
             &This::DeactivateIds,
             (boost::python::arg("ids")))

        .def("VisId", 
             &This::VisId,
             (boost::python::arg("id"), boost::python::arg("time")))
        .def("VisIds", 
             &This::VisIds,
             (boost::python::arg("ids"), boost::python::arg("time")))
        .def("VisAllIds", 
             &This::VisAllIds,
             (boost::python::arg("time")))
        .def("InvisId", 
             &This::InvisId,
             (boost::python::arg("id"), boost::python::arg("time")))
        .def("InvisIds", 
             &This::InvisIds,
             (boost::python::arg("ids"), boost::python::arg("time")))
        
        // The cost to fetch 'ids' is likely dwarfed by python marshalling
        // costs, so let's not worry about the 'ids' optional arg
        .def("ComputeMaskAtTime",
             &_ComputeMaskAtTime,
             (boost::python::arg("time")))

        .def("ComputeInstanceTransformsAtTime",
             &_ComputeInstanceTransformsAtTime,
             (boost::python::arg("time"), boost::python::arg("baseTime"),
              boost::python::arg("doProtoXforms")=This::IncludeProtoXform,
              boost::python::arg("applyMask")=This::ApplyMask))
        .def("ComputeInstanceTransformsAtTimes",
             &_ComputeInstanceTransformsAtTimes,
             (boost::python::arg("times"), boost::python::arg("baseTime"),
              boost::python::arg("doProtoXforms")=This::IncludeProtoXform,
              boost::python::arg("applyMask")=This::ApplyMask))

        .def("ComputeExtentAtTime",
             &_ComputeExtentAtTime,
             (boost::python::arg("time"), boost::python::arg("baseTime")))
        .def("ComputeExtentAtTimes",
             &_ComputeExtentAtTimes,
             (boost::python::arg("times"), boost::python::arg("baseTime")))

        .def("GetInstanceCount", &UsdGeomPointInstancer::GetInstanceCount,
            boost::python::arg("timeCode")=UsdTimeCode::Default()) 
        ;
    TfPyRegisterStlSequencesFromPython<UsdTimeCode>();
    boost::python::to_python_converter<std::vector<VtArray<GfMatrix4d>>,
        TfPySequenceToPython<std::vector<VtArray<GfMatrix4d>>>>();
    boost::python::to_python_converter<std::vector<VtVec3fArray>,
        TfPySequenceToPython<std::vector<VtVec3fArray>>>();
}

} // anonymous namespace
