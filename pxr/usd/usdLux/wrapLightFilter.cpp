//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLux/lightFilter.h"
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
_CreateShaderIdAttr(UsdLuxLightFilter &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateShaderIdAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static std::string
_Repr(const UsdLuxLightFilter &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdLux.LightFilter(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdLuxLightFilter(nb::module_ &m)
{
    typedef UsdLuxLightFilter This;

    nb::class_<This, UsdGeomXformable> cls(m, "LightFilter");

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

        
        .def("GetShaderIdAttr",
             &This::GetShaderIdAttr)
        .def("CreateShaderIdAttr",
             &_CreateShaderIdAttr,
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

#include "pxr/usd/usdShade/connectableAPI.h"

static UsdAttribute
_CreateShaderIdAttrForRenderContext(
    UsdLuxLightFilter &self, 
    const TfToken &renderContext,
    nb::handle defaultVal, 
    bool writeSparsely) 
{
    return self.CreateShaderIdAttrForRenderContext(
        renderContext,
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), 
        writeSparsely);
}

namespace {

WRAP_CUSTOM {
    _class
        .def(nb::init<UsdShadeConnectableAPI>(), nb::arg("connectable"))
        .def("ConnectableAPI", &UsdLuxLightFilter::ConnectableAPI)

        .def("CreateOutput", &UsdLuxLightFilter::CreateOutput,
             nb::arg("name"), nb::arg("type"))
        .def("GetOutput", &UsdLuxLightFilter::GetOutput, nb::arg("name"))
        .def("GetOutputs", &UsdLuxLightFilter::GetOutputs,
             (nb::arg("onlyAuthored")=true),
             nb::rv_policy::copy)

        .def("CreateInput", &UsdLuxLightFilter::CreateInput,
             nb::arg("name"), nb::arg("type"))
        .def("GetInput", &UsdLuxLightFilter::GetInput, nb::arg("name"))
        .def("GetInputs", &UsdLuxLightFilter::GetInputs,
             (nb::arg("onlyAuthored")=true),
             nb::rv_policy::copy)

        .def("GetFilterLinkCollectionAPI",
             &UsdLuxLightFilter::GetFilterLinkCollectionAPI)

        .def("GetShaderIdAttrForRenderContext",
             &UsdLuxLightFilter::GetShaderIdAttrForRenderContext, 
             nb::arg("renderContext"))
        .def("CreateShaderIdAttrForRenderContext",
             &_CreateShaderIdAttrForRenderContext,
             nb::arg("renderContext"),
              nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        .def("GetShaderId", 
             &UsdLuxLightFilter::GetShaderId, nb::arg("renderContexts"))
        ;
}

}
