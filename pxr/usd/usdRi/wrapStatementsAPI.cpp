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
#include "pxr/usd/usdRi/statementsAPI.h"
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

namespace pxrUsdUsdRiWrapStatementsAPI {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;


static std::string
_Repr(const UsdRiStatementsAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdRi.StatementsAPI(%s)",
        primRepr.c_str());
}

struct UsdRiStatementsAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdRiStatementsAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdRiStatementsAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdRiStatementsAPI::CanApply(prim, &whyNot);
    return UsdRiStatementsAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdRiStatementsAPI()
{
    typedef UsdRiStatementsAPI This;

    pxrUsdUsdRiWrapStatementsAPI::UsdRiStatementsAPI_CanApplyResult::Wrap<pxrUsdUsdRiWrapStatementsAPI::UsdRiStatementsAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    boost::python::class_<This, boost::python::bases<UsdAPISchemaBase> >
        cls("StatementsAPI");

    cls
        .def(boost::python::init<UsdPrim>(boost::python::arg("prim")))
        .def(boost::python::init<UsdSchemaBase const&>(boost::python::arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Get")

        .def("CanApply", &pxrUsdUsdRiWrapStatementsAPI::_WrapCanApply, (boost::python::arg("prim")))
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


        .def("__repr__", pxrUsdUsdRiWrapStatementsAPI::_Repr)
    ;

    pxrUsdUsdRiWrapStatementsAPI::_CustomWrapCode(cls);
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

namespace pxrUsdUsdRiWrapStatementsAPI {

static SdfPathVector
_GetModelCoordinateSystems(const UsdRiStatementsAPI &self)
{
    SdfPathVector result;
    self.GetModelCoordinateSystems(&result);
    return result;
}

static SdfPathVector
_GetModelScopedCoordinateSystems(const UsdRiStatementsAPI &self)
{
    SdfPathVector result;
    self.GetModelScopedCoordinateSystems(&result);
    return result;
}

WRAP_CUSTOM {
    _class
        .def("CreateRiAttribute",
             (UsdAttribute (UsdRiStatementsAPI::*)(
                 const TfToken &, const TfType &, const std::string &))
             &UsdRiStatementsAPI::CreateRiAttribute,
             (boost::python::arg("name"), boost::python::arg("tfType"), boost::python::arg("nameSpace")="user"))
        .def("CreateRiAttribute",
             (UsdAttribute (UsdRiStatementsAPI::*)(
                 const TfToken &, const std::string &, const std::string &))
             &UsdRiStatementsAPI::CreateRiAttribute,
             (boost::python::arg("name"), boost::python::arg("riType"), boost::python::arg("nameSpace")="user"))
        .def("GetRiAttribute",
             &UsdRiStatementsAPI::GetRiAttribute,
             (boost::python::arg("name"), boost::python::arg("nameSpace")="user"))
        .def("GetRiAttributes", &UsdRiStatementsAPI::GetRiAttributes,
             (boost::python::arg("nameSpace")=""),
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetRiAttributeName",
             UsdRiStatementsAPI::GetRiAttributeName, (boost::python::arg("prop")))
        .staticmethod("GetRiAttributeName")
        .def("GetRiAttributeNameSpace",
             &UsdRiStatementsAPI::GetRiAttributeNameSpace, (boost::python::arg("prop")))
        .staticmethod("GetRiAttributeNameSpace")
        .def("IsRiAttribute", &UsdRiStatementsAPI::IsRiAttribute, (boost::python::arg("prop")))
        .staticmethod("IsRiAttribute")
        .def("MakeRiAttributePropertyName",
             &UsdRiStatementsAPI::MakeRiAttributePropertyName, (boost::python::arg("attrName")))
        .staticmethod("MakeRiAttributePropertyName")
        .def("SetCoordinateSystem", &UsdRiStatementsAPI::SetCoordinateSystem,
             (boost::python::arg("coordSysName")))
        .def("GetCoordinateSystem", &UsdRiStatementsAPI::GetCoordinateSystem)
        .def("HasCoordinateSystem", &UsdRiStatementsAPI::HasCoordinateSystem)

        .def("SetScopedCoordinateSystem",
             &UsdRiStatementsAPI::SetScopedCoordinateSystem,
             (boost::python::arg("coordSysName")))
        .def("GetScopedCoordinateSystem",
             &UsdRiStatementsAPI::GetScopedCoordinateSystem)
        .def("HasScopedCoordinateSystem",
             &UsdRiStatementsAPI::HasScopedCoordinateSystem)

        .def("GetModelCoordinateSystems", _GetModelCoordinateSystems,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetModelScopedCoordinateSystems", _GetModelScopedCoordinateSystems,
             boost::python::return_value_policy<TfPySequenceToList>())
        ;
}

} // anonymous namespace
