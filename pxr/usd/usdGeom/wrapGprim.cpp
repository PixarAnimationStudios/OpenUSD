//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/gprim.h"
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
_CreateDisplayColorAttr(UsdGeomGprim &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateDisplayColorAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateDisplayOpacityAttr(UsdGeomGprim &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateDisplayOpacityAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}
        
static UsdAttribute
_CreateDoubleSidedAttr(UsdGeomGprim &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateDoubleSidedAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateOrientationAttr(UsdGeomGprim &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateOrientationAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static std::string
_Repr(const UsdGeomGprim &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.Gprim(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdGeomGprim(nb::module_ &m)
{
    typedef UsdGeomGprim This;

    nb::class_<This, UsdGeomBoundable> cls(m, "Gprim");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

        .def_static("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        
        .def("GetDisplayColorAttr",
             &This::GetDisplayColorAttr)
        .def("CreateDisplayColorAttr",
             &_CreateDisplayColorAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetDisplayOpacityAttr",
             &This::GetDisplayOpacityAttr)
        .def("CreateDisplayOpacityAttr",
             &_CreateDisplayOpacityAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetDoubleSidedAttr",
             &This::GetDoubleSidedAttr)
        .def("CreateDoubleSidedAttr",
             &_CreateDoubleSidedAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetOrientationAttr",
             &This::GetOrientationAttr)
        .def("CreateOrientationAttr",
             &_CreateOrientationAttr,
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

WRAP_CUSTOM {
    _class
        .def("GetDisplayColorPrimvar", &UsdGeomGprim::GetDisplayColorPrimvar)
        .def("CreateDisplayColorPrimvar", 
             &UsdGeomGprim::CreateDisplayColorPrimvar,
             nb::arg("interpolation")=TfToken(), nb::arg("elementSize")=-1)
        .def("GetDisplayOpacityPrimvar",
             &UsdGeomGprim::GetDisplayOpacityPrimvar)
        .def("CreateDisplayOpacityPrimvar", 
             &UsdGeomGprim::CreateDisplayOpacityPrimvar,
             nb::arg("interpolation")=TfToken(), nb::arg("elementSize")=-1)
        ;
}

} // anonymous namespace 
