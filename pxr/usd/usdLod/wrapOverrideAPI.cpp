//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLod/overrideAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateLodOverrideModeAttr(UsdLodOverrideAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateLodOverrideModeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateLodOverrideIndexAttr(UsdLodOverrideAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateLodOverrideIndexAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}

static std::string
_Repr(const UsdLodOverrideAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdLod.OverrideAPI(%s)",
        primRepr.c_str());
}

struct UsdLodOverrideAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdLodOverrideAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdLodOverrideAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdLodOverrideAPI::CanApply(prim, &whyNot);
    return UsdLodOverrideAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdLodOverrideAPI(nb::module_ &m)
{
    typedef UsdLodOverrideAPI This;

    UsdLodOverrideAPI_CanApplyResult::WrapNb<UsdLodOverrideAPI_CanApplyResult>(
        m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase> cls(m, "OverrideAPI");

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

        
        .def("GetLodOverrideModeAttr",
             &This::GetLodOverrideModeAttr)
        .def("CreateLodOverrideModeAttr",
             &_CreateLodOverrideModeAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetLodOverrideIndexAttr",
             &This::GetLodOverrideIndexAttr)
        .def("CreateLodOverrideIndexAttr",
             &_CreateLodOverrideIndexAttr,
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

static
nb::tuple
_WrapComputeLODOverride(const UsdLodOverrideAPI& self,
                        UsdTimeCode time)
{
    float overrideIndex;
    TfToken overrideMode = self.ComputeLODOverride(&overrideIndex, time);
    if (overrideMode == UsdLodTokens->indexedLOD) {
        return nb::make_tuple(overrideMode, double(overrideIndex));
    } else {
        return nb::make_tuple(overrideMode, nb::none());
    }
}
    
WRAP_CUSTOM {
    typedef UsdLodOverrideAPI This;

    _class
        .def("ComputeLODOverride",
             &_WrapComputeLODOverride,
             nb::arg("time") = UsdTimeCode::Default())

        ;
}

}
