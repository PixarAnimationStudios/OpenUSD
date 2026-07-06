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
_CreateLodOverrideModeAttr(UsdLodOverrideAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateLodOverrideModeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateLodOverrideIndexAttr(UsdLodOverrideAPI &self,
                                      object defaultVal, bool writeSparsely) {
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

void wrapUsdLodOverrideAPI()
{
    typedef UsdLodOverrideAPI This;

    UsdLodOverrideAPI_CanApplyResult::Wrap<UsdLodOverrideAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    class_<This, bases<UsdAPISchemaBase> >
        cls("OverrideAPI");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("CanApply", &_WrapCanApply, (arg("prim")))
        .staticmethod("CanApply")

        .def("Apply", &This::Apply, (arg("prim")))
        .staticmethod("Apply")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetLodOverrideModeAttr",
             &This::GetLodOverrideModeAttr)
        .def("CreateLodOverrideModeAttr",
             &_CreateLodOverrideModeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetLodOverrideIndexAttr",
             &This::GetLodOverrideIndexAttr)
        .def("CreateLodOverrideIndexAttr",
             &_CreateLodOverrideIndexAttr,
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

static
pxr_boost::python::tuple
_WrapComputeLODOverride(const UsdLodOverrideAPI& self,
                        UsdTimeCode time)
{
    float overrideIndex;
    TfToken overrideMode = self.ComputeLODOverride(&overrideIndex, time);
    if (overrideMode == UsdLodTokens->indexedLOD) {
        return pxr_boost::python::make_tuple(overrideMode,
                                             double(overrideIndex));
    } else {
        return pxr_boost::python::make_tuple(overrideMode,
                                             pxr_boost::python::object());
    }
}
    
WRAP_CUSTOM {
    typedef UsdLodOverrideAPI This;

    _class
        .def("ComputeLODOverride",
             &_WrapComputeLODOverride,
             arg("time") = UsdTimeCode::Default())

        ;
}

}
