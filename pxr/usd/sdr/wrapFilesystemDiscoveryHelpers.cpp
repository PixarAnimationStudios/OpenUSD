//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdr/filesystemDiscoveryHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/weakPtr.h"

#include "pxr/external/boost/python.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/tuple.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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

static object
_WrapFsHelpersSplitShaderIdentifier(const TfToken &identifier)
{
    TfToken family, name;
    SdrVersion version;
    if (SdrFsHelpersSplitShaderIdentifier(identifier,
            &family, &name, &version)) {
        return pxr_boost::python::make_tuple(family, name, version);
    } else {
        return object();
    }
}

void wrapFilesystemDiscoveryHelpers()
{
    class_<SdrDiscoveryUri>("DiscoveryUri")
        .def(init<SdrDiscoveryUri>())
        .def_readwrite("uri", &SdrDiscoveryUri::uri)
        .def_readwrite("resolvedUri", &SdrDiscoveryUri::resolvedUri)
    ;

    def("FsHelpersSplitShaderIdentifier", _WrapFsHelpersSplitShaderIdentifier,
        arg("identifier"));
    def("FsHelpersDiscoverShaderNodes", _WrapFsHelpersDiscoverShaderNodes,
        (args("searchPaths"),
        args("allowedExtensions"),
        args("followSymlinks") = true,
        args("context") = TfWeakPtr<SdrDiscoveryPluginContext>()));
    def("FsHelpersDiscoverFiles", SdrFsHelpersDiscoverFiles,
        (args("searchPaths"),
        args("allowedExtensions"),
        args("followSymlinks") = true),
        return_value_policy<TfPySequenceToList>());
}
