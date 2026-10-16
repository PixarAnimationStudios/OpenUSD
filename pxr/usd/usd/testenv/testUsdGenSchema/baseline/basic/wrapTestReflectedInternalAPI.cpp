//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdContrived/testReflectedInternalAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/wrapTypeHelpers.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateTestAttrInternalAttr(UsdContrivedTestReflectedInternalAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateTestAttrInternalAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateTestAttrDuplicateAttr(UsdContrivedTestReflectedInternalAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateTestAttrDuplicateAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}

static std::string
_Repr(const UsdContrivedTestReflectedInternalAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdContrived.TestReflectedInternalAPI(%s)",
        primRepr.c_str());
}

struct UsdContrivedTestReflectedInternalAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdContrivedTestReflectedInternalAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdContrivedTestReflectedInternalAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdContrivedTestReflectedInternalAPI::CanApply(prim, &whyNot);
    return UsdContrivedTestReflectedInternalAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdContrivedTestReflectedInternalAPI(nb::module_ &m)
{
    typedef UsdContrivedTestReflectedInternalAPI This;

    UsdContrivedTestReflectedInternalAPI_CanApplyResult::Wrap<UsdContrivedTestReflectedInternalAPI_CanApplyResult>(
        m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase>
        cls(m, "TestReflectedInternalAPI");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

        .def_static("CanApply", &_WrapCanApply, nb::arg("prim"))

        .def_static("Apply", &This::Apply, nb::arg("prim"))

        .def_static("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        
        .def("GetTestAttrInternalAttr",
             &This::GetTestAttrInternalAttr)
        .def("CreateTestAttrInternalAttr",
             &_CreateTestAttrInternalAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetTestAttrDuplicateAttr",
             &This::GetTestAttrDuplicateAttr)
        .def("CreateTestAttrDuplicateAttr",
             &_CreateTestAttrDuplicateAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        
        .def("GetTestRelInternalRel",
             &This::GetTestRelInternalRel)
        .def("CreateTestRelInternalRel",
             &This::CreateTestRelInternalRel)
        
        .def("GetTestRelDuplicateRel",
             &This::GetTestRelDuplicateRel)
        .def("CreateTestRelDuplicateRel",
             &This::CreateTestRelDuplicateRel)
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
