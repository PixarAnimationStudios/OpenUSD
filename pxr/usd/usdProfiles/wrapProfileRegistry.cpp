//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usdProfiles/profileRegistry.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pySingleton.h"
#include "pxr/base/tf/type.h"

#include "pxr/base/js/json.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"

PXR_NAMESPACE_OPEN_SCOPE
USDPROFILES_API void Usd_ProfilesRegistryTestClear();
USDPROFILES_API bool Usd_ProfilesRegistryTestLoadFromFile(
    const std::string& filePath,
    std::vector<std::string>* errors = nullptr);
PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace {

std::set<TfToken>
_ToTokenSet(nb::handle iterable)
{
    std::set<TfToken> out;
    if (!iterable.is_valid() || iterable.is_none()) {
        return out;
    }
    for (nb::handle item : nb::iter(iterable)) {
        out.insert(nb::cast<TfToken>(item));
    }
    return out;
}

// CoversCapabilities has an optional out-param; wrap it to return
// (QueryStatus, [CapabilityResult, ...]) as a Python tuple.
// Python callers may pass lists or sets for required/excepted.
nb::tuple
_CoversCapabilities(const TfToken& perspective,
                    nb::handle required,
                    nb::handle excepted)
{
    std::set<UsdProfileRegistry::CapabilityResult> results;
    UsdProfileRegistry::QueryStatus status =
        UsdProfileRegistry::CoversCapabilities(
            perspective,
            _ToTokenSet(required),
            _ToTokenSet(excepted),
            &results);
    nb::list pyResults;
    for (const auto& r : results) {
        pyResults.append(nb::cast(r));
    }
    return nb::make_tuple(status, pyResults);
}

// GetCapabilityStyles returns a JsObject (map<string,JsValue>); JsValue is not
// wrapped for Python, so return just the keys as a Python list.
nb::list
_GetCapabilityStyles()
{
    auto styles = UsdProfileRegistry::GetCapabilityStyles();
    nb::list keys;
    for (const auto& pair : styles) {
        keys.append(pair.first);
    }
    return keys;
}

// ParseCapabilityVersion returns std::pair<TfToken,int>; wrap as (str, int) tuple.
nb::tuple
_ParseCapabilityVersion(const TfToken& capability)
{
    auto [base, ver] = UsdProfileRegistry::ParseCapabilityVersion(capability);
    return nb::make_tuple(base, ver);
}

// _TestLoadFromFile returns (bool, [errors]) as a Python tuple.
nb::tuple
_TestLoadFromFile(const std::string& filePath)
{
    std::vector<std::string> errors;
    bool ok = Usd_ProfilesRegistryTestLoadFromFile(filePath, &errors);
    nb::list pyErrors;
    for (const auto& e : errors) {
        pyErrors.append(e);
    }
    return nb::make_tuple(ok, pyErrors);
}


} // anonymous namespace

void wrapUsdProfileRegistry(nb::module_ &m)
{
    using This = UsdProfileRegistry;
    using CR   = UsdProfileRegistry::CapabilityResult;
    using QS   = This::QueryStatus;

    nb::class_<This> cls(m, "ProfileRegistry");
    cls
        .def_static("HasCapability",             &This::HasCapability)
        .def_static("IsProfile",                 &This::IsProfile)
        .def_static("GetCapabilityMetadata",     &This::GetCapabilityMetadata)
        .def_static("GetPredecessors",
            +[](const TfToken& capability) {
                return TfNbSequenceToList(This::GetPredecessors(capability));
            })
        .def_static("GetTransitivePredecessors",
            +[](const TfToken& capability) {
                return TfNbSequenceToList(
                    This::GetTransitivePredecessors(capability));
            })
        .def_static("ParseCapabilityVersion",    &_ParseCapabilityVersion)
        .def_static("ResolveCapability",         &This::ResolveCapability)
        .def_static("HasPredecessor",            &This::HasPredecessor)
        .def_static("CoversCapabilities",        &_CoversCapabilities,
             nb::arg("perspective"),
             nb::arg("required"),
             nb::arg("excepted").none() = nb::none())
        .def_static("GetAllCapabilities",        &This::GetAllCapabilities,
             nb::rv_policy::copy)
        .def_static("GetAllProfiles",            &This::GetAllProfiles,
             nb::rv_policy::copy)
        .def_static("GetStyleForCapability",     &This::GetStyleForCapability)
        .def_static("GetSubgraphForCapability",  &This::GetSubgraphForCapability)
        .def_static("GetDocString",              &This::GetDocString)
        .def_static("GetDisplayName",            &This::GetDisplayName)
        .def_static("GetCapabilityStyles",       &_GetCapabilityStyles)
        .def_static("GetSchemaImpliedCapabilities",
             &This::GetSchemaImpliedCapabilities,
             nb::arg("schemaType"),
             nb::rv_policy::copy)
        .def_static("GetFileFormatImpliedCapabilities",
             &This::GetFileFormatImpliedCapabilities,
             nb::arg("formatType"),
             nb::rv_policy::copy)
        .def_static("_TestClear",                &Usd_ProfilesRegistryTestClear)
        .def_static("_TestLoadFromFile",         &_TestLoadFromFile,
             nb::arg("filePath"))
        ;

    // Nested types: QueryStatus and CapabilityResult are scoped under
    // ProfileRegistry in Python (UsdProfiles.ProfileRegistry.QueryStatus).
    nb::enum_<QS>(cls, "QueryStatus", nb::is_arithmetic())
        .value("NoPath",              QS::NoPath)
        .value("ValidPath",           QS::ValidPath)
        .value("Deprecated",          QS::Deprecated)
        .value("DeprecationConflict", QS::DeprecationConflict)
        .value("Excepted",            QS::Excepted)
        .value("CycleFound",          QS::CycleFound)
        ;

    nb::class_<CR>(cls, "CapabilityResult")
        .def_prop_ro("capability",
            +[](const CR& r) -> std::string {
                return r.capability.GetString(); })
        .def_ro("status", &CR::status)
        ;
}
