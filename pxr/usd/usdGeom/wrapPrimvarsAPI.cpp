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
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
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

    boost::python::class_<This, boost::python::bases<UsdAPISchemaBase> >
        cls("PrimvarsAPI");

    cls
        .def(boost::python::init<UsdPrim>(boost::python::arg("prim")))
        .def(boost::python::init<UsdSchemaBase const&>(boost::python::arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Get")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             boost::python::arg("includeInherited")=true,
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!boost::python::self)


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
        const SdfValueTypeName &typeName, const boost::python::object &pyVal, 
        const TfToken &interpolation, int elementSize, UsdTimeCode time)
{
    VtValue val = UsdPythonToSdfType(pyVal, typeName);
    return self.CreateNonIndexedPrimvar(name, typeName, val, interpolation, 
            elementSize, time);
}

static UsdGeomPrimvar
_CreateIndexedPrimvar(const UsdGeomPrimvarsAPI &self, const TfToken &name, 
        const SdfValueTypeName &typeName, const boost::python::object &pyVal, 
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
             (boost::python::arg("name"), boost::python::arg("typeName"), boost::python::arg("interpolation")=TfToken(),
              boost::python::arg("elementSize")=-1))
        .def("CreateNonIndexedPrimvar", _CreateNonIndexedPrimvar,
             (boost::python::arg("name"), boost::python::arg("typeName"), boost::python::arg("value"),
              boost::python::arg("interpolation")=TfToken(), boost::python::arg("elementSize")=-1, 
              boost::python::arg("time")=UsdTimeCode::Default()))
        .def("CreateIndexedPrimvar", _CreateIndexedPrimvar,
             (boost::python::arg("name"), boost::python::arg("typeName"), boost::python::arg("value"), boost::python::arg("indices"),
              boost::python::arg("interpolation")=TfToken(), boost::python::arg("elementSize")=-1, 
              boost::python::arg("time")=UsdTimeCode::Default()))
        .def("RemovePrimvar", &UsdGeomPrimvarsAPI::RemovePrimvar,
             boost::python::arg("name"))
        .def("BlockPrimvar", &UsdGeomPrimvarsAPI::BlockPrimvar,
             boost::python::arg("name"))
        .def("GetPrimvar", &UsdGeomPrimvarsAPI::GetPrimvar, boost::python::arg("name"))
        .def("GetPrimvars", &UsdGeomPrimvarsAPI::GetPrimvars,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetAuthoredPrimvars", &UsdGeomPrimvarsAPI::GetAuthoredPrimvars,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetPrimvarsWithValues", &UsdGeomPrimvarsAPI::GetPrimvarsWithValues,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetPrimvarsWithAuthoredValues", 
             &UsdGeomPrimvarsAPI::GetPrimvarsWithAuthoredValues,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("FindInheritablePrimvars", 
             &UsdGeomPrimvarsAPI::FindInheritablePrimvars,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("FindIncrementallyInheritablePrimvars", 
             &UsdGeomPrimvarsAPI::FindIncrementallyInheritablePrimvars,
             (boost::python::arg("inheritedFromAncestors")),
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("FindPrimvarWithInheritance", 
             (UsdGeomPrimvar (UsdGeomPrimvarsAPI::*)(const TfToken&) const)&UsdGeomPrimvarsAPI::FindPrimvarWithInheritance,
             (boost::python::arg("name")))
        .def("FindPrimvarWithInheritance", 
             (UsdGeomPrimvar (UsdGeomPrimvarsAPI::*)(const TfToken&, const std::vector<UsdGeomPrimvar>&) const)&UsdGeomPrimvarsAPI::FindPrimvarWithInheritance,
             (boost::python::arg("name"), boost::python::arg("inheritedFromAncestors")))
        .def("FindPrimvarsWithInheritance", 
             (std::vector<UsdGeomPrimvar> (UsdGeomPrimvarsAPI::*)() const)&UsdGeomPrimvarsAPI::FindPrimvarsWithInheritance,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("FindPrimvarsWithInheritance", 
             (std::vector<UsdGeomPrimvar> (UsdGeomPrimvarsAPI::*)(const std::vector<UsdGeomPrimvar>&) const)&UsdGeomPrimvarsAPI::FindPrimvarsWithInheritance,
             (boost::python::arg("inheritedFromAncestors")),
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("HasPrimvar", &UsdGeomPrimvarsAPI::HasPrimvar, boost::python::arg("name"))
        .def("HasPossiblyInheritedPrimvar", 
             &UsdGeomPrimvarsAPI::HasPossiblyInheritedPrimvar,
             boost::python::arg("name"))
        .def("CanContainPropertyName", 
             &UsdGeomPrimvarsAPI::CanContainPropertyName, boost::python::arg("name"))
        .staticmethod("CanContainPropertyName")
        ;
}

}
