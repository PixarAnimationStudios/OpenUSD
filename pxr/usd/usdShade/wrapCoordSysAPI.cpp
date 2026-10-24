//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdShade/coordSysAPI.h"
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
#include <nanobind/stl/pair.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/tuple.h>

namespace nb = nanobind;

#include <string>

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

void wrapUsdShadeCoordSysAPI(nb::module_ &m)
{
    typedef UsdShadeCoordSysAPI This;

    UsdShadeCoordSysAPI_CanApplyResult::WrapNb<UsdShadeCoordSysAPI_CanApplyResult>(m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase> cls(m, "CoordSysAPI");

    cls
        .def(nb::init<UsdPrim, TfToken>(), nb::arg("prim")=UsdPrim(), nb::arg("name")=TfToken())
        .def(nb::init<UsdSchemaBase const&, TfToken>(), nb::arg("schemaObj"), nb::arg("name"))
        .def(TfTypePythonClass())

        .def_static("Get",
            (UsdShadeCoordSysAPI(*)(const UsdStagePtr &stage, 
                                       const SdfPath &path))
               &This::Get,
            nb::arg("stage"), nb::arg("path"))
        .def_static("Get",
            (UsdShadeCoordSysAPI(*)(const UsdPrim &prim,
                                       const TfToken &name))
               &This::Get,
            nb::arg("prim"), nb::arg("name"))

        .def_static("GetAll",
            (std::vector<UsdShadeCoordSysAPI>(*)(const UsdPrim &prim))
                &This::GetAll,
            nb::arg("prim"),
             nb::rv_policy::copy)

        .def_static("CanApply", &_WrapCanApply, nb::arg("prim"), nb::arg("name"))

        .def_static("Apply", &This::Apply, nb::arg("prim"), nb::arg("name"))

        .def_static("GetSchemaAttributeNames",
             (const TfTokenVector &(*)(bool))&This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)
        .def_static("GetSchemaAttributeNames",
             (TfTokenVector(*)(bool, const TfToken &))
                &This::GetSchemaAttributeNames,
             nb::arg("includeInherited"),
             nb::arg("instanceName"),
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        
        .def("GetBindingRel",
             &This::GetBindingRel)
        .def("CreateBindingRel",
             &This::CreateBindingRel)
        .def_static("IsCoordSysAPIPath", _WrapIsCoordSysAPIPath)
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

namespace nanobind::detail {

template <>
struct type_caster<PXR_NS::UsdShadeCoordSysAPI::Binding> {
    NB_TYPE_CASTER(PXR_NS::UsdShadeCoordSysAPI::Binding, const_name("tuple"))

    static handle from_cpp(PXR_NS::UsdShadeCoordSysAPI::Binding const &b,
                           rv_policy, cleanup_list *) noexcept {
        return make_tuple(
            b.name, b.bindingRelPath, b.coordSysPrimPath).release();
    }
};

} // namespace nanobind::detail

namespace {

WRAP_CUSTOM {
    using This = UsdShadeCoordSysAPI;

    _class
        .def("HasLocalBindings", &This::HasLocalBindings) //deprecated
        .def_static("HasLocalBindingsForPrim", &This::HasLocalBindingsForPrim,
            nb::arg("prim"))

        .def("GetLocalBindings",
             &This::GetLocalBindings,
             nb::rv_policy::copy) //deprecated
        .def_static("GetLocalBindingsForPrim",
             &This::GetLocalBindingsForPrim,
             nb::arg("prim"),
             nb::rv_policy::copy)
        .def("GetLocalBinding",
             &This::GetLocalBinding)

        .def("FindBindingsWithInheritance",
             &This::FindBindingsWithInheritance,
             nb::rv_policy::copy) //deprecated
        .def_static("FindBindingsWithInheritanceForPrim",
             &This::FindBindingsWithInheritanceForPrim,
             nb::rv_policy::copy)
        .def("FindBindingWithInheritance",
             &This::FindBindingWithInheritance)

        .def("Bind", 
            (bool(This::*)(const TfToken&, const SdfPath&) const)
            &This::Bind,
            nb::arg("name"), nb::arg("path")) //deprecated
        .def("ApplyAndBind",
            (bool(This::*)(const TfToken&, const SdfPath&) const)
            &This::Bind,
            nb::arg("name"), nb::arg("path")) //deprecated
        .def("Bind",
            (bool(This::*)(const SdfPath&) const)
            &This::Bind,
            nb::arg("path"))

        .def("ClearBinding", 
            (bool(This::*)(const TfToken&, bool) const)
            &This::ClearBinding,
            nb::arg("name"), nb::arg("removeSpec")) //deprecated
        .def("ClearBinding",
            (bool(This::*)(bool) const)
            &This::ClearBinding,
            nb::arg("removeSpec"))

        .def("BlockBinding", 
            (bool(This::*)(const TfToken&) const)
            &This::BlockBinding,
            nb::arg("name")) //deprecated
        .def("BlockBinding",
            (bool(This::*)() const)
            &This::BlockBinding)

        .def_static("GetCoordSysRelationshipName",
            &This::GetCoordSysRelationshipName,
            nb::arg("coordSysName")) //deprecated

        .def_static("CanContainPropertyName", 
            &This::CanContainPropertyName, nb::arg("name"))
        ;
}

}
