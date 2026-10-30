//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/tetMesh.h"
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
_CreateTetVertexIndicesAttr(UsdGeomTetMesh &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateTetVertexIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int4Array), writeSparsely);
}
        
static UsdAttribute
_CreateSurfaceFaceVertexIndicesAttr(UsdGeomTetMesh &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateSurfaceFaceVertexIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int3Array), writeSparsely);
}

static std::string
_Repr(const UsdGeomTetMesh &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.TetMesh(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdGeomTetMesh(nb::module_ &m)
{
    typedef UsdGeomTetMesh This;

    nb::class_<This, UsdGeomPointBased> cls(m, "TetMesh");

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

        
        .def("GetTetVertexIndicesAttr",
             &This::GetTetVertexIndicesAttr)
        .def("CreateTetVertexIndicesAttr",
             &_CreateTetVertexIndicesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetSurfaceFaceVertexIndicesAttr",
             &This::GetSurfaceFaceVertexIndicesAttr)
        .def("CreateSurfaceFaceVertexIndicesAttr",
             &_CreateSurfaceFaceVertexIndicesAttr,
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
  
    static VtVec3iArray
    _ComputeSurfaceFacesHelper(const UsdGeomTetMesh &tetMesh, 
                               const UsdTimeCode timeCode = UsdTimeCode::Default())
    {           
        VtVec3iArray surfaceFacesArray; 
        UsdGeomTetMesh::ComputeSurfaceFaces(tetMesh, &surfaceFacesArray, timeCode);  
        return surfaceFacesArray;      
    }

    static VtIntArray
    _FindInvertedElementsHelper(const UsdGeomTetMesh &tetMesh, 
                                const UsdTimeCode timeCode = UsdTimeCode::Default())
    {           
        VtIntArray invertedElements; 
        UsdGeomTetMesh::FindInvertedElements(tetMesh, &invertedElements, timeCode);  
        return invertedElements;      
    }

WRAP_CUSTOM {
    _class
         .def_static("ComputeSurfaceFaces", _ComputeSurfaceFacesHelper, 
             nb::arg("tetMesh"), nb::arg("timeCode")=UsdTimeCode::Default())
         .def_static("FindInvertedElements", _FindInvertedElementsHelper, 
             nb::arg("tetMesh"), nb::arg("timeCode")=UsdTimeCode::Default())
     ;
}

}
