//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdRender/settings.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
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
_CreateIncludedPurposesAttr(UsdRenderSettings &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateIncludedPurposesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->TokenArray), writeSparsely);
}
        
static UsdAttribute
_CreateMaterialBindingPurposesAttr(UsdRenderSettings &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMaterialBindingPurposesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->TokenArray), writeSparsely);
}
        
static UsdAttribute
_CreateRenderingColorSpaceAttr(UsdRenderSettings &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRenderingColorSpaceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static std::string
_Repr(const UsdRenderSettings &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdRender.Settings(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdRenderSettings()
{
    typedef UsdRenderSettings This;

    class_<This, bases<UsdRenderSettingsBase> >
        cls("Settings");

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

        
        .def("GetIncludedPurposesAttr",
             &This::GetIncludedPurposesAttr)
        .def("CreateIncludedPurposesAttr",
             &_CreateIncludedPurposesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMaterialBindingPurposesAttr",
             &This::GetMaterialBindingPurposesAttr)
        .def("CreateMaterialBindingPurposesAttr",
             &_CreateMaterialBindingPurposesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRenderingColorSpaceAttr",
             &This::GetRenderingColorSpaceAttr)
        .def("CreateRenderingColorSpaceAttr",
             &_CreateRenderingColorSpaceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        
        .def("GetProductsRel",
             &This::GetProductsRel)
        .def("CreateProductsRel",
             &This::CreateProductsRel)
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
        .def("GetStageRenderSettings",
             &UsdRenderSettings::GetStageRenderSettings)
        .staticmethod("GetStageRenderSettings")
        ;
}

}
