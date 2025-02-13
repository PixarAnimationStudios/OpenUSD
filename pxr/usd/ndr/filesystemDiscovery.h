//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_NDR_FILESYSTEM_DISCOVERY_H
#define PXR_USD_NDR_FILESYSTEM_DISCOVERY_H

/// \file ndr/filesystemDiscovery.h
///
/// \deprecated
/// All Ndr objects are deprecated in favor of the corresponding Sdr objects
/// in sdr/filesystemDiscovery.h

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
/// \deprecated
/// Deprecated in favor of _SdrFilesystemDiscoveryPlugin. 
/// PXR_NDR_* environment variables will be moved to PXR_SDR_* environment
/// variables
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
    ///
    /// \deprecated
    /// Deprecated in favor of _SdrFilesystemDiscoveryPlugin::DiscoverShaderNodes
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
