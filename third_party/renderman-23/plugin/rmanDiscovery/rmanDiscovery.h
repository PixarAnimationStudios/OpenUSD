//
// Copyright 2019 Pixar
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

#ifndef EXT_RMANPKG_23_0_PLUGIN_RENDERMAN_PLUGIN_RMAN_DISCOVERY_RMAN_DISCOVERY_H
#define EXT_RMANPKG_23_0_PLUGIN_RENDERMAN_PLUGIN_RMAN_DISCOVERY_RMAN_DISCOVERY_H

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

#endif // EXT_RMANPKG_23_0_PLUGIN_RENDERMAN_PLUGIN_RMAN_DISCOVERY_RMAN_DISCOVERY_H
