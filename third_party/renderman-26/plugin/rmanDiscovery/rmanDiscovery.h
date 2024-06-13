//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_RMAN_DISCOVERY_RMAN_DISCOVERY_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_RMAN_DISCOVERY_RMAN_DISCOVERY_H

/// \file rmanDiscovery/rmanDiscovery.h

#include "pxr/pxr.h"
#include "pxr/usd/ndr/discoveryPlugin.h"
#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

/// \class RmanDiscoveryPlugin
///
/// Discovers nodes supported by the HdPrman render delegate.
///
class RmanDiscoveryPlugin final : public NdrDiscoveryPlugin
{
public:
    /// A filter for discovered nodes.  If the function returns false
    /// then the discovered node is discarded.  Otherwise the function
    /// can modify the discovery result.
    using Filter = std::function<bool(NdrNodeDiscoveryResult&)>;

    /// Constructor.
    RmanDiscoveryPlugin();

    /// DiscoverNodes() will pass each result to the given function for
    /// modification.  If the function returns false then the result is
    /// discarded.
    RmanDiscoveryPlugin(Filter filter);

    /// Virtual destructor
    ~RmanDiscoveryPlugin();

    /// Discover all of the nodes that appear within the the search paths
    /// provided and match the extensions provided.
    NdrNodeDiscoveryResultVec DiscoverNodes(const Context&) override;

    /// Gets the paths that this plugin is searching for nodes in.
    const NdrStringVec& GetSearchURIs() const override;

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

void
RmanDiscoveryPlugin_SetDefaultSearchPaths(const NdrStringVec &paths);

void
RmanDiscoveryPlugin_SetDefaultFollowSymlinks(bool followSymlinks);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_RMAN_DISCOVERY_RMAN_DISCOVERY_H
