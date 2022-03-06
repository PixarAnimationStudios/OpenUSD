//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/usd/usdUI/nodeGraphNodeAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>


PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrUsdUsdUIWrapNodeGraphNodeAPI {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreatePosAttr(UsdUINodeGraphNodeAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreatePosAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float2), writeSparsely);
}
        
static UsdAttribute
_CreateStackingOrderAttr(UsdUINodeGraphNodeAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateStackingOrderAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateDisplayColorAttr(UsdUINodeGraphNodeAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateDisplayColorAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color3f), writeSparsely);
}
        
static UsdAttribute
_CreateIconAttr(UsdUINodeGraphNodeAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateIconAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateExpansionStateAttr(UsdUINodeGraphNodeAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateExpansionStateAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateSizeAttr(UsdUINodeGraphNodeAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateSizeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float2), writeSparsely);
}

static std::string
_Repr(const UsdUINodeGraphNodeAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdUI.NodeGraphNodeAPI(%s)",
        primRepr.c_str());
}

struct UsdUINodeGraphNodeAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdUINodeGraphNodeAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdUINodeGraphNodeAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdUINodeGraphNodeAPI::CanApply(prim, &whyNot);
    return UsdUINodeGraphNodeAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdUINodeGraphNodeAPI()
{
    typedef UsdUINodeGraphNodeAPI This;

    pxrUsdUsdUIWrapNodeGraphNodeAPI::UsdUINodeGraphNodeAPI_CanApplyResult::Wrap<pxrUsdUsdUIWrapNodeGraphNodeAPI::UsdUINodeGraphNodeAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    boost::python::class_<This, boost::python::bases<UsdAPISchemaBase> >
        cls("NodeGraphNodeAPI");

    cls
        .def(boost::python::init<UsdPrim>(boost::python::arg("prim")))
        .def(boost::python::init<UsdSchemaBase const&>(boost::python::arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Get")

        .def("CanApply", &pxrUsdUsdUIWrapNodeGraphNodeAPI::_WrapCanApply, (boost::python::arg("prim")))
        .staticmethod("CanApply")

        .def("Apply", &This::Apply, (boost::python::arg("prim")))
        .staticmethod("Apply")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             boost::python::arg("includeInherited")=true,
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!boost::python::self)

        
        .def("GetPosAttr",
             &This::GetPosAttr)
        .def("CreatePosAttr",
             &pxrUsdUsdUIWrapNodeGraphNodeAPI::_CreatePosAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetStackingOrderAttr",
             &This::GetStackingOrderAttr)
        .def("CreateStackingOrderAttr",
             &pxrUsdUsdUIWrapNodeGraphNodeAPI::_CreateStackingOrderAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetDisplayColorAttr",
             &This::GetDisplayColorAttr)
        .def("CreateDisplayColorAttr",
             &pxrUsdUsdUIWrapNodeGraphNodeAPI::_CreateDisplayColorAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetIconAttr",
             &This::GetIconAttr)
        .def("CreateIconAttr",
             &pxrUsdUsdUIWrapNodeGraphNodeAPI::_CreateIconAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetExpansionStateAttr",
             &This::GetExpansionStateAttr)
        .def("CreateExpansionStateAttr",
             &pxrUsdUsdUIWrapNodeGraphNodeAPI::_CreateExpansionStateAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetSizeAttr",
             &This::GetSizeAttr)
        .def("CreateSizeAttr",
             &pxrUsdUsdUIWrapNodeGraphNodeAPI::_CreateSizeAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))

        .def("__repr__", pxrUsdUsdUIWrapNodeGraphNodeAPI::_Repr)
    ;

    pxrUsdUsdUIWrapNodeGraphNodeAPI::_CustomWrapCode(cls);
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

namespace pxrUsdUsdUIWrapNodeGraphNodeAPI {

WRAP_CUSTOM {
}

} // anonymous namespace
