//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/execIr/fkController.h"
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
_CreateParentInSpaceAttr(ExecIrFkController &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateParentInSpaceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix4d), writeSparsely);
}
        
static UsdAttribute
_CreateParentInDefaultSpaceAttr(ExecIrFkController &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateParentInDefaultSpaceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix4d), writeSparsely);
}
        
static UsdAttribute
_CreateOutSpaceAttr(ExecIrFkController &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateOutSpaceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix4d), writeSparsely);
}
        
static UsdAttribute
_CreateOutDefaultSpaceAttr(ExecIrFkController &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateOutDefaultSpaceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix4d), writeSparsely);
}
        
static UsdAttribute
_CreateInDefaultSpaceAttr(ExecIrFkController &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateInDefaultSpaceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix4d), writeSparsely);
}
        
static UsdAttribute
_CreateInTxAttr(ExecIrFkController &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateInTxAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateInTyAttr(ExecIrFkController &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateInTyAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateInTzAttr(ExecIrFkController &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateInTzAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateInRxAttr(ExecIrFkController &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateInRxAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateInRyAttr(ExecIrFkController &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateInRyAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateInRzAttr(ExecIrFkController &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateInRzAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateInRspinAttr(ExecIrFkController &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateInRspinAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateInRotationOrderAttr(ExecIrFkController &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateInRotationOrderAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static std::string
_Repr(const ExecIrFkController &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "ExecIr.FkController(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapExecIrFkController()
{
    typedef ExecIrFkController This;

    class_<This, bases<ExecIrController> >
        cls("FkController");

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

        
        .def("GetParentInSpaceAttr",
             &This::GetParentInSpaceAttr)
        .def("CreateParentInSpaceAttr",
             &_CreateParentInSpaceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetParentInDefaultSpaceAttr",
             &This::GetParentInDefaultSpaceAttr)
        .def("CreateParentInDefaultSpaceAttr",
             &_CreateParentInDefaultSpaceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetOutSpaceAttr",
             &This::GetOutSpaceAttr)
        .def("CreateOutSpaceAttr",
             &_CreateOutSpaceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetOutDefaultSpaceAttr",
             &This::GetOutDefaultSpaceAttr)
        .def("CreateOutDefaultSpaceAttr",
             &_CreateOutDefaultSpaceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetInDefaultSpaceAttr",
             &This::GetInDefaultSpaceAttr)
        .def("CreateInDefaultSpaceAttr",
             &_CreateInDefaultSpaceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetInTxAttr",
             &This::GetInTxAttr)
        .def("CreateInTxAttr",
             &_CreateInTxAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetInTyAttr",
             &This::GetInTyAttr)
        .def("CreateInTyAttr",
             &_CreateInTyAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetInTzAttr",
             &This::GetInTzAttr)
        .def("CreateInTzAttr",
             &_CreateInTzAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetInRxAttr",
             &This::GetInRxAttr)
        .def("CreateInRxAttr",
             &_CreateInRxAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetInRyAttr",
             &This::GetInRyAttr)
        .def("CreateInRyAttr",
             &_CreateInRyAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetInRzAttr",
             &This::GetInRzAttr)
        .def("CreateInRzAttr",
             &_CreateInRzAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetInRspinAttr",
             &This::GetInRspinAttr)
        .def("CreateInRspinAttr",
             &_CreateInRspinAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetInRotationOrderAttr",
             &This::GetInRotationOrderAttr)
        .def("CreateInRotationOrderAttr",
             &_CreateInRotationOrderAttr,
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
