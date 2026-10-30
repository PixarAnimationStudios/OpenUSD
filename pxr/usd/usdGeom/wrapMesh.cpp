//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/mesh.h"
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
_CreateFaceVertexIndicesAttr(UsdGeomMesh &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateFaceVertexIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateFaceVertexCountsAttr(UsdGeomMesh &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateFaceVertexCountsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateSubdivisionSchemeAttr(UsdGeomMesh &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateSubdivisionSchemeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateInterpolateBoundaryAttr(UsdGeomMesh &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateInterpolateBoundaryAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateFaceVaryingLinearInterpolationAttr(UsdGeomMesh &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateFaceVaryingLinearInterpolationAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateTriangleSubdivisionRuleAttr(UsdGeomMesh &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateTriangleSubdivisionRuleAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateHoleIndicesAttr(UsdGeomMesh &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateHoleIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateCornerIndicesAttr(UsdGeomMesh &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateCornerIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateCornerSharpnessesAttr(UsdGeomMesh &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateCornerSharpnessesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}
        
static UsdAttribute
_CreateCreaseIndicesAttr(UsdGeomMesh &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateCreaseIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateCreaseLengthsAttr(UsdGeomMesh &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateCreaseLengthsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateCreaseSharpnessesAttr(UsdGeomMesh &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateCreaseSharpnessesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}

static std::string
_Repr(const UsdGeomMesh &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.Mesh(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdGeomMesh(nb::module_ &m)
{
    typedef UsdGeomMesh This;

    nb::class_<This, UsdGeomPointBased> cls(m, "Mesh");

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

        
        .def("GetFaceVertexIndicesAttr",
             &This::GetFaceVertexIndicesAttr)
        .def("CreateFaceVertexIndicesAttr",
             &_CreateFaceVertexIndicesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetFaceVertexCountsAttr",
             &This::GetFaceVertexCountsAttr)
        .def("CreateFaceVertexCountsAttr",
             &_CreateFaceVertexCountsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetSubdivisionSchemeAttr",
             &This::GetSubdivisionSchemeAttr)
        .def("CreateSubdivisionSchemeAttr",
             &_CreateSubdivisionSchemeAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetInterpolateBoundaryAttr",
             &This::GetInterpolateBoundaryAttr)
        .def("CreateInterpolateBoundaryAttr",
             &_CreateInterpolateBoundaryAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetFaceVaryingLinearInterpolationAttr",
             &This::GetFaceVaryingLinearInterpolationAttr)
        .def("CreateFaceVaryingLinearInterpolationAttr",
             &_CreateFaceVaryingLinearInterpolationAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetTriangleSubdivisionRuleAttr",
             &This::GetTriangleSubdivisionRuleAttr)
        .def("CreateTriangleSubdivisionRuleAttr",
             &_CreateTriangleSubdivisionRuleAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetHoleIndicesAttr",
             &This::GetHoleIndicesAttr)
        .def("CreateHoleIndicesAttr",
             &_CreateHoleIndicesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetCornerIndicesAttr",
             &This::GetCornerIndicesAttr)
        .def("CreateCornerIndicesAttr",
             &_CreateCornerIndicesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetCornerSharpnessesAttr",
             &This::GetCornerSharpnessesAttr)
        .def("CreateCornerSharpnessesAttr",
             &_CreateCornerSharpnessesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetCreaseIndicesAttr",
             &This::GetCreaseIndicesAttr)
        .def("CreateCreaseIndicesAttr",
             &_CreateCreaseIndicesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetCreaseLengthsAttr",
             &This::GetCreaseLengthsAttr)
        .def("CreateCreaseLengthsAttr",
             &_CreateCreaseLengthsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetCreaseSharpnessesAttr",
             &This::GetCreaseSharpnessesAttr)
        .def("CreateCreaseSharpnessesAttr",
             &_CreateCreaseSharpnessesAttr,
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

nb::tuple
_ValidateTopology(const VtIntArray& faceVertexIndices,
                  const VtIntArray& faceVertexCounts,
                  size_t numPoints)
{
    std::string reason;
    bool valid = UsdGeomMesh::ValidateTopology(faceVertexIndices,
                                               faceVertexCounts,
                                               numPoints, &reason);
    return nb::make_tuple(valid, reason);
}

WRAP_CUSTOM {
    typedef UsdGeomMesh This;

    _class
        .def_static("ValidateTopology", &_ValidateTopology,
             nb::arg("faceVertexIndices"),
              nb::arg("faceVertexCounts"),
              nb::arg("numPoints"))
        .def("GetFaceCount", &UsdGeomMesh::GetFaceCount,
            nb::arg("timeCode")=UsdTimeCode::Default());

    _class.attr("SHARPNESS_INFINITE") = UsdGeomMesh::SHARPNESS_INFINITE;
    ;
}

} // anonymous namespace 
