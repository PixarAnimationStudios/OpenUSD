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

#ifndef PXR_USD_NDR_FILESYSTEM_DISCOVERY_H
#define PXR_USD_NDR_FILESYSTEM_DISCOVERY_H

/// \file ndrDiscovery/filesystemDiscovery.h

#include "pxr/pxr.h"
#include "pxr/usd/ndr/api.h"
#include "pxr/usd/ndr/discoveryPlugin.h"
#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(_NdrFilesystemDiscoveryPlugin);

/// \class _NdrFilesystemDiscoveryPlugin
///
/// Discovers nodes on the filesystem. The provided search paths are walked to
/// find files that have certain extensions. If a file with a matching extension
/// is found, it is turned into a `NdrNodeDiscoveryResult` and will be parsed
/// into a node when its information is accessed.
///
/// Parameters for this plugin are specified via environment variables (which
/// must be set before the library is loaded):
///
/// PXR_NDR_FS_PLUGIN_SEARCH_PATHS - The paths that should be searched,
/// recursively, for files that represent nodes. Paths should be separated by 
/// either a ':' or a ';' depending on your platform (it should mimic the PATH 
/// env var on your platform).  See ARCH_PATH_LIST_SEP.
///
/// PXR_NDR_FS_PLUGIN_ALLOWED_EXTS - The extensions on files that define nodes.
/// Do not include the leading ".". Extensions should be separated by a colon.
///
/// PXR_NDR_FS_PLUGIN_FOLLOW_SYMLINKS - Whether symlinks should be followed
/// while walking the search paths. Set to "true" (case sensitive) if they
/// should be followed.
///
class _NdrFilesystemDiscoveryPlugin final : public NdrDiscoveryPlugin
{
public:
    /// A filter for discovered nodes.  If the function returns false
    /// then the discovered node is discarded.  Otherwise the function
    /// can modify the discovery result.
    using Filter = std::function<bool(NdrNodeDiscoveryResult&)>;

    /// Constructor.
    NDR_API
    _NdrFilesystemDiscoveryPlugin();

    /// DiscoverNodes() will pass each result to the given function for
    /// modification.  If the function returns false then the result is
    /// discarded.
    NDR_API
    _NdrFilesystemDiscoveryPlugin(Filter filter);

    /// Destructor
    NDR_API
    ~_NdrFilesystemDiscoveryPlugin() {}

    /// Discover all of the nodes that appear within the the search paths
    /// provided and match the extensions provided.
    NDR_API
    NdrNodeDiscoveryResultVec DiscoverNodes(const Context&) override;

    /// Gets the paths that this plugin is searching for nodes in.
    NDR_API
    const NdrStringVec& GetSearchURIs() const override { return _searchPaths; }

private:
    /// The paths (abs) indicating where the plugin should search for nodes.
    NdrStringVec _searchPaths;

    /// The extensions (excluding leading '.') that signify a valid node file.
    /// The extension will be used as the `type` member in the resulting
    /// `NdrNodeDiscoveryResult` instance.
    NdrStringVec _allowedExtensions;

    /// Whether or not to follow symlinks while scanning directories for files.
    bool _followSymlinks;

    // The filter to run on the results.
    Filter _filter;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_NDR_FILESYSTEM_DISCOVERY_H
