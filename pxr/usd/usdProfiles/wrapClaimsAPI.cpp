//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdProfiles/claimsAPI.h"
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

static std::string
_Repr(const UsdProfilesClaimsAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdProfiles.ClaimsAPI(%s)",
        primRepr.c_str());
}

struct UsdProfilesClaimsAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdProfilesClaimsAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdProfilesClaimsAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdProfilesClaimsAPI::CanApply(prim, &whyNot);
    return UsdProfilesClaimsAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdProfilesClaimsAPI(nb::module_ &m)
{
    typedef UsdProfilesClaimsAPI This;

    UsdProfilesClaimsAPI_CanApplyResult::WrapNb<UsdProfilesClaimsAPI_CanApplyResult>(
        m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase> cls(m, "ClaimsAPI");

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

static nb::tuple
_IsCompatibleWith(const UsdProfilesClaimsAPI& self, const TfToken& profile)
{
    std::set<UsdProfileRegistry::CapabilityResult> results;
    UsdProfileRegistry::QueryStatus status =
        self.IsCompatibleWith(profile, &results);
    nb::list pyResults;
    for (const auto& r : results)
        pyResults.append(nb::cast(r));
    return nb::make_tuple(status, pyResults);
}

WRAP_CUSTOM {
    _class
        .def("GetProfilesInfo",    &UsdProfilesClaimsAPI::GetProfilesInfo)
        .def("SetProfilesInfo",    &UsdProfilesClaimsAPI::SetProfilesInfo,
             nb::arg("info"))

        .def("GetCapabilityUsages",  &UsdProfilesClaimsAPI::GetCapabilityUsages)
        .def("SetCapabilityUsages",  &UsdProfilesClaimsAPI::SetCapabilityUsages,
             nb::arg("usages"))
        .def("SetCapabilityUsage",   &UsdProfilesClaimsAPI::SetCapabilityUsage,
             nb::arg("capability"), nb::arg("degradationClass"))
        .def("GetCapabilityUsage",   &UsdProfilesClaimsAPI::GetCapabilityUsage,
             nb::arg("capability"))

        .def("GetCompatibleProfiles",
             &UsdProfilesClaimsAPI::GetCompatibleProfiles,
             nb::rv_policy::copy)
        .def("SetProfileCompatible",
             &UsdProfilesClaimsAPI::SetProfileCompatible,
             nb::arg("profile"))
        .def("SetProfileCompatibleWithExceptions",
             &UsdProfilesClaimsAPI::SetProfileCompatibleWithExceptions,
             nb::arg("profile"), nb::arg("exceptions"))
        .def("GetProfileExceptions",
             &UsdProfilesClaimsAPI::GetProfileExceptions,
             nb::arg("profile"),
             nb::rv_policy::copy)
        .def("ClearProfileCompatibility",
             &UsdProfilesClaimsAPI::ClearProfileCompatibility,
             nb::arg("profile"))

        .def("IsCompatibleWith", &_IsCompatibleWith,
             nb::arg("profile"))

        .def("PopulateCapabilityUsages",
             &UsdProfilesClaimsAPI::PopulateCapabilityUsages)
    ;
}

}
