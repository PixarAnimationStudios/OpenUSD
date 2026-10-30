//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/subset.h"
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
_CreateElementTypeAttr(UsdGeomSubset &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateElementTypeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateIndicesAttr(UsdGeomSubset &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateFamilyNameAttr(UsdGeomSubset &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateFamilyNameAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static std::string
_Repr(const UsdGeomSubset &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.Subset(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdGeomSubset(nb::module_ &m)
{
    typedef UsdGeomSubset This;

    nb::class_<This, UsdTyped> cls(m, "Subset");

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

        
        .def("GetElementTypeAttr",
             &This::GetElementTypeAttr)
        .def("CreateElementTypeAttr",
             &_CreateElementTypeAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetIndicesAttr",
             &This::GetIndicesAttr)
        .def("CreateIndicesAttr",
             &_CreateIndicesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetFamilyNameAttr",
             &This::GetFamilyNameAttr)
        .def("CreateFamilyNameAttr",
             &_CreateFamilyNameAttr,
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

static nb::tuple
_WrapValidateFamily(const UsdGeomImageable &geom, 
               const TfToken &elementType,
               const TfToken &familyName)
{
    std::string reason;
    bool valid = UsdGeomSubset::ValidateFamily(geom, elementType, 
        familyName, &reason);
    return nb::make_tuple(valid, reason);
}

static nb::tuple
_WrapValidateSubsets(    const std::vector<UsdGeomSubset> &subsets,
    const size_t elementCount, const TfToken &familyType)
{
    std::string reason;
    bool valid = UsdGeomSubset::ValidateSubsets(subsets, elementCount, 
        familyType, &reason);
    return nb::make_tuple(valid, reason);
}

WRAP_CUSTOM {
    typedef UsdGeomSubset This;

    // Register to and from vector conversions.

    TfPyContainerConversions::from_python_sequence<std::vector<This>,
        TfPyContainerConversions::variable_capacity_policy >();

    _class
        .def_static("CreateGeomSubset", &This::CreateGeomSubset,
            nb::arg("geom"), nb::arg("subsetName"), nb::arg("elementType"), 
             nb::arg("indices"), nb::arg("familyName")=TfToken(),
             nb::arg("familyType")=TfToken())

        .def_static("CreateUniqueGeomSubset", &This::CreateUniqueGeomSubset,
            nb::arg("geom"), nb::arg("subsetName"), nb::arg("elementType"), 
             nb::arg("indices"), nb::arg("familyName")=TfToken(),
             nb::arg("familyType")=TfToken())

        .def_static("GetAllGeomSubsets", &This::GetAllGeomSubsets,
            nb::arg("geom"),
             nb::rv_policy::copy)

        .def_static("GetGeomSubsets", &This::GetGeomSubsets,
            nb::arg("geom"), 
             nb::arg("elementType")=TfToken(), 
             nb::arg("familyName")=TfToken(),
             nb::rv_policy::copy)

        .def_static("GetAllGeomSubsetFamilyNames", &This::GetAllGeomSubsetFamilyNames,
            nb::arg("geom"),
             nb::rv_policy::copy)

        .def_static("SetFamilyType", &This::SetFamilyType,
            nb::arg("geom"), nb::arg("familyName"), nb::arg("familyType"))
        
        .def_static("GetFamilyType", &This::GetFamilyType,
            nb::arg("geom"), nb::arg("familyName"))

        .def_static("GetUnassignedIndices", (VtIntArray(*)(
            const UsdGeomImageable &geom, 
            const TfToken &elementType,
            const TfToken &familyName,
            const UsdTimeCode &time))&This::GetUnassignedIndices,
            nb::arg("geom"), nb::arg("elementType"), nb::arg("familyName"),
             nb::arg("time")=UsdTimeCode::EarliestTime())
        .def_static("GetUnassignedIndices", (VtIntArray(*)(
            const std::vector<UsdGeomSubset> &subsets,
            const size_t elementCount,
            const UsdTimeCode &time))&This::GetUnassignedIndices,
            nb::arg("subsets"), nb::arg("elementCount"), 
             nb::arg("time")=UsdTimeCode::EarliestTime())

        .def_static("ValidateFamily", _WrapValidateFamily, 
            nb::arg("geom"), 
             nb::arg("elementType")=TfToken(), 
             nb::arg("familyName")=TfToken())

        .def_static("ValidateSubsets", _WrapValidateSubsets, 
            nb::arg("subsets"), 
             nb::arg("elementCount"),
             nb::arg("familyType"))

        ;

}

}
