//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/motionAPI.h"
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
_CreateMotionBlurScaleAttr(UsdGeomMotionAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMotionBlurScaleAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateVelocityScaleAttr(UsdGeomMotionAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateVelocityScaleAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateNonlinearSampleCountAttr(UsdGeomMotionAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateNonlinearSampleCountAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}

static std::string
_Repr(const UsdGeomMotionAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.MotionAPI(%s)",
        primRepr.c_str());
}

struct UsdGeomMotionAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdGeomMotionAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdGeomMotionAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdGeomMotionAPI::CanApply(prim, &whyNot);
    return UsdGeomMotionAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdGeomMotionAPI()
{
    typedef UsdGeomMotionAPI This;

    UsdGeomMotionAPI_CanApplyResult::Wrap<UsdGeomMotionAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    class_<This, bases<UsdAPISchemaBase> >
        cls("MotionAPI");

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

        
        .def("GetMotionBlurScaleAttr",
             &This::GetMotionBlurScaleAttr)
        .def("CreateMotionBlurScaleAttr",
             &_CreateMotionBlurScaleAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetVelocityScaleAttr",
             &This::GetVelocityScaleAttr)
        .def("CreateVelocityScaleAttr",
             &_CreateVelocityScaleAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetNonlinearSampleCountAttr",
             &This::GetNonlinearSampleCountAttr)
        .def("CreateNonlinearSampleCountAttr",
             &_CreateNonlinearSampleCountAttr,
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
    _class
        .def("ComputeVelocityScale", &UsdGeomMotionAPI::ComputeVelocityScale,
                (arg("time")=UsdTimeCode::Default()))
        .def("ComputeNonlinearSampleCount",
             &UsdGeomMotionAPI::ComputeNonlinearSampleCount,
                (arg("time")=UsdTimeCode::Default()))
        .def("ComputeMotionBlurScale", 
             &UsdGeomMotionAPI::ComputeMotionBlurScale,
             (arg("time")=UsdTimeCode::Default()))
     ;
}

} // anonymous namespace 
