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
#include "pxr/usd/usdLux/shapingAPI.h"
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

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateShapingFocusAttr(UsdLuxShapingAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateShapingFocusAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateShapingFocusTintAttr(UsdLuxShapingAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateShapingFocusTintAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color3f), writeSparsely);
}
        
static UsdAttribute
_CreateShapingConeAngleAttr(UsdLuxShapingAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateShapingConeAngleAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateShapingConeSoftnessAttr(UsdLuxShapingAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateShapingConeSoftnessAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateShapingIesFileAttr(UsdLuxShapingAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateShapingIesFileAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateShapingIesAngleScaleAttr(UsdLuxShapingAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateShapingIesAngleScaleAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateShapingIesNormalizeAttr(UsdLuxShapingAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateShapingIesNormalizeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}

static std::string
_Repr(const UsdLuxShapingAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdLux.ShapingAPI(%s)",
        primRepr.c_str());
}

struct UsdLuxShapingAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdLuxShapingAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdLuxShapingAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdLuxShapingAPI::CanApply(prim, &whyNot);
    return UsdLuxShapingAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdLuxShapingAPI()
{
    typedef UsdLuxShapingAPI This;

    UsdLuxShapingAPI_CanApplyResult::Wrap<UsdLuxShapingAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    boost::python::class_<This, boost::python::bases<UsdAPISchemaBase> >
        cls("ShapingAPI");

    cls
        .def(boost::python::init<UsdPrim>(boost::python::arg("prim")))
        .def(boost::python::init<UsdSchemaBase const&>(boost::python::arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Get")

        .def("CanApply", &_WrapCanApply, (boost::python::arg("prim")))
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

        
        .def("GetShapingFocusAttr",
             &This::GetShapingFocusAttr)
        .def("CreateShapingFocusAttr",
             &_CreateShapingFocusAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetShapingFocusTintAttr",
             &This::GetShapingFocusTintAttr)
        .def("CreateShapingFocusTintAttr",
             &_CreateShapingFocusTintAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetShapingConeAngleAttr",
             &This::GetShapingConeAngleAttr)
        .def("CreateShapingConeAngleAttr",
             &_CreateShapingConeAngleAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetShapingConeSoftnessAttr",
             &This::GetShapingConeSoftnessAttr)
        .def("CreateShapingConeSoftnessAttr",
             &_CreateShapingConeSoftnessAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetShapingIesFileAttr",
             &This::GetShapingIesFileAttr)
        .def("CreateShapingIesFileAttr",
             &_CreateShapingIesFileAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetShapingIesAngleScaleAttr",
             &This::GetShapingIesAngleScaleAttr)
        .def("CreateShapingIesAngleScaleAttr",
             &_CreateShapingIesAngleScaleAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetShapingIesNormalizeAttr",
             &This::GetShapingIesNormalizeAttr)
        .def("CreateShapingIesNormalizeAttr",
             &_CreateShapingIesNormalizeAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))

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
        .def(boost::python::init<UsdShadeConnectableAPI>(boost::python::arg("connectable")))
        .def("ConnectableAPI", &UsdLuxShapingAPI::ConnectableAPI)

        .def("CreateOutput", &UsdLuxShapingAPI::CreateOutput,
             (boost::python::arg("name"), boost::python::arg("type")))
        .def("GetOutput", &UsdLuxShapingAPI::GetOutput, boost::python::arg("name"))
        .def("GetOutputs", &UsdLuxShapingAPI::GetOutputs,
             (boost::python::arg("onlyAuthored")=true),
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("CreateInput", &UsdLuxShapingAPI::CreateInput,
             (boost::python::arg("name"), boost::python::arg("type")))
        .def("GetInput", &UsdLuxShapingAPI::GetInput, boost::python::arg("name"))
        .def("GetInputs", &UsdLuxShapingAPI::GetInputs,
             (boost::python::arg("onlyAuthored")=true),
             boost::python::return_value_policy<TfPySequenceToList>())
        ;
}

}
