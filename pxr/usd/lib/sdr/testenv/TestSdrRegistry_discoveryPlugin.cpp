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
#include "pxr/usd/ndr/discoveryPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

/// A simple test-only discovery plugin that directly returns the nodes in the
/// test's testenv folder.
class _NdrTestDiscoveryPlugin : public NdrDiscoveryPlugin
{
public:
    _NdrTestDiscoveryPlugin() {
        _searchPaths.push_back("/TestSearchPath");
    }

    ~_NdrTestDiscoveryPlugin() { }

    NdrNodeDiscoveryResultVec DiscoverNodes(const Context&) override
    {
        return {
            NdrNodeDiscoveryResult(
                // Identifier
                TfToken("TestNodeARGS"),

                // Version
                NdrVersion().GetAsDefault(),

                // Name
                "TestNodeARGS",

                // Family
                TfToken(),

                // Discovery type
                TfToken("args"),

                // Source type
                TfToken("RmanCpp"),

                // URI
                "TestNodeARGS.args",

                // Resolved URI
                "TestNodeARGS.args"
            ),
            NdrNodeDiscoveryResult(
                TfToken("TestNodeOSL"),
                NdrVersion().GetAsDefault(),
                "TestNodeOSL",
                TfToken(),
                TfToken("oso"),
                TfToken("OSL"),
                "TestNodeOSL.oso",
                "TestNodeOSL.oso"
            ),
            NdrNodeDiscoveryResult(
                TfToken("TestNodeSameName"),
                NdrVersion().GetAsDefault(),
                "TestNodeSameName",
                TfToken(),
                TfToken("args"),
                TfToken("RmanCpp"),
                "TestNodeSameName.args",
                "TestNodeSameName.args"
            ),
            NdrNodeDiscoveryResult(
                TfToken("TestNodeSameName"),
                NdrVersion().GetAsDefault(),
                "TestNodeSameName",
                TfToken(),
                TfToken("oso"),
                TfToken("OSL"),
                "TestNodeSameName.oso",
                "TestNodeSameName.oso"
            )
        };
    }

    /// Gets the paths that this plugin is searching for nodes in.
    const NdrStringVec& GetSearchURIs() const override { return _searchPaths; }

private:
    /// The paths (abs) indicating where the plugin should search for nodes.
    NdrStringVec _searchPaths;
};

NDR_REGISTER_DISCOVERY_PLUGIN(_NdrTestDiscoveryPlugin)

PXR_NAMESPACE_CLOSE_SCOPE
