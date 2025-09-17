//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdr/discoveryPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

/// A simple test-only discovery plugin that directly returns the nodes in the
/// test's testenv folder.
class _SdrTestDiscoveryPlugin : public SdrDiscoveryPlugin
{
public:
    _SdrTestDiscoveryPlugin() {
        _searchPaths.push_back("/TestSearchPath");
    }

    ~_SdrTestDiscoveryPlugin() { }

    SdrShaderNodeDiscoveryResultVec DiscoverShaderNodes(
        const Context&) override
    {
        return {
            SdrShaderNodeDiscoveryResult(
                // Identifier
                TfToken("TestNodeARGS"),

                // Version
                SdrVersion().GetAsDefault(),

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
            SdrShaderNodeDiscoveryResult(
                TfToken("TestNodeOSL"),
                SdrVersion().GetAsDefault(),
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
            SdrShaderNodeDiscoveryResult(
                TfToken("TestNodeSameName"),
                SdrVersion().GetAsDefault(),
                "TestNodeSameName",
                TfToken(),
                TfToken("args"),
                TfToken("RmanCpp"),
                "TestNodeSameName.args",
                "TestNodeSameName.args"
            ),
            SdrShaderNodeDiscoveryResult(
                TfToken("TestNodeSameName"),
                SdrVersion().GetAsDefault(),
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
    const SdrStringVec& GetSearchURIs() const override { return _searchPaths; }

private:
    /// The paths (abs) indicating where the plugin should search for nodes.
    SdrStringVec _searchPaths;
};

SDR_REGISTER_DISCOVERY_PLUGIN(_SdrTestDiscoveryPlugin)

/// A second simple test-only discovery plugin that directly returns the nodes
/// in the test's testenv folder.
class _SdrTestDiscoveryPlugin2 : public SdrDiscoveryPlugin
{
public:
    _SdrTestDiscoveryPlugin2() {
        _searchPaths.push_back("/TestSearchPath2");
    }

    ~_SdrTestDiscoveryPlugin2() { }

    SdrShaderNodeDiscoveryResultVec DiscoverShaderNodes(
        const Context&) override
    {
        return {
            SdrShaderNodeDiscoveryResult(
                // Identifier
                TfToken("TestNodeARGS2"),

                // Version
                SdrVersion().GetAsDefault(),

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
            SdrShaderNodeDiscoveryResult(
                TfToken("TestNodeGLSLFX"),
                SdrVersion().GetAsDefault(),
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
    const SdrStringVec& GetSearchURIs() const override { return _searchPaths; }

private:
    /// The paths (abs) indicating where the plugin should search for nodes.
    SdrStringVec _searchPaths;
};

SDR_REGISTER_DISCOVERY_PLUGIN(_SdrTestDiscoveryPlugin2)

PXR_NAMESPACE_CLOSE_SCOPE
