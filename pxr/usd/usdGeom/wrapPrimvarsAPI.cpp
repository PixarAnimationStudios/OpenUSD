//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/primvarsAPI.h"
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

static std::string
_Repr(const UsdGeomPrimvarsAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.PrimvarsAPI(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdGeomPrimvarsAPI(nb::module_ &m)
{
    typedef UsdGeomPrimvarsAPI This;

    nb::class_<This, UsdAPISchemaBase> cls(m, "PrimvarsAPI");

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

static UsdGeomPrimvar
_CreateNonIndexedPrimvar(const UsdGeomPrimvarsAPI &self, const TfToken &name, 
        const SdfValueTypeName &typeName, TfPyObjWrapper pyVal, 
        const TfToken &interpolation, int elementSize, UsdTimeCode time)
{
    VtValue val = UsdPythonToSdfType(pyVal, typeName);
    return self.CreateNonIndexedPrimvar(name, typeName, val, interpolation, 
            elementSize, time);
}

static UsdGeomPrimvar
_CreateIndexedPrimvar(const UsdGeomPrimvarsAPI &self, const TfToken &name, 
        const SdfValueTypeName &typeName, TfPyObjWrapper pyVal, 
        const VtIntArray &indices, const TfToken &interpolation, 
        int elementSize, UsdTimeCode time)
{
    VtValue val = UsdPythonToSdfType(pyVal, typeName);
    return self.CreateIndexedPrimvar(name, typeName, val, indices, 
            interpolation, elementSize, time);
}

WRAP_CUSTOM {
    _class
        .def("CreatePrimvar", &UsdGeomPrimvarsAPI::CreatePrimvar,
             nb::arg("name"), nb::arg("typeName"), nb::arg("interpolation")=TfToken(),
              nb::arg("elementSize")=-1)
        .def("CreateNonIndexedPrimvar", _CreateNonIndexedPrimvar,
             nb::arg("name"), nb::arg("typeName"), nb::arg("value"),
              nb::arg("interpolation")=TfToken(), nb::arg("elementSize")=-1, 
              nb::arg("time")=UsdTimeCode::Default())
        .def("CreateIndexedPrimvar", _CreateIndexedPrimvar,
             nb::arg("name"), nb::arg("typeName"), nb::arg("value"), nb::arg("indices"),
              nb::arg("interpolation")=TfToken(), nb::arg("elementSize")=-1, 
              nb::arg("time")=UsdTimeCode::Default())
        .def("RemovePrimvar", &UsdGeomPrimvarsAPI::RemovePrimvar,
             nb::arg("name"))
        .def("BlockPrimvar", &UsdGeomPrimvarsAPI::BlockPrimvar,
             nb::arg("name"))
        .def("GetPrimvar", &UsdGeomPrimvarsAPI::GetPrimvar, nb::arg("name"))
        .def("GetPrimvars", &UsdGeomPrimvarsAPI::GetPrimvars,
             nb::rv_policy::copy)
        .def("GetAuthoredPrimvars", &UsdGeomPrimvarsAPI::GetAuthoredPrimvars,
             nb::rv_policy::copy)
        .def("GetPrimvarsWithValues", &UsdGeomPrimvarsAPI::GetPrimvarsWithValues,
             nb::rv_policy::copy)
        .def("GetPrimvarsWithAuthoredValues", 
             &UsdGeomPrimvarsAPI::GetPrimvarsWithAuthoredValues,
             nb::rv_policy::copy)
        .def("FindInheritablePrimvars", 
             &UsdGeomPrimvarsAPI::FindInheritablePrimvars,
             nb::rv_policy::copy)
        .def("FindIncrementallyInheritablePrimvars", 
             &UsdGeomPrimvarsAPI::FindIncrementallyInheritablePrimvars,
             nb::arg("inheritedFromAncestors"),
             nb::rv_policy::copy)
        .def("FindPrimvarWithInheritance", 
             (UsdGeomPrimvar (UsdGeomPrimvarsAPI::*)(const TfToken&) const)&UsdGeomPrimvarsAPI::FindPrimvarWithInheritance,
             nb::arg("name"))
        .def("FindPrimvarWithInheritance", 
             (UsdGeomPrimvar (UsdGeomPrimvarsAPI::*)(const TfToken&, const std::vector<UsdGeomPrimvar>&) const)&UsdGeomPrimvarsAPI::FindPrimvarWithInheritance,
             nb::arg("name"), nb::arg("inheritedFromAncestors"))
        .def("FindPrimvarsWithInheritance", 
             (std::vector<UsdGeomPrimvar> (UsdGeomPrimvarsAPI::*)() const)&UsdGeomPrimvarsAPI::FindPrimvarsWithInheritance,
             nb::rv_policy::copy)
        .def("FindPrimvarsWithInheritance", 
             (std::vector<UsdGeomPrimvar> (UsdGeomPrimvarsAPI::*)(const std::vector<UsdGeomPrimvar>&) const)&UsdGeomPrimvarsAPI::FindPrimvarsWithInheritance,
             nb::arg("inheritedFromAncestors"),
             nb::rv_policy::copy)
        .def("HasPrimvar", &UsdGeomPrimvarsAPI::HasPrimvar, nb::arg("name"))
        .def("HasPossiblyInheritedPrimvar", 
             &UsdGeomPrimvarsAPI::HasPossiblyInheritedPrimvar,
             nb::arg("name"))
        .def_static("CanContainPropertyName", 
             &UsdGeomPrimvarsAPI::CanContainPropertyName, nb::arg("name"))
        ;
}

}
