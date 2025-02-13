//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_SDR_FILESYSTEM_DISCOVERY_H
#define PXR_USD_SDR_FILESYSTEM_DISCOVERY_H

/// \file sdr/filesystemDiscovery.h
///
/// \note
/// All Ndr objects are deprecated in favor of the corresponding Sdr objects
/// in this file. All existing pxr/usd/ndr implementations will be moved to
/// pxr/usd/sdr.

#include "pxr/pxr.h"
#include "pxr/usd/ndr/filesystemDiscoveryHelpers.h"
#include "pxr/usd/sdr/api.h"
#include "pxr/usd/sdr/discoveryPlugin.h"
#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(_SdrFilesystemDiscoveryPlugin);

/// \class _SdrFilesystemDiscoveryPlugin
///
/// Discovers shader nodes on the filesystem. The provided search paths are
/// walked to find files that have certain extensions. If a file with a
/// matching extension is found, it is turned into a
/// `SdrShaderNodeDiscoveryResult` and will be parsed into a node when its
/// information is accessed.
///
/// Parameters for this plugin are specified via environment variables (which
/// must be set before the library is loaded):
///
/// PXR_SDR_FS_PLUGIN_SEARCH_PATHS - The paths that should be searched,
/// recursively, for files that represent nodes. Paths should be separated by 
/// either a ':' or a ';' depending on your platform (it should mimic the PATH 
/// env var on your platform).  See ARCH_PATH_LIST_SEP.
///
/// PXR_SDR_FS_PLUGIN_ALLOWED_EXTS - The extensions on files that define nodes.
/// Do not include the leading ".". Extensions should be separated by a colon.
///
/// PXR_SDR_FS_PLUGIN_FOLLOW_SYMLINKS - Whether symlinks should be followed
/// while walking the search paths. Set to "true" (case sensitive) if they
/// should be followed.
class _SdrFilesystemDiscoveryPlugin final : public SdrDiscoveryPlugin {
public:
    /// A filter for discovered nodes.  If the function returns false
    /// then the discovered node is discarded.  Otherwise the function
    /// can modify the discovery result.
    using Filter = std::function<bool(SdrShaderNodeDiscoveryResult&)>;

    /// Constructor.
    SDR_API
    _SdrFilesystemDiscoveryPlugin();

    /// DiscoverNodes() will pass each result to the given function for
    /// modification.  If the function returns false then the result is
    /// discarded.
    SDR_API
    _SdrFilesystemDiscoveryPlugin(Filter filter);

    /// Destructor
    SDR_API
    ~_SdrFilesystemDiscoveryPlugin() {}

    /// Discover all of the nodes that appear within the the search paths
    /// provided and match the extensions provided.
    SDR_API
    SdrShaderNodeDiscoveryResultVec DiscoverShaderNodes(
        const Context&) override;

    /// Gets the paths that this plugin is searching for nodes in.
    SDR_API
    const SdrStringVec& GetSearchURIs() const override { return _searchPaths; }

private:
    /// The paths (abs) indicating where the plugin should search for nodes.
    SdrStringVec _searchPaths;

    /// The extensions (excluding leading '.') that signify a valid node file.
    /// The extension will be used as the `type` member in the resulting
    /// `SdrShaderNodeDiscoveryResult` instance.
    SdrStringVec _allowedExtensions;

    /// Whether or not to follow symlinks while scanning directories for files.
    bool _followSymlinks;

    // The filter to run on the results.
    Filter _filter;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDR_FILESYSTEM_DISCOVERY_H
