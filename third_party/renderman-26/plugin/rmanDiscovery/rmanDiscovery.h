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
#if PXR_VERSION >= 2505
#include "pxr/usd/sdr/discoveryPlugin.h"
#else
#include "pxr/usd/ndr/discoveryPlugin.h"
#endif
#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

#if PXR_VERSION < 2505
using SdrDiscoveryPlugin = NdrDiscoveryPlugin;
using SdrShaderNodeDiscoveryResult = NdrNodeDiscoveryResult;
using SdrStringVec = NdrStringVec;
#endif

/// \class RmanDiscoveryPlugin
///
/// Discovers nodes supported by the HdPrman render delegate.
///
class RmanDiscoveryPlugin final : public SdrDiscoveryPlugin
{
public:
    /// A filter for discovered nodes.  If the function returns false
    /// then the discovered node is discarded.  Otherwise the function
    /// can modify the discovery result.
    using Filter = std::function<bool(SdrShaderNodeDiscoveryResult&)>;

    /// Constructor.
    RmanDiscoveryPlugin();

    /// DiscoverShaderNodes() will pass each result to the given function for
    /// modification.  If the function returns false then the result is
    /// discarded.
    RmanDiscoveryPlugin(Filter filter);

    /// Virtual destructor
    ~RmanDiscoveryPlugin();

    /// Discover all of the nodes that appear within the the search paths
    /// provided and match the extensions provided.
#if PXR_VERSION >= 2505
    SdrShaderNodeDiscoveryResultVec DiscoverShaderNodes(const Context&) override;
#else
    NdrNodeDiscoveryResultVec DiscoverNodes(const Context&) override;
#endif

    /// Gets the paths that this plugin is searching for nodes in.
    const SdrStringVec& GetSearchURIs() const override;

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

void
RmanDiscoveryPlugin_SetDefaultSearchPaths(const SdrStringVec &paths);

void
RmanDiscoveryPlugin_SetDefaultFollowSymlinks(bool followSymlinks);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_RMAN_DISCOVERY_RMAN_DISCOVERY_H
