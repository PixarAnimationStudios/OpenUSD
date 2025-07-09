//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/gaussiansAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
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
_CreateGaussianShapeAttr(UsdLightFieldGaussiansAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateGaussianShapeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateSortingModeHintAttr(UsdLightFieldGaussiansAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateSortingModeHintAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateProjectionModeHintAttr(UsdLightFieldGaussiansAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateProjectionModeHintAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateOrientationsAttr(UsdLightFieldGaussiansAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateOrientationsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->QuathArray), writeSparsely);
}
        
static UsdAttribute
_CreateOrientationsfAttr(UsdLightFieldGaussiansAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateOrientationsfAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->QuatfArray), writeSparsely);
}
        
static UsdAttribute
_CreateScalesAttr(UsdLightFieldGaussiansAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateScalesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Half3Array), writeSparsely);
}
        
static UsdAttribute
_CreateScalesfAttr(UsdLightFieldGaussiansAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateScalesfAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3Array), writeSparsely);
}

static std::string
_Repr(const UsdLightFieldGaussiansAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdLightField.GaussiansAPI(%s)",
        primRepr.c_str());
}

struct UsdLightFieldGaussiansAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdLightFieldGaussiansAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdLightFieldGaussiansAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdLightFieldGaussiansAPI::CanApply(prim, &whyNot);
    return UsdLightFieldGaussiansAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdLightFieldGaussiansAPI()
{
    typedef UsdLightFieldGaussiansAPI This;

    UsdLightFieldGaussiansAPI_CanApplyResult::Wrap<UsdLightFieldGaussiansAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    class_<This, bases<UsdAPISchemaBase> >
        cls("GaussiansAPI");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("CanApply", &_WrapCanApply, (arg("prim")))
        .staticmethod("CanApply")

        .def("Apply", &This::Apply, (arg("prim")))
        .staticmethod("Apply")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetGaussianShapeAttr",
             &This::GetGaussianShapeAttr)
        .def("CreateGaussianShapeAttr",
             &_CreateGaussianShapeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetSortingModeHintAttr",
             &This::GetSortingModeHintAttr)
        .def("CreateSortingModeHintAttr",
             &_CreateSortingModeHintAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetProjectionModeHintAttr",
             &This::GetProjectionModeHintAttr)
        .def("CreateProjectionModeHintAttr",
             &_CreateProjectionModeHintAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetOrientationsAttr",
             &This::GetOrientationsAttr)
        .def("CreateOrientationsAttr",
             &_CreateOrientationsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetOrientationsfAttr",
             &This::GetOrientationsfAttr)
        .def("CreateOrientationsfAttr",
             &_CreateOrientationsfAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetScalesAttr",
             &This::GetScalesAttr)
        .def("CreateScalesAttr",
             &_CreateScalesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetScalesfAttr",
             &This::GetScalesfAttr)
        .def("CreateScalesfAttr",
             &_CreateScalesfAttr,
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

WRAP_CUSTOM {
}

}
