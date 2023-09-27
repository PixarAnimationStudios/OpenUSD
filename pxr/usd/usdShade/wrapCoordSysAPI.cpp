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
#include "pxr/usd/usdShade/coordSysAPI.h"
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

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;


static bool _WrapIsCoordSysAPIPath(const SdfPath &path) {
    TfToken collectionName;
    return UsdShadeCoordSysAPI::IsCoordSysAPIPath(
        path, &collectionName);
}

static std::string
_Repr(const UsdShadeCoordSysAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    std::string instanceName = TfPyRepr(self.GetName());
    return TfStringPrintf(
        "UsdShade.CoordSysAPI(%s, '%s')",
        primRepr.c_str(), instanceName.c_str());
}

struct UsdShadeCoordSysAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdShadeCoordSysAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdShadeCoordSysAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim, const TfToken& name)
{
    std::string whyNot;
    bool result = UsdShadeCoordSysAPI::CanApply(prim, name, &whyNot);
    return UsdShadeCoordSysAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdShadeCoordSysAPI()
{
    typedef UsdShadeCoordSysAPI This;

    UsdShadeCoordSysAPI_CanApplyResult::Wrap<UsdShadeCoordSysAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    class_<This, bases<UsdAPISchemaBase> >
        cls("CoordSysAPI");

    cls
        .def(init<UsdPrim, TfToken>((arg("prim"), arg("name"))))
        .def(init<UsdSchemaBase const&, TfToken>((arg("schemaObj"), arg("name"))))
        .def(TfTypePythonClass())

        .def("Get",
            (UsdShadeCoordSysAPI(*)(const UsdStagePtr &stage, 
                                       const SdfPath &path))
               &This::Get,
            (arg("stage"), arg("path")))
        .def("Get",
            (UsdShadeCoordSysAPI(*)(const UsdPrim &prim,
                                       const TfToken &name))
               &This::Get,
            (arg("prim"), arg("name")))
        .staticmethod("Get")

        .def("GetAll",
            (std::vector<UsdShadeCoordSysAPI>(*)(const UsdPrim &prim))
                &This::GetAll,
            arg("prim"),
            return_value_policy<TfPySequenceToList>())
        .staticmethod("GetAll")

        .def("CanApply", &_WrapCanApply, (arg("prim"), arg("name")))
        .staticmethod("CanApply")

        .def("Apply", &This::Apply, (arg("prim"), arg("name")))
        .staticmethod("Apply")

        .def("GetSchemaAttributeNames",
             (const TfTokenVector &(*)(bool))&This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .def("GetSchemaAttributeNames",
             (TfTokenVector(*)(bool, const TfToken &))
                &This::GetSchemaAttributeNames,
             arg("includeInherited"),
             arg("instanceName"),
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)


        
        .def("GetBindingRel",
             &This::GetBindingRel)
        .def("CreateBindingRel",
             &This::CreateBindingRel)
        .def("IsCoordSysAPIPath", _WrapIsCoordSysAPIPath)
            .staticmethod("IsCoordSysAPIPath")
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

struct _BindingToTuple {
    static PyObject*
    convert(UsdShadeCoordSysAPI::Binding const& b) {
        boost::python::tuple result = boost::python::make_tuple(
            b.name, b.bindingRelPath, b.coordSysPrimPath );
        return boost::python::incref(result.ptr());
    }
};

WRAP_CUSTOM {
    using This = UsdShadeCoordSysAPI;

    _class
        .def("HasLocalBindings", &This::HasLocalBindings) //deprecated
        .def("HasLocalBindingsForPrim", &This::HasLocalBindingsForPrim,
            (args("prim")))
        .staticmethod("HasLocalBindingsForPrim")

        .def("GetLocalBindings",
             &This::GetLocalBindings,
             return_value_policy<TfPySequenceToList>()) //deprecated
        .def("GetLocalBindingsForPrim",
             &This::GetLocalBindingsForPrim,
             (args("prim")),
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetLocalBindingsForPrim")
        .def("GetLocalBinding",
             &This::GetLocalBinding)

        .def("FindBindingsWithInheritance",
             &This::FindBindingsWithInheritance,
             return_value_policy<TfPySequenceToList>()) //deprecated
        .def("FindBindingsWithInheritanceForPrim",
             &This::FindBindingsWithInheritanceForPrim,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("FindBindingsWithInheritanceForPrim")
        .def("FindBindingWithInheritance",
             &This::FindBindingWithInheritance)

        .def("Bind", 
            (bool(This::*)(const TfToken&, const SdfPath&) const)
            &This::Bind,
            (args("name"), args("path"))) //deprecated
        .def("ApplyAndBind",
            (bool(This::*)(const TfToken&, const SdfPath&) const)
            &This::Bind,
            (args("name"), args("path"))) //deprecated
        .def("Bind",
            (bool(This::*)(const SdfPath&) const)
            &This::Bind,
            (args("path")))

        .def("ClearBinding", 
            (bool(This::*)(const TfToken&, bool) const)
            &This::ClearBinding,
            (args("name"), arg("removeSpec"))) //deprecated
        .def("ClearBinding",
            (bool(This::*)(bool) const)
            &This::ClearBinding,
            (args("removeSpec")))

        .def("BlockBinding", 
            (bool(This::*)(const TfToken&) const)
            &This::BlockBinding,
            (args("name"))) //deprecated
        .def("BlockBinding",
            (bool(This::*)() const)
            &This::BlockBinding)

        .def("GetCoordSysRelationshipName",
            &This::GetCoordSysRelationshipName,
            arg("coordSysName"))
        .staticmethod("GetCoordSysRelationshipName") //deprecated

        .def("CanContainPropertyName", 
            &This::CanContainPropertyName, arg("name"))
        .staticmethod("CanContainPropertyName")
        ;

     // Register to and from python conversion for parameter pairs
     to_python_converter<This::Binding,
         _BindingToTuple>();
}

}
