//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/modelAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
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
_CreateModelDrawModeAttr(UsdGeomModelAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateModelDrawModeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateModelApplyDrawModeAttr(UsdGeomModelAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateModelApplyDrawModeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateModelDrawModeColorAttr(UsdGeomModelAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateModelDrawModeColorAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3), writeSparsely);
}
        
static UsdAttribute
_CreateModelCardGeometryAttr(UsdGeomModelAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateModelCardGeometryAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateModelCardVisibilityAttr(UsdGeomModelAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateModelCardVisibilityAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateModelCardTextureXPosAttr(UsdGeomModelAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateModelCardTextureXPosAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateModelCardTextureYPosAttr(UsdGeomModelAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateModelCardTextureYPosAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateModelCardTextureZPosAttr(UsdGeomModelAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateModelCardTextureZPosAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateModelCardTextureXNegAttr(UsdGeomModelAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateModelCardTextureXNegAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateModelCardTextureYNegAttr(UsdGeomModelAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateModelCardTextureYNegAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateModelCardTextureZNegAttr(UsdGeomModelAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
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

void wrapUsdGeomModelAPI(nb::module_ &m)
{
    typedef UsdGeomModelAPI This;

    UsdGeomModelAPI_CanApplyResult::WrapNb<UsdGeomModelAPI_CanApplyResult>(m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase> cls(m, "ModelAPI");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

        .def_static("CanApply", &_WrapCanApply, nb::arg("prim"))

        .def_static("Apply", &This::Apply, nb::arg("prim"))

        .def_static("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        
        .def("GetModelDrawModeAttr",
             &This::GetModelDrawModeAttr)
        .def("CreateModelDrawModeAttr",
             &_CreateModelDrawModeAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetModelApplyDrawModeAttr",
             &This::GetModelApplyDrawModeAttr)
        .def("CreateModelApplyDrawModeAttr",
             &_CreateModelApplyDrawModeAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetModelDrawModeColorAttr",
             &This::GetModelDrawModeColorAttr)
        .def("CreateModelDrawModeColorAttr",
             &_CreateModelDrawModeColorAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetModelCardGeometryAttr",
             &This::GetModelCardGeometryAttr)
        .def("CreateModelCardGeometryAttr",
             &_CreateModelCardGeometryAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetModelCardVisibilityAttr",
             &This::GetModelCardVisibilityAttr)
        .def("CreateModelCardVisibilityAttr",
             &_CreateModelCardVisibilityAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetModelCardTextureXPosAttr",
             &This::GetModelCardTextureXPosAttr)
        .def("CreateModelCardTextureXPosAttr",
             &_CreateModelCardTextureXPosAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetModelCardTextureYPosAttr",
             &This::GetModelCardTextureYPosAttr)
        .def("CreateModelCardTextureYPosAttr",
             &_CreateModelCardTextureYPosAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetModelCardTextureZPosAttr",
             &This::GetModelCardTextureZPosAttr)
        .def("CreateModelCardTextureZPosAttr",
             &_CreateModelCardTextureZPosAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetModelCardTextureXNegAttr",
             &This::GetModelCardTextureXNegAttr)
        .def("CreateModelCardTextureXNegAttr",
             &_CreateModelCardTextureXNegAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetModelCardTextureYNegAttr",
             &This::GetModelCardTextureYNegAttr)
        .def("CreateModelCardTextureYNegAttr",
             &_CreateModelCardTextureYNegAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetModelCardTextureZNegAttr",
             &This::GetModelCardTextureZNegAttr)
        .def("CreateModelCardTextureZNegAttr",
             &_CreateModelCardTextureZNegAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

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

static nb::object
_GetExtentsHint(
        const UsdGeomModelAPI& self,
        const UsdTimeCode &time)
{
    VtVec3fArray extents;
    if (!self.GetExtentsHint(&extents, time)) {
        return nb::none();
    }

    return nb::cast(extents);
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
                (nb::arg("time")=UsdTimeCode::Default()))
        .def("SetExtentsHint", &_SetExtentsHint,
                nb::arg("extents"),
                 nb::arg("time")=UsdTimeCode::Default())
        .def("ComputeExtentsHint", &UsdGeomModelAPI::ComputeExtentsHint,
                nb::arg("bboxCache"))

        .def("GetExtentsHintAttr", &UsdGeomModelAPI::GetExtentsHintAttr)

        .def("GetConstraintTarget", &UsdGeomModelAPI::GetConstraintTarget)
        .def("CreateConstraintTarget", &UsdGeomModelAPI::CreateConstraintTarget)
        .def("GetConstraintTargets", &UsdGeomModelAPI::GetConstraintTargets,
             nb::rv_policy::copy)

        .def("ComputeModelDrawMode", &UsdGeomModelAPI::ComputeModelDrawMode,
            (nb::arg("parentDrawMode")=TfToken()))
    ;
}

} // anonymous namespace 
