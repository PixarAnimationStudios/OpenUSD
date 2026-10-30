//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/hermiteCurves.h"
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
_CreateTangentsAttr(UsdGeomHermiteCurves &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateTangentsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3fArray), writeSparsely);
}

static std::string
_Repr(const UsdGeomHermiteCurves &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.HermiteCurves(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdGeomHermiteCurves(nb::module_ &m)
{
    typedef UsdGeomHermiteCurves This;

    nb::class_<This, UsdGeomCurves> cls(m, "HermiteCurves");

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

        
        .def("GetTangentsAttr",
             &This::GetTangentsAttr)
        .def("CreateTangentsAttr",
             &_CreateTangentsAttr,
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

static std::string _PointAndTangentsRepr(
    const UsdGeomHermiteCurves::PointAndTangentArrays &arrays) {
    return TfStringPrintf("UsdGeom.HermiteCurves(%s, %s)",
                          TfPyRepr(arrays.GetPoints()).c_str(),
                          TfPyRepr(arrays.GetTangents()).c_str());
}

WRAP_CUSTOM {
    typedef UsdGeomHermiteCurves This;
    {
        typedef UsdGeomHermiteCurves::PointAndTangentArrays ThisStruct;
        nb::class_<ThisStruct>(_class, "PointAndTangentArrays")
            .def(nb::init<>())
            .def(nb::init<const VtVec3fArray&,const VtVec3fArray&>())
            .def("GetPoints", &ThisStruct::GetPoints,
                 nb::rv_policy::copy)
            .def("GetTangents", &ThisStruct::GetTangents,
                 nb::rv_policy::copy)
            .def("IsEmpty", &ThisStruct::IsEmpty)
            .def("Interleave", &ThisStruct::Interleave)
            .def_static("Separate", &ThisStruct::Separate)
            .def("__repr__", &::_PointAndTangentsRepr)
            .def("__bool__", [](const ThisStruct &self) { return bool(self); })
            .def("__eq__", [](ThisStruct &self, const ThisStruct &rhs) { return self == rhs; }, nb::is_operator())
            .def("__ne__", [](ThisStruct &self, const ThisStruct &rhs) { return self != rhs; }, nb::is_operator());
    }
}

}
