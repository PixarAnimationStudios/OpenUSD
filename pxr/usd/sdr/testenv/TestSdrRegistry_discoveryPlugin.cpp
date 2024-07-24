//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
                "TestNodeOSL.oso",
                std::string(),
                // Test specifying an invalid encoding
                {{TfToken("sdrUsdEncodingVersion"), std::string("foobar")}}
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
                "TestNodeSameName.oso",
                std::string(),
                // Mark this shader as having a legacy USD encoding
                {{TfToken("sdrUsdEncodingVersion"), std::string("0")}}
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

/// A second simple test-only discovery plugin that directly returns the nodes
/// in the test's testenv folder.
class _NdrTestDiscoveryPlugin2 : public NdrDiscoveryPlugin
{
public:
    _NdrTestDiscoveryPlugin2() {
        _searchPaths.push_back("/TestSearchPath2");
    }

    ~_NdrTestDiscoveryPlugin2() { }

    NdrNodeDiscoveryResultVec DiscoverNodes(const Context&) override
    {
        return {
            NdrNodeDiscoveryResult(
                // Identifier
                TfToken("TestNodeARGS2"),

                // Version
                NdrVersion().GetAsDefault(),

                // Name
                "TestNodeARGS2",

                // Family
                TfToken(),

                // Discovery type
                TfToken("args"),

                // Source type
                TfToken("RmanCpp"),

                // URI
                "TestNodeARGS2.args",

                // Resolved URI
                "TestNodeARGS2.args"
            ),
            NdrNodeDiscoveryResult(
                TfToken("TestNodeGLSLFX"),
                NdrVersion().GetAsDefault(),
                "TestNodeGLSLFX",
                TfToken(),
                TfToken("glslfx"),
                TfToken("glslfx"),
                "TestNodeGLSLFX.glslfx",
                "TestNodeGLSLFX.glslfx"
            )
        };
    }

    /// Gets the paths that this plugin is searching for nodes in.
    const NdrStringVec& GetSearchURIs() const override { return _searchPaths; }

private:
    /// The paths (abs) indicating where the plugin should search for nodes.
    NdrStringVec _searchPaths;
};

NDR_REGISTER_DISCOVERY_PLUGIN(_NdrTestDiscoveryPlugin2)

PXR_NAMESPACE_CLOSE_SCOPE
