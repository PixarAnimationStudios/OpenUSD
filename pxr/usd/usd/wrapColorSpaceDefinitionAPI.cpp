//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usd/colorSpaceDefinitionAPI.h"
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
_CreateNameAttr(UsdColorSpaceDefinitionAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateNameAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateRedChromaAttr(UsdColorSpaceDefinitionAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRedChromaAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float2), writeSparsely);
}
        
static UsdAttribute
_CreateGreenChromaAttr(UsdColorSpaceDefinitionAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateGreenChromaAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float2), writeSparsely);
}
        
static UsdAttribute
_CreateBlueChromaAttr(UsdColorSpaceDefinitionAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateBlueChromaAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float2), writeSparsely);
}
        
static UsdAttribute
_CreateWhitePointAttr(UsdColorSpaceDefinitionAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateWhitePointAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float2), writeSparsely);
}
        
static UsdAttribute
_CreateGammaAttr(UsdColorSpaceDefinitionAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateGammaAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateLinearBiasAttr(UsdColorSpaceDefinitionAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateLinearBiasAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}

static bool _WrapIsColorSpaceDefinitionAPIPath(const SdfPath &path) {
    TfToken collectionName;
    return UsdColorSpaceDefinitionAPI::IsColorSpaceDefinitionAPIPath(
        path, &collectionName);
}

static std::string
_Repr(const UsdColorSpaceDefinitionAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    std::string instanceName = TfPyRepr(self.GetName());
    return TfStringPrintf(
        "Usd.ColorSpaceDefinitionAPI(%s, '%s')",
        primRepr.c_str(), instanceName.c_str());
}

struct UsdColorSpaceDefinitionAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdColorSpaceDefinitionAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdColorSpaceDefinitionAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim, const TfToken& name)
{
    std::string whyNot;
    bool result = UsdColorSpaceDefinitionAPI::CanApply(prim, name, &whyNot);
    return UsdColorSpaceDefinitionAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdColorSpaceDefinitionAPI()
{
    typedef UsdColorSpaceDefinitionAPI This;

    UsdColorSpaceDefinitionAPI_CanApplyResult::Wrap<UsdColorSpaceDefinitionAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    class_<This, bases<UsdAPISchemaBase> >
        cls("ColorSpaceDefinitionAPI");

    cls
        .def(init<UsdPrim, TfToken>((arg("prim"), arg("name"))))
        .def(init<UsdSchemaBase const&, TfToken>((arg("schemaObj"), arg("name"))))
        .def(TfTypePythonClass())

        .def("Get",
            (UsdColorSpaceDefinitionAPI(*)(const UsdStagePtr &stage, 
                                       const SdfPath &path))
               &This::Get,
            (arg("stage"), arg("path")))
        .def("Get",
            (UsdColorSpaceDefinitionAPI(*)(const UsdPrim &prim,
                                       const TfToken &name))
               &This::Get,
            (arg("prim"), arg("name")))
        .staticmethod("Get")

        .def("GetAll",
            (std::vector<UsdColorSpaceDefinitionAPI>(*)(const UsdPrim &prim))
                &This::GetAll,
            arg("prim"),
            return_value_policy<TfPySequenceToList>())
        .staticmethod("GetAll")

        .def("CanApply", &_WrapCanApply, (arg("prim"), arg("name")))
        .staticmethod("CanApply")

        .def("Apply", &This::Apply, (arg("prim"), arg("name")))
        .staticmethod("Apply")

        .def("GetSchemaAttributeNames",
             (const TfTokenVector &(*)(bool))&This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .def("GetSchemaAttributeNames",
             (TfTokenVector(*)(bool, const TfToken &))
                &This::GetSchemaAttributeNames,
             arg("includeInherited"),
             arg("instanceName"),
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetNameAttr",
             &This::GetNameAttr)
        .def("CreateNameAttr",
             &_CreateNameAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRedChromaAttr",
             &This::GetRedChromaAttr)
        .def("CreateRedChromaAttr",
             &_CreateRedChromaAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetGreenChromaAttr",
             &This::GetGreenChromaAttr)
        .def("CreateGreenChromaAttr",
             &_CreateGreenChromaAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetBlueChromaAttr",
             &This::GetBlueChromaAttr)
        .def("CreateBlueChromaAttr",
             &_CreateBlueChromaAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetWhitePointAttr",
             &This::GetWhitePointAttr)
        .def("CreateWhitePointAttr",
             &_CreateWhitePointAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetGammaAttr",
             &This::GetGammaAttr)
        .def("CreateGammaAttr",
             &_CreateGammaAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetLinearBiasAttr",
             &This::GetLinearBiasAttr)
        .def("CreateLinearBiasAttr",
             &_CreateLinearBiasAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        .def("IsColorSpaceDefinitionAPIPath", _WrapIsColorSpaceDefinitionAPIPath)
            .staticmethod("IsColorSpaceDefinitionAPIPath")
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
    _class
        .def("CreateColorSpaceAttrsWithChroma",
             &UsdColorSpaceDefinitionAPI::CreateColorSpaceAttrsWithChroma,
             (arg("redChroma"), arg("greenChroma"), arg("blueChroma"),
              arg("whitePoint"), arg("gamma"), arg("linearBias")))
        .def("CreateColorSpaceAttrsWithMatrix",
             &UsdColorSpaceDefinitionAPI::CreateColorSpaceAttrsWithMatrix,
             (arg("rgbToXYZ"), arg("gamma"), arg("linearBias")))
        .def("ComputeColorSpaceFromDefinitionAttributes",
             &UsdColorSpaceDefinitionAPI::ComputeColorSpaceFromDefinitionAttributes)
    ;
}

}
