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

        
static UsdAttribute
_CreateElementTypeAttr(UsdGeomSubset &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateElementTypeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateIndicesAttr(UsdGeomSubset &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateFamilyNameAttr(UsdGeomSubset &self,
                                      object defaultVal, bool writeSparsely) {
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

void wrapUsdGeomSubset()
{
    typedef UsdGeomSubset This;

    class_<This, bases<UsdTyped> >
        cls("Subset");

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

        
        .def("GetElementTypeAttr",
             &This::GetElementTypeAttr)
        .def("CreateElementTypeAttr",
             &_CreateElementTypeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetIndicesAttr",
             &This::GetIndicesAttr)
        .def("CreateIndicesAttr",
             &_CreateIndicesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFamilyNameAttr",
             &This::GetFamilyNameAttr)
        .def("CreateFamilyNameAttr",
             &_CreateFamilyNameAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

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

#include "pxr/external/boost/python/tuple.hpp"

namespace {

static object
_WrapValidateFamily(const UsdGeomImageable &geom, 
               const TfToken &elementType,
               const TfToken &familyName)
{
    std::string reason;
    bool valid = UsdGeomSubset::ValidateFamily(geom, elementType, 
        familyName, &reason);
    return pxr_boost::python::make_tuple(valid, reason);
}

static object
_WrapValidateSubsets(    const std::vector<UsdGeomSubset> &subsets,
    const size_t elementCount, const TfToken &familyType)
{
    std::string reason;
    bool valid = UsdGeomSubset::ValidateSubsets(subsets, elementCount, 
        familyType, &reason);
    return pxr_boost::python::make_tuple(valid, reason);
}

WRAP_CUSTOM {
    typedef UsdGeomSubset This;

    // Register to and from vector conversions.
    pxr_boost::python::to_python_converter<std::vector<This>,  
        TfPySequenceToPython<std::vector<This> > >();

    TfPyContainerConversions::from_python_sequence<std::vector<This>,
        TfPyContainerConversions::variable_capacity_policy >();

    scope s_enum = _class 
        ;
    scope s = _class
        .def("CreateGeomSubset", &This::CreateGeomSubset,
            (arg("geom"), arg("subsetName"), arg("elementType"), 
             arg("indices"), arg("familyName")=TfToken(),
             arg("familyType")=TfToken()))
            .staticmethod("CreateGeomSubset")

        .def("CreateUniqueGeomSubset", &This::CreateUniqueGeomSubset,
            (arg("geom"), arg("subsetName"), arg("elementType"), 
             arg("indices"), arg("familyName")=TfToken(),
             arg("familyType")=TfToken()))
            .staticmethod("CreateUniqueGeomSubset")

        .def("GetAllGeomSubsets", &This::GetAllGeomSubsets,
            arg("geom"),
            return_value_policy<TfPySequenceToList>())
            .staticmethod("GetAllGeomSubsets")

        .def("GetGeomSubsets", &This::GetGeomSubsets,
            (arg("geom"), 
             arg("elementType")=TfToken(), 
             arg("familyName")=TfToken()),
            return_value_policy<TfPySequenceToList>())
            .staticmethod("GetGeomSubsets")

        .def("GetAllGeomSubsetFamilyNames", &This::GetAllGeomSubsetFamilyNames,
            (arg("geom")), 
            return_value_policy<TfPySequenceToList>())
            .staticmethod("GetAllGeomSubsetFamilyNames")

        .def("SetFamilyType", &This::SetFamilyType,
            (arg("geom"), arg("familyName"), arg("familyType")))
            .staticmethod("SetFamilyType")
        
        .def("GetFamilyType", &This::GetFamilyType,
            (arg("geom"), arg("familyName")))
            .staticmethod("GetFamilyType")

        .def("GetUnassignedIndices", (VtIntArray(*)(
            const UsdGeomImageable &geom, 
            const TfToken &elementType,
            const TfToken &familyName,
            const UsdTimeCode &time))&This::GetUnassignedIndices,
            (arg("geom"), arg("elementType"), arg("familyName"),
             arg("time")=UsdTimeCode::EarliestTime()))
        .def("GetUnassignedIndices", (VtIntArray(*)(
            const std::vector<UsdGeomSubset> &subsets,
            const size_t elementCount,
            const UsdTimeCode &time))&This::GetUnassignedIndices,
            (arg("subsets"), arg("elementCount"), 
             arg("time")=UsdTimeCode::EarliestTime()))
            .staticmethod("GetUnassignedIndices")

        .def("ValidateFamily", _WrapValidateFamily, 
            (arg("geom"), 
             arg("elementType")=TfToken(), 
             arg("familyName")=TfToken()))
            .staticmethod("ValidateFamily")

        .def("ValidateSubsets", _WrapValidateSubsets, 
            (arg("subsets"), 
             arg("elementCount"),
             arg("familyType")))
            .staticmethod("ValidateSubsets")

        ;

}

}
