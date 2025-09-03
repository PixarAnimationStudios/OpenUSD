//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdContrived/testReflectedAPIBase.h"
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
_Repr(const UsdContrivedTestReflectedAPIBase &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdContrived.TestReflectedAPIBase(%s)",
        primRepr.c_str());
}

        
static UsdAttribute
_CreateTestAttrInternalAttr(UsdContrivedTestReflectedAPIBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTestAttrInternalAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateTestAttrDuplicateAttr(UsdContrivedTestReflectedAPIBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTestAttrDuplicateAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateTestAttrExternalAttr(UsdContrivedTestReflectedAPIBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTestAttrExternalAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
} // anonymous namespace

void wrapUsdContrivedTestReflectedAPIBase()
{
    typedef UsdContrivedTestReflectedAPIBase This;

    class_<This, bases<UsdTyped> >
        cls("TestReflectedAPIBase");

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


        
        .def("GetTestAttrInternalAttr",
             &This::GetTestAttrInternalAttr)
        .def("CreateTestAttrInternalAttr",
             &_CreateTestAttrInternalAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTestAttrDuplicateAttr",
             &This::GetTestAttrDuplicateAttr)
        .def("CreateTestAttrDuplicateAttr",
             &_CreateTestAttrDuplicateAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        .def("GetTestRelInternalRel",
             &This::GetTestRelInternalRel)
        .def("CreateTestRelInternalRel",
             &This::CreateTestRelInternalRel)

        .def("GetTestRelDuplicateRel",
             &This::GetTestRelDuplicateRel)
        .def("CreateTestRelDuplicateRel",
             &This::CreateTestRelDuplicateRel)

        .def("TestReflectedInternalAPI", &This::TestReflectedInternalAPI)
        
        .def("GetTestAttrExternalAttr",
             &This::GetTestAttrExternalAttr)
        .def("CreateTestAttrExternalAttr",
             &_CreateTestAttrExternalAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        .def("GetTestRelExternalRel",
             &This::GetTestRelExternalRel)
        .def("CreateTestRelExternalRel",
             &This::CreateTestRelExternalRel)

        .def("TestReflectedExternalAPI", &This::TestReflectedExternalAPI)
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

}
