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


static std::string
_Repr(const UsdGeomPrimvarsAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.PrimvarsAPI(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdGeomPrimvarsAPI()
{
    typedef UsdGeomPrimvarsAPI This;

    class_<This, bases<UsdAPISchemaBase> >
        cls("PrimvarsAPI");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)


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
        const SdfValueTypeName &typeName, const object &pyVal, 
        const TfToken &interpolation, int elementSize, UsdTimeCode time)
{
    VtValue val = UsdPythonToSdfType(pyVal, typeName);
    return self.CreateNonIndexedPrimvar(name, typeName, val, interpolation, 
            elementSize, time);
}

static UsdGeomPrimvar
_CreateIndexedPrimvar(const UsdGeomPrimvarsAPI &self, const TfToken &name, 
        const SdfValueTypeName &typeName, const object &pyVal, 
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
             (arg("name"), arg("typeName"), arg("interpolation")=TfToken(),
              arg("elementSize")=-1))
        .def("CreateNonIndexedPrimvar", _CreateNonIndexedPrimvar,
             (arg("name"), arg("typeName"), arg("value"),
              arg("interpolation")=TfToken(), arg("elementSize")=-1, 
              arg("time")=UsdTimeCode::Default()))
        .def("CreateIndexedPrimvar", _CreateIndexedPrimvar,
             (arg("name"), arg("typeName"), arg("value"), arg("indices"),
              arg("interpolation")=TfToken(), arg("elementSize")=-1, 
              arg("time")=UsdTimeCode::Default()))
        .def("RemovePrimvar", &UsdGeomPrimvarsAPI::RemovePrimvar,
             arg("name"))
        .def("BlockPrimvar", &UsdGeomPrimvarsAPI::BlockPrimvar,
             arg("name"))
        .def("GetPrimvar", &UsdGeomPrimvarsAPI::GetPrimvar, arg("name"))
        .def("GetPrimvars", &UsdGeomPrimvarsAPI::GetPrimvars,
             return_value_policy<TfPySequenceToList>())
        .def("GetAuthoredPrimvars", &UsdGeomPrimvarsAPI::GetAuthoredPrimvars,
             return_value_policy<TfPySequenceToList>())
        .def("GetPrimvarsWithValues", &UsdGeomPrimvarsAPI::GetPrimvarsWithValues,
             return_value_policy<TfPySequenceToList>())
        .def("GetPrimvarsWithAuthoredValues", 
             &UsdGeomPrimvarsAPI::GetPrimvarsWithAuthoredValues,
             return_value_policy<TfPySequenceToList>())
        .def("FindInheritablePrimvars", 
             &UsdGeomPrimvarsAPI::FindInheritablePrimvars,
             return_value_policy<TfPySequenceToList>())
        .def("FindIncrementallyInheritablePrimvars", 
             &UsdGeomPrimvarsAPI::FindIncrementallyInheritablePrimvars,
             (arg("inheritedFromAncestors")),
             return_value_policy<TfPySequenceToList>())
        .def("FindPrimvarWithInheritance", 
             (UsdGeomPrimvar (UsdGeomPrimvarsAPI::*)(const TfToken&) const)&UsdGeomPrimvarsAPI::FindPrimvarWithInheritance,
             (arg("name")))
        .def("FindPrimvarWithInheritance", 
             (UsdGeomPrimvar (UsdGeomPrimvarsAPI::*)(const TfToken&, const std::vector<UsdGeomPrimvar>&) const)&UsdGeomPrimvarsAPI::FindPrimvarWithInheritance,
             (arg("name"), arg("inheritedFromAncestors")))
        .def("FindPrimvarsWithInheritance", 
             (std::vector<UsdGeomPrimvar> (UsdGeomPrimvarsAPI::*)() const)&UsdGeomPrimvarsAPI::FindPrimvarsWithInheritance,
             return_value_policy<TfPySequenceToList>())
        .def("FindPrimvarsWithInheritance", 
             (std::vector<UsdGeomPrimvar> (UsdGeomPrimvarsAPI::*)(const std::vector<UsdGeomPrimvar>&) const)&UsdGeomPrimvarsAPI::FindPrimvarsWithInheritance,
             (arg("inheritedFromAncestors")),
             return_value_policy<TfPySequenceToList>())
        .def("HasPrimvar", &UsdGeomPrimvarsAPI::HasPrimvar, arg("name"))
        .def("HasPossiblyInheritedPrimvar", 
             &UsdGeomPrimvarsAPI::HasPossiblyInheritedPrimvar,
             arg("name"))
        .def("CanContainPropertyName", 
             &UsdGeomPrimvarsAPI::CanContainPropertyName, arg("name"))
        .staticmethod("CanContainPropertyName")
        ;
}

}
