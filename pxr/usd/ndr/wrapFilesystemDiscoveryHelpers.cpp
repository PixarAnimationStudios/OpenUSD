//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/ndr/filesystemDiscoveryHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/weakPtr.h"

#include <boost/python.hpp>
#include <boost/python/def.hpp>
#include <boost/python/tuple.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

static NdrNodeDiscoveryResultVec
_WrapFsHelpersDiscoverNodes(
    const NdrStringVec& searchPaths,
    const NdrStringVec& allowedExtensions,
    bool followSymlinks,
    const TfWeakPtr<NdrDiscoveryPluginContext>& context)
{
    return NdrFsHelpersDiscoverNodes(searchPaths,
                                     allowedExtensions,
                                     followSymlinks,
                                     get_pointer(context));
}

static object
_WrapFsHelpersSplitShaderIdentifier(const TfToken &identifier)
{
    TfToken family, name;
    NdrVersion version;
    if (NdrFsHelpersSplitShaderIdentifier(identifier,
            &family, &name, &version)) {
        return boost::python::make_tuple(family, name, version);
    } else {
        return object();
    }
}

void wrapFilesystemDiscoveryHelpers()
{
    class_<NdrDiscoveryUri>("DiscoveryUri")
        .def(init<NdrDiscoveryUri>())
        .def_readwrite("uri", &NdrDiscoveryUri::uri)
        .def_readwrite("resolvedUri", &NdrDiscoveryUri::resolvedUri)
    ;

    def("FsHelpersSplitShaderIdentifier", _WrapFsHelpersSplitShaderIdentifier,
        arg("identifier"));
    def("FsHelpersDiscoverNodes", _WrapFsHelpersDiscoverNodes,
        (args("searchPaths"),
        args("allowedExtensions"),
        args("followSymlinks") = true,
        args("context") = TfWeakPtr<NdrDiscoveryPluginContext>()));
    def("FsHelpersDiscoverFiles", NdrFsHelpersDiscoverFiles,
        (args("searchPaths"),
        args("allowedExtensions"),
        args("followSymlinks") = true),
        return_value_policy<TfPySequenceToList>());
}
