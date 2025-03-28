//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/skeleton.h"
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
_CreateJointsAttr(UsdSkelSkeleton &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateJointsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->TokenArray), writeSparsely);
}
        
static UsdAttribute
_CreateJointNamesAttr(UsdSkelSkeleton &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateJointNamesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->TokenArray), writeSparsely);
}
        
static UsdAttribute
_CreateBindTransformsAttr(UsdSkelSkeleton &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateBindTransformsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix4dArray), writeSparsely);
}
        
static UsdAttribute
_CreateRestTransformsAttr(UsdSkelSkeleton &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRestTransformsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix4dArray), writeSparsely);
}

static std::string
_Repr(const UsdSkelSkeleton &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdSkel.Skeleton(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdSkelSkeleton()
{
    typedef UsdSkelSkeleton This;

    class_<This, bases<UsdGeomBoundable> >
        cls("Skeleton");

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

        
        .def("GetJointsAttr",
             &This::GetJointsAttr)
        .def("CreateJointsAttr",
             &_CreateJointsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetJointNamesAttr",
             &This::GetJointNamesAttr)
        .def("CreateJointNamesAttr",
             &_CreateJointNamesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetBindTransformsAttr",
             &This::GetBindTransformsAttr)
        .def("CreateBindTransformsAttr",
             &_CreateBindTransformsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRestTransformsAttr",
             &This::GetRestTransformsAttr)
        .def("CreateRestTransformsAttr",
             &_CreateRestTransformsAttr,
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

namespace {

WRAP_CUSTOM {
}

} // anonymous namespace 
