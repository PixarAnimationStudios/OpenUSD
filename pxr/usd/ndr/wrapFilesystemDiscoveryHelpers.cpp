//
// Copyright 2018 Pixar
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
                                     boost::get_pointer(context));
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
