//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/execIr/xformable.h"
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
_CreateRestTxAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRestTxAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateRestTyAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRestTyAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateRestTzAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRestTzAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateRestRxAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRestRxAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateRestRyAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRestRyAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateRestRzAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRestRzAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateRestSpaceAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRestSpaceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix4d), writeSparsely);
}
        
static UsdAttribute
_CreateDefaultTxAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDefaultTxAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateDefaultTyAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDefaultTyAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateDefaultTzAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDefaultTzAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateDefaultRxAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDefaultRxAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateDefaultRyAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDefaultRyAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateDefaultRzAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDefaultRzAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateDefaultSpaceAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDefaultSpaceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix4d), writeSparsely);
}
        
static UsdAttribute
_CreatePosedSpaceAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePosedSpaceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix4d), writeSparsely);
}
        
static UsdAttribute
_CreatePosedDefaultSpaceAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePosedDefaultSpaceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix4d), writeSparsely);
}
        
static UsdAttribute
_CreateAvarsTxAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAvarsTxAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateAvarsTyAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAvarsTyAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateAvarsTzAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAvarsTzAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateAvarsRxAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAvarsRxAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateAvarsRyAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAvarsRyAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateAvarsRzAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAvarsRzAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateAvarsRspinAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAvarsRspinAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateAvarsRotationOrderAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAvarsRotationOrderAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateAvarsDefaultSpaceAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAvarsDefaultSpaceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix4d), writeSparsely);
}
        
static UsdAttribute
_CreateAvarsUnitScaleFactorAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAvarsUnitScaleFactorAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateParentSpaceAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateParentSpaceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix4d), writeSparsely);
}
        
static UsdAttribute
_CreateParentDefaultSpaceAttr(ExecIrXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateParentDefaultSpaceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix4d), writeSparsely);
}

static std::string
_Repr(const ExecIrXformable &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "ExecIr.Xformable(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapExecIrXformable()
{
    typedef ExecIrXformable This;

    class_<This, bases<UsdTyped> >
        cls("Xformable");

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

        
        .def("GetRestTxAttr",
             &This::GetRestTxAttr)
        .def("CreateRestTxAttr",
             &_CreateRestTxAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRestTyAttr",
             &This::GetRestTyAttr)
        .def("CreateRestTyAttr",
             &_CreateRestTyAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRestTzAttr",
             &This::GetRestTzAttr)
        .def("CreateRestTzAttr",
             &_CreateRestTzAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRestRxAttr",
             &This::GetRestRxAttr)
        .def("CreateRestRxAttr",
             &_CreateRestRxAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRestRyAttr",
             &This::GetRestRyAttr)
        .def("CreateRestRyAttr",
             &_CreateRestRyAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRestRzAttr",
             &This::GetRestRzAttr)
        .def("CreateRestRzAttr",
             &_CreateRestRzAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRestSpaceAttr",
             &This::GetRestSpaceAttr)
        .def("CreateRestSpaceAttr",
             &_CreateRestSpaceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDefaultTxAttr",
             &This::GetDefaultTxAttr)
        .def("CreateDefaultTxAttr",
             &_CreateDefaultTxAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDefaultTyAttr",
             &This::GetDefaultTyAttr)
        .def("CreateDefaultTyAttr",
             &_CreateDefaultTyAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDefaultTzAttr",
             &This::GetDefaultTzAttr)
        .def("CreateDefaultTzAttr",
             &_CreateDefaultTzAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDefaultRxAttr",
             &This::GetDefaultRxAttr)
        .def("CreateDefaultRxAttr",
             &_CreateDefaultRxAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDefaultRyAttr",
             &This::GetDefaultRyAttr)
        .def("CreateDefaultRyAttr",
             &_CreateDefaultRyAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDefaultRzAttr",
             &This::GetDefaultRzAttr)
        .def("CreateDefaultRzAttr",
             &_CreateDefaultRzAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDefaultSpaceAttr",
             &This::GetDefaultSpaceAttr)
        .def("CreateDefaultSpaceAttr",
             &_CreateDefaultSpaceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetPosedSpaceAttr",
             &This::GetPosedSpaceAttr)
        .def("CreatePosedSpaceAttr",
             &_CreatePosedSpaceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetPosedDefaultSpaceAttr",
             &This::GetPosedDefaultSpaceAttr)
        .def("CreatePosedDefaultSpaceAttr",
             &_CreatePosedDefaultSpaceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAvarsTxAttr",
             &This::GetAvarsTxAttr)
        .def("CreateAvarsTxAttr",
             &_CreateAvarsTxAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAvarsTyAttr",
             &This::GetAvarsTyAttr)
        .def("CreateAvarsTyAttr",
             &_CreateAvarsTyAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAvarsTzAttr",
             &This::GetAvarsTzAttr)
        .def("CreateAvarsTzAttr",
             &_CreateAvarsTzAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAvarsRxAttr",
             &This::GetAvarsRxAttr)
        .def("CreateAvarsRxAttr",
             &_CreateAvarsRxAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAvarsRyAttr",
             &This::GetAvarsRyAttr)
        .def("CreateAvarsRyAttr",
             &_CreateAvarsRyAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAvarsRzAttr",
             &This::GetAvarsRzAttr)
        .def("CreateAvarsRzAttr",
             &_CreateAvarsRzAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAvarsRspinAttr",
             &This::GetAvarsRspinAttr)
        .def("CreateAvarsRspinAttr",
             &_CreateAvarsRspinAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAvarsRotationOrderAttr",
             &This::GetAvarsRotationOrderAttr)
        .def("CreateAvarsRotationOrderAttr",
             &_CreateAvarsRotationOrderAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAvarsDefaultSpaceAttr",
             &This::GetAvarsDefaultSpaceAttr)
        .def("CreateAvarsDefaultSpaceAttr",
             &_CreateAvarsDefaultSpaceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAvarsUnitScaleFactorAttr",
             &This::GetAvarsUnitScaleFactorAttr)
        .def("CreateAvarsUnitScaleFactorAttr",
             &_CreateAvarsUnitScaleFactorAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetParentSpaceAttr",
             &This::GetParentSpaceAttr)
        .def("CreateParentSpaceAttr",
             &_CreateParentSpaceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetParentDefaultSpaceAttr",
             &This::GetParentDefaultSpaceAttr)
        .def("CreateParentDefaultSpaceAttr",
             &_CreateParentDefaultSpaceAttr,
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

}
