//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLux/shadowAPI.h"
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
_CreateShadowEnableAttr(UsdLuxShadowAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateShadowEnableAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateShadowColorAttr(UsdLuxShadowAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateShadowColorAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color3f), writeSparsely);
}
        
static UsdAttribute
_CreateShadowDistanceAttr(UsdLuxShadowAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateShadowDistanceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateShadowFalloffAttr(UsdLuxShadowAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateShadowFalloffAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateShadowFalloffGammaAttr(UsdLuxShadowAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateShadowFalloffGammaAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}

static std::string
_Repr(const UsdLuxShadowAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdLux.ShadowAPI(%s)",
        primRepr.c_str());
}

struct UsdLuxShadowAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdLuxShadowAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdLuxShadowAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdLuxShadowAPI::CanApply(prim, &whyNot);
    return UsdLuxShadowAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdLuxShadowAPI()
{
    typedef UsdLuxShadowAPI This;

    UsdLuxShadowAPI_CanApplyResult::Wrap<UsdLuxShadowAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    class_<This, bases<UsdAPISchemaBase> >
        cls("ShadowAPI");

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

        
        .def("GetShadowEnableAttr",
             &This::GetShadowEnableAttr)
        .def("CreateShadowEnableAttr",
             &_CreateShadowEnableAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetShadowColorAttr",
             &This::GetShadowColorAttr)
        .def("CreateShadowColorAttr",
             &_CreateShadowColorAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetShadowDistanceAttr",
             &This::GetShadowDistanceAttr)
        .def("CreateShadowDistanceAttr",
             &_CreateShadowDistanceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetShadowFalloffAttr",
             &This::GetShadowFalloffAttr)
        .def("CreateShadowFalloffAttr",
             &_CreateShadowFalloffAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetShadowFalloffGammaAttr",
             &This::GetShadowFalloffGammaAttr)
        .def("CreateShadowFalloffGammaAttr",
             &_CreateShadowFalloffGammaAttr,
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

#include "pxr/usd/usdShade/connectableAPI.h"

namespace {

WRAP_CUSTOM {
    _class
        .def(init<UsdShadeConnectableAPI>(arg("connectable")))
        .def("ConnectableAPI", &UsdLuxShadowAPI::ConnectableAPI)

        .def("CreateOutput", &UsdLuxShadowAPI::CreateOutput,
             (arg("name"), arg("type")))
        .def("GetOutput", &UsdLuxShadowAPI::GetOutput, arg("name"))
        .def("GetOutputs", &UsdLuxShadowAPI::GetOutputs,
             (arg("onlyAuthored")=true),
             return_value_policy<TfPySequenceToList>())

        .def("CreateInput", &UsdLuxShadowAPI::CreateInput,
             (arg("name"), arg("type")))
        .def("GetInput", &UsdLuxShadowAPI::GetInput, arg("name"))
        .def("GetInputs", &UsdLuxShadowAPI::GetInputs,
             (arg("onlyAuthored")=true),
             return_value_policy<TfPySequenceToList>())
        ;
}

}
