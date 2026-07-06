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

#include "pxr/base/js/json.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/dict.hpp"
#include "pxr/external/boost/python/enum.hpp"
#include "pxr/external/boost/python/list.hpp"
#include "pxr/external/boost/python/scope.hpp"
#include "pxr/external/boost/python/tuple.hpp"

PXR_NAMESPACE_OPEN_SCOPE
USDPROFILES_API void Usd_ProfilesRegistryTestClear();
USDPROFILES_API bool Usd_ProfilesRegistryTestLoadFromFile(
    const std::string& filePath,
    std::vector<std::string>* errors = nullptr);
PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

// CoversCapabilities has an optional out-param; wrap it to return
// (QueryStatus, [CapabilityResult, ...]) as a Python tuple.
object
_CoversCapabilities(const TfToken& perspective,
                    const std::vector<TfToken>& required,
                    const std::vector<TfToken>& excepted)
{
    std::vector<UsdProfileRegistry::CapabilityResult> results;
    UsdProfileRegistry::QueryStatus status =
        UsdProfileRegistry::CoversCapabilities(
            perspective, required, excepted, &results);
    list pyResults;
    for (const auto& r : results) {
        pyResults.append(r);
    }
    return make_tuple(status, pyResults);
}

// GetCapabilityStyles returns a JsObject (map<string,JsValue>); JsValue is not
// wrapped for Python, so return just the keys as a Python list.
object
_GetCapabilityStyles()
{
    auto styles = UsdProfileRegistry::GetCapabilityStyles();
    list keys;
    for (const auto& pair : styles) {
        keys.append(pair.first);
    }
    return keys;
}

// ParseCapabilityVersion returns std::pair<TfToken,int>; wrap as (str, int) tuple.
object
_ParseCapabilityVersion(const TfToken& capability)
{
    auto [base, ver] = UsdProfileRegistry::ParseCapabilityVersion(capability);
    return make_tuple(base, ver);
}

// _TestLoadFromFile returns (bool, [errors]) as a Python tuple.
object
_TestLoadFromFile(const std::string& filePath)
{
    std::vector<std::string> errors;
    bool ok = Usd_ProfilesRegistryTestLoadFromFile(filePath, &errors);
    list pyErrors;
    for (const auto& e : errors) {
        pyErrors.append(e);
    }
    return make_tuple(ok, pyErrors);
}

} // anonymous namespace

void wrapUsdProfileRegistry()
{
    using This = UsdProfileRegistry;
    using CR   = UsdProfileRegistry::CapabilityResult;
    using QS   = This::QueryStatus;

    // Open a scope so QueryStatus and CapabilityResult become nested types
    // accessible as UsdProfiles.ProfileRegistry.QueryStatus etc.
    {
        scope in_ProfileRegistry =
            class_<This, noncopyable>("ProfileRegistry", no_init)

            .def("HasCapability",             &This::HasCapability)
            .staticmethod("HasCapability")

            .def("IsProfile",                 &This::IsProfile)
            .staticmethod("IsProfile")

            .def("GetCapabilityMetadata",     &This::GetCapabilityMetadata)
            .staticmethod("GetCapabilityMetadata")

            .def("GetPredecessors",           &This::GetPredecessors,
                 return_value_policy<TfPySequenceToList>())
            .staticmethod("GetPredecessors")

            .def("GetTransitivePredecessors", &This::GetTransitivePredecessors,
                 return_value_policy<TfPySequenceToList>())
            .staticmethod("GetTransitivePredecessors")

            .def("ParseCapabilityVersion",    &_ParseCapabilityVersion)
            .staticmethod("ParseCapabilityVersion")

            .def("ResolveCapability",         &This::ResolveCapability)
            .staticmethod("ResolveCapability")

            .def("HasPredecessor",            &This::HasPredecessor)
            .staticmethod("HasPredecessor")

            .def("CoversCapabilities",        &_CoversCapabilities,
                 (arg("perspective"), arg("required"), arg("excepted")=
                  std::vector<TfToken>()))
            .staticmethod("CoversCapabilities")

            .def("GetAllCapabilities",        &This::GetAllCapabilities,
                 return_value_policy<TfPySequenceToList>())
            .staticmethod("GetAllCapabilities")

            .def("GetAllProfiles",            &This::GetAllProfiles,
                 return_value_policy<TfPySequenceToList>())
            .staticmethod("GetAllProfiles")

            .def("GetStyleForCapability",     &This::GetStyleForCapability)
            .staticmethod("GetStyleForCapability")

            .def("GetSubgraphForCapability",  &This::GetSubgraphForCapability)
            .staticmethod("GetSubgraphForCapability")

            .def("GetDocString",              &This::GetDocString)
            .staticmethod("GetDocString")

            .def("GetDisplayName",            &This::GetDisplayName)
            .staticmethod("GetDisplayName")

            .def("GetCapabilityStyles",       &_GetCapabilityStyles)
            .staticmethod("GetCapabilityStyles")

            .def("_TestClear",                &Usd_ProfilesRegistryTestClear)
            .staticmethod("_TestClear")

            .def("_TestLoadFromFile",         &_TestLoadFromFile,
                 (arg("filePath")))
            .staticmethod("_TestLoadFromFile")
            ;

        // Nested types: QueryStatus and CapabilityResult are scoped under
        // ProfileRegistry in Python (UsdProfiles.ProfileRegistry.QueryStatus).
        enum_<QS>("QueryStatus")
            .value("NoPath",              QS::NoPath)
            .value("ValidPath",           QS::ValidPath)
            .value("Deprecated",          QS::Deprecated)
            .value("DeprecationConflict", QS::DeprecationConflict)
            .value("Excepted",            QS::Excepted)
            .value("CycleFound",          QS::CycleFound)
            ;

        class_<CR>("CapabilityResult", no_init)
            .add_property("capability",
                +[](const CR& r) -> std::string {
                    return r.capability.GetString(); })
            .def_readonly("status", &CR::status)
            ;
    }
}
