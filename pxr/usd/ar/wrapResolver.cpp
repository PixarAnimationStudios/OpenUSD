//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/assetInfo.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverContext.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/usd/ar/pyResolverContext.h"
#include "pxr/base/tf/refPtr.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

class Ar_PyAnnotatedBoolResult
    : public TfPyAnnotatedBoolResult<std::string>
{
public:
    Ar_PyAnnotatedBoolResult(bool val, const std::string& annotation)
        : TfPyAnnotatedBoolResult<std::string>(val, annotation)
    {
    }
};

static 
Ar_PyAnnotatedBoolResult
_CanWriteAssetToPath(
    ArResolver& resolver, const ArResolvedPath& resolvedPath)
{
    std::string whyNot;
    const bool rval = resolver.CanWriteAssetToPath(resolvedPath, &whyNot);
    return Ar_PyAnnotatedBoolResult(rval, whyNot);
}

void
wrapResolver(nanobind::module_& m)
{
    namespace nb = nanobind;
    Ar_PyAnnotatedBoolResult::WrapNb<Ar_PyAnnotatedBoolResult>
        (m, "_PyAnnotatedBoolResult", "whyNot");

    typedef ArResolver This;

    nb::class_<This>(m, "Resolver")
        .def("CreateDefaultContext", &This::CreateDefaultContext)
        .def("CreateDefaultContextForAsset",
             &This::CreateDefaultContextForAsset,
             nb::arg("assetPath"))

        .def("CreateContextFromString",
             (ArResolverContext (This::*)(const std::string&) const)
                 &This::CreateContextFromString,
             nb::arg("contextStr"))

        .def("CreateContextFromString",
             (ArResolverContext (This::*)
                 (const std::string&, const std::string&) const)
                 &This::CreateContextFromString,
             nb::arg("uriScheme"), nb::arg("contextStr"))

        .def("CreateContextFromStrings", &This::CreateContextFromStrings,
             nb::arg("contextStrs"))

        .def("GetCurrentContext", &This::GetCurrentContext)

        .def("IsContextDependentPath", &This::IsContextDependentPath,
             nb::arg("assetPath"))

        .def("CreateIdentifier", &This::CreateIdentifier,
             nb::arg("assetPath"),
             nb::arg("anchorAssetPath") = ArResolvedPath())
        .def("CreateIdentifierForNewAsset", &This::CreateIdentifierForNewAsset,
             nb::arg("assetPath"),
             nb::arg("anchorAssetPath") = ArResolvedPath())

        .def("Resolve", &This::Resolve,
             nb::arg("assetPath"))
        .def("ResolveForNewAsset", &This::ResolveForNewAsset,
             nb::arg("assetPath"))

        .def("GetAssetInfo", &This::GetAssetInfo,
             nb::arg("assetPath"), nb::arg("resolvedPath"))
        .def("GetModificationTimestamp", &This::GetModificationTimestamp,
             nb::arg("assetPath"), nb::arg("resolvedPath"))
        .def("OpenAsset", &This::OpenAsset,
             nb::arg("resolvedPath"))
        .def("GetExtension", &This::GetExtension,
             nb::arg("assetPath"))

        .def("CanWriteAssetToPath", &_CanWriteAssetToPath,
             nb::arg("resolvedPath"))

        .def("RefreshContext", &This::RefreshContext)
        ;

    m.def("GetResolver", ArGetResolver,
          nb::rv_policy::reference);

    m.def("GetRegisteredURISchemes", ArGetRegisteredURISchemes);

    m.def("SetPreferredResolver", ArSetPreferredResolver,
          nb::arg("resolverTypeName"));

    m.def("GetUnderlyingResolver", ArGetUnderlyingResolver,
          nb::rv_policy::reference);
}
