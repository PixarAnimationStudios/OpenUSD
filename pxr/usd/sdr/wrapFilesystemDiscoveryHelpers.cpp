//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdr/discoveryPlugin.h"
#include "pxr/usd/sdr/filesystemDiscoveryHelpers.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/weakPtr.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

static SdrShaderNodeDiscoveryResultVec
_WrapFsHelpersDiscoverShaderNodes(
    const SdrStringVec& searchPaths,
    const SdrStringVec& allowedExtensions,
    bool followSymlinks,
    const TfWeakPtr<SdrDiscoveryPluginContext>& context)
{
    return SdrFsHelpersDiscoverShaderNodes(searchPaths,
                                           allowedExtensions,
                                           followSymlinks,
                                           get_pointer(context));
}

static nb::object
_WrapFsHelpersSplitShaderIdentifier(const TfToken &identifier)
{
    TfToken function, name;
    SdrVersion version;
    if (SdrFsHelpersSplitShaderIdentifier(identifier,
            &function, &name, &version)) {
        return nb::make_tuple(function, name, version);
    } else {
        return nb::none();
    }
}

void wrapFilesystemDiscoveryHelpers(nanobind::module_ &m)
{
    nb::class_<SdrDiscoveryUri>(m, "DiscoveryUri")
        .def(nb::init<>())
        .def(nb::init<const SdrDiscoveryUri&>())
        .def_rw("uri", &SdrDiscoveryUri::uri)
        .def_rw("resolvedUri", &SdrDiscoveryUri::resolvedUri)
    ;

    m.def("FsHelpersSplitShaderIdentifier", _WrapFsHelpersSplitShaderIdentifier,
        nb::arg("identifier"));
    m.def("FsHelpersDiscoverShaderNodes", _WrapFsHelpersDiscoverShaderNodes,
        nb::arg("searchPaths"),
        nb::arg("allowedExtensions"),
        nb::arg("followSymlinks") = true,
        nb::arg("context") = TfWeakPtr<SdrDiscoveryPluginContext>());
    m.def("FsHelpersDiscoverFiles", SdrFsHelpersDiscoverFiles,
        nb::arg("searchPaths"),
        nb::arg("allowedExtensions"),
        nb::arg("followSymlinks") = true);
}
