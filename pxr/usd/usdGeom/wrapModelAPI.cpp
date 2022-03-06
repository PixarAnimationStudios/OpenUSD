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
#include "pxr/usd/usdGeom/modelAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>


PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrUsdUsdGeomWrapModelAPI {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateModelDrawModeAttr(UsdGeomModelAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateModelDrawModeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateModelApplyDrawModeAttr(UsdGeomModelAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateModelApplyDrawModeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateModelDrawModeColorAttr(UsdGeomModelAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateModelDrawModeColorAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3), writeSparsely);
}
        
static UsdAttribute
_CreateModelCardGeometryAttr(UsdGeomModelAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateModelCardGeometryAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateModelCardTextureXPosAttr(UsdGeomModelAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateModelCardTextureXPosAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateModelCardTextureYPosAttr(UsdGeomModelAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateModelCardTextureYPosAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateModelCardTextureZPosAttr(UsdGeomModelAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateModelCardTextureZPosAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateModelCardTextureXNegAttr(UsdGeomModelAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateModelCardTextureXNegAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateModelCardTextureYNegAttr(UsdGeomModelAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateModelCardTextureYNegAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateModelCardTextureZNegAttr(UsdGeomModelAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateModelCardTextureZNegAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}

static std::string
_Repr(const UsdGeomModelAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.ModelAPI(%s)",
        primRepr.c_str());
}

struct UsdGeomModelAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdGeomModelAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdGeomModelAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdGeomModelAPI::CanApply(prim, &whyNot);
    return UsdGeomModelAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdGeomModelAPI()
{
    typedef UsdGeomModelAPI This;

    pxrUsdUsdGeomWrapModelAPI::UsdGeomModelAPI_CanApplyResult::Wrap<pxrUsdUsdGeomWrapModelAPI::UsdGeomModelAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    boost::python::class_<This, boost::python::bases<UsdAPISchemaBase> >
        cls("ModelAPI");

    cls
        .def(boost::python::init<UsdPrim>(boost::python::arg("prim")))
        .def(boost::python::init<UsdSchemaBase const&>(boost::python::arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Get")

        .def("CanApply", &pxrUsdUsdGeomWrapModelAPI::_WrapCanApply, (boost::python::arg("prim")))
        .staticmethod("CanApply")

        .def("Apply", &This::Apply, (boost::python::arg("prim")))
        .staticmethod("Apply")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             boost::python::arg("includeInherited")=true,
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!boost::python::self)

        
        .def("GetModelDrawModeAttr",
             &This::GetModelDrawModeAttr)
        .def("CreateModelDrawModeAttr",
             &pxrUsdUsdGeomWrapModelAPI::_CreateModelDrawModeAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetModelApplyDrawModeAttr",
             &This::GetModelApplyDrawModeAttr)
        .def("CreateModelApplyDrawModeAttr",
             &pxrUsdUsdGeomWrapModelAPI::_CreateModelApplyDrawModeAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetModelDrawModeColorAttr",
             &This::GetModelDrawModeColorAttr)
        .def("CreateModelDrawModeColorAttr",
             &pxrUsdUsdGeomWrapModelAPI::_CreateModelDrawModeColorAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetModelCardGeometryAttr",
             &This::GetModelCardGeometryAttr)
        .def("CreateModelCardGeometryAttr",
             &pxrUsdUsdGeomWrapModelAPI::_CreateModelCardGeometryAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetModelCardTextureXPosAttr",
             &This::GetModelCardTextureXPosAttr)
        .def("CreateModelCardTextureXPosAttr",
             &pxrUsdUsdGeomWrapModelAPI::_CreateModelCardTextureXPosAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetModelCardTextureYPosAttr",
             &This::GetModelCardTextureYPosAttr)
        .def("CreateModelCardTextureYPosAttr",
             &pxrUsdUsdGeomWrapModelAPI::_CreateModelCardTextureYPosAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetModelCardTextureZPosAttr",
             &This::GetModelCardTextureZPosAttr)
        .def("CreateModelCardTextureZPosAttr",
             &pxrUsdUsdGeomWrapModelAPI::_CreateModelCardTextureZPosAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetModelCardTextureXNegAttr",
             &This::GetModelCardTextureXNegAttr)
        .def("CreateModelCardTextureXNegAttr",
             &pxrUsdUsdGeomWrapModelAPI::_CreateModelCardTextureXNegAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetModelCardTextureYNegAttr",
             &This::GetModelCardTextureYNegAttr)
        .def("CreateModelCardTextureYNegAttr",
             &pxrUsdUsdGeomWrapModelAPI::_CreateModelCardTextureYNegAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetModelCardTextureZNegAttr",
             &This::GetModelCardTextureZNegAttr)
        .def("CreateModelCardTextureZNegAttr",
             &pxrUsdUsdGeomWrapModelAPI::_CreateModelCardTextureZNegAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))

        .def("__repr__", pxrUsdUsdGeomWrapModelAPI::_Repr)
    ;

    pxrUsdUsdGeomWrapModelAPI::_CustomWrapCode(cls);
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

namespace pxrUsdUsdGeomWrapModelAPI {

static boost::python::object
_GetExtentsHint(
        const UsdGeomModelAPI& self,
        const UsdTimeCode &time)
{
    VtVec3fArray extents;
    if (!self.GetExtentsHint(&extents, time)) {
        return boost::python::object();
    }

    return boost::python::object(extents);
}

static bool
_SetExtentsHint(
        UsdGeomModelAPI& self,
        const TfPyObjWrapper pyVal,
        const UsdTimeCode &timeVal)
{
    VtValue value = UsdPythonToSdfType(pyVal, SdfValueTypeNames->Float3Array);
    if (!value.IsHolding<VtVec3fArray>()) {
        TF_CODING_ERROR("Improper value for 'extentsHint' on %s",
                        UsdDescribe(self.GetPrim()).c_str());
        return false;
    }

    return self.SetExtentsHint(value.UncheckedGet<VtVec3fArray>(), timeVal);
}

WRAP_CUSTOM {
    _class
        .def("GetExtentsHint", &_GetExtentsHint,
                (boost::python::arg("time")=UsdTimeCode::Default()))
        .def("SetExtentsHint", &_SetExtentsHint,
                (boost::python::arg("extents"),
                 boost::python::arg("time")=UsdTimeCode::Default()))
        .def("ComputeExtentsHint", &UsdGeomModelAPI::ComputeExtentsHint,
                (boost::python::arg("bboxCache")))

        .def("GetExtentsHintAttr", &UsdGeomModelAPI::GetExtentsHintAttr)

        .def("GetConstraintTarget", &UsdGeomModelAPI::GetConstraintTarget)
        .def("CreateConstraintTarget", &UsdGeomModelAPI::CreateConstraintTarget)
        .def("GetConstraintTargets", &UsdGeomModelAPI::GetConstraintTargets,
            boost::python::return_value_policy<TfPySequenceToList>())

        .def("ComputeModelDrawMode", &UsdGeomModelAPI::ComputeModelDrawMode,
            (boost::python::arg("parentDrawMode")=TfToken()))
    ;
}

} // anonymous namespace 
