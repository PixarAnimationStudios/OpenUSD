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
#include "pxr/usd/usd/conversions.h"

#include "pxr/usd/sdf/primSpec.h"

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

        
static UsdAttribute
_CreateProtoIndicesAttr(UsdGeomPointInstancer &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateProtoIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateIdsAttr(UsdGeomPointInstancer &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateIdsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int64Array), writeSparsely);
}
        
static UsdAttribute
_CreatePositionsAttr(UsdGeomPointInstancer &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePositionsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateOrientationsAttr(UsdGeomPointInstancer &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateOrientationsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->QuathArray), writeSparsely);
}
        
static UsdAttribute
_CreateScalesAttr(UsdGeomPointInstancer &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateScalesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3Array), writeSparsely);
}
        
static UsdAttribute
_CreateVelocitiesAttr(UsdGeomPointInstancer &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateVelocitiesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateAngularVelocitiesAttr(UsdGeomPointInstancer &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAngularVelocitiesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateInvisibleIdsAttr(UsdGeomPointInstancer &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateInvisibleIdsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int64Array), writeSparsely);
}
        
static UsdAttribute
_CreatePrototypeDrawModeAttr(UsdGeomPointInstancer &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePrototypeDrawModeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

} // anonymous namespace

void wrapUsdGeomPointInstancer()
{
    typedef UsdGeomPointInstancer This;

    class_<This, bases<UsdGeomBoundable> >
        cls("PointInstancer");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("Define", &This::Define, (arg("stage"), arg("path")))
        .staticmethod("Define")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetProtoIndicesAttr",
             &This::GetProtoIndicesAttr)
        .def("CreateProtoIndicesAttr",
             &_CreateProtoIndicesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetIdsAttr",
             &This::GetIdsAttr)
        .def("CreateIdsAttr",
             &_CreateIdsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetPositionsAttr",
             &This::GetPositionsAttr)
        .def("CreatePositionsAttr",
             &_CreatePositionsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetOrientationsAttr",
             &This::GetOrientationsAttr)
        .def("CreateOrientationsAttr",
             &_CreateOrientationsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetScalesAttr",
             &This::GetScalesAttr)
        .def("CreateScalesAttr",
             &_CreateScalesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetVelocitiesAttr",
             &This::GetVelocitiesAttr)
        .def("CreateVelocitiesAttr",
             &_CreateVelocitiesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAngularVelocitiesAttr",
             &This::GetAngularVelocitiesAttr)
        .def("CreateAngularVelocitiesAttr",
             &_CreateAngularVelocitiesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetInvisibleIdsAttr",
             &This::GetInvisibleIdsAttr)
        .def("CreateInvisibleIdsAttr",
             &_CreateInvisibleIdsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetPrototypeDrawModeAttr",
             &This::GetPrototypeDrawModeAttr)
        .def("CreatePrototypeDrawModeAttr",
             &_CreatePrototypeDrawModeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        
        .def("GetPrototypesRel",
             &This::GetPrototypesRel)
        .def("CreatePrototypesRel",
             &This::CreatePrototypesRel)
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
std::vector<bool>
_ComputeMaskAtTime(
    const UsdGeomPointInstancer& self,
    const UsdTimeCode time)
{
    return self.ComputeMaskAtTime(time);
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

WRAP_CUSTOM {

    typedef UsdGeomPointInstancer This;

    TfPyWrapEnum<This::MaskApplication>();

    TfPyWrapEnum<This::ProtoXformInclusion>();

    _class
        .def("ActivateId", 
             &This::ActivateId,
             (arg("id")))
        .def("ActivateIds", 
             &This::ActivateIds,
             (arg("ids")))
        .def("ActivateAllIds", 
             &This::ActivateAllIds)
        .def("DeactivateId", 
             &This::DeactivateId,
             (arg("id")))
        .def("DeactivateIds", 
             &This::DeactivateIds,
             (arg("ids")))

        .def("VisId", 
             &This::VisId,
             (arg("id"), arg("time")))
        .def("VisIds", 
             &This::VisIds,
             (arg("ids"), arg("time")))
        .def("VisAllIds", 
             &This::VisAllIds,
             (arg("time")))
        .def("InvisId", 
             &This::InvisId,
             (arg("id"), arg("time")))
        .def("InvisIds", 
             &This::InvisIds,
             (arg("ids"), arg("time")))
        
        // The cost to fetch 'ids' is likely dwarfed by python marshalling
        // costs, so let's not worry about the 'ids' optional arg
        .def("ComputeMaskAtTime",
             &_ComputeMaskAtTime,
             (arg("time")))

        .def("ComputeInstanceTransformsAtTime",
             &_ComputeInstanceTransformsAtTime,
             (arg("time"), arg("baseTime"),
              arg("doProtoXforms")=This::IncludeProtoXform,
              arg("applyMask")=This::ApplyMask))

        .def("ComputeExtentAtTime",
             &_ComputeExtentAtTime,
             (arg("time"), arg("baseTime")))

        ;

}

} // anonymous namespace
