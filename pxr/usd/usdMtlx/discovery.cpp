//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usdMtlx/utils.h"
#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/sdr/discoveryPlugin.h"
#include "pxr/usd/sdr/filesystemDiscoveryHelpers.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include <algorithm>
#include <cctype>
#include <map>

#include "pxr/base/trace/trace.h"

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((discoveryType, "mtlx"))
);

// Maps a nodedef name to its SdrShaderNode name.
using _NameMapping = std::map<std::string, std::string>;

// Fill the name mapping with the shortest name found in the inheritance
// hierarchy:
void
_MapNodeNamesToBaseForVersioning(mx::ConstElementPtr mtlx, _NameMapping* mapping)
{
    TRACE_FUNCTION();

    static const std::string inheritAttr("inherit");

    // Find shortest:
    const std::string* shortestName = &mtlx->getName();
    mx::ConstElementPtr current = mtlx;
    while (true) {
        const std::string& inherit = current->getAttribute(inheritAttr);
        if (inherit.empty()) {
            break;
        }
        if (auto inherited = current->getRoot()->getChild(inherit)) {
            current = inherited;
            if (current->getName().size() < shortestName->size()) {
                shortestName = &current->getName();
            }
        }
        else {
            break;
        }
    }

    // Populate mapping:
    auto r = mapping->emplace(mtlx->getName(), *shortestName);
    // If shortestName is shorter than the existing name, replace it.
    if (!r.second && shortestName->size() < r.first->second.size()) {
        r.first->second = *shortestName;
    }
    while (true) {
        const std::string& inherit = mtlx->getAttribute(inheritAttr);
        if (inherit.empty()) {
            break;
        }
        if (auto inherited = mtlx->getRoot()->getChild(inherit)) {
            mtlx = inherited;
            auto r = mapping->emplace(mtlx->getName(), *shortestName);
            // If shortestName is shorter than the existing name, replace it.
            if (!r.second && shortestName->size() < r.first->second.size()) {
                r.first->second = *shortestName;
            }
        }
        else {
            break;
        }
    }
}

// Choose an Sdr name based on compatible MaterialX nodedef names.
_NameMapping
_ComputeNameMapping(const mx::ConstDocumentPtr& doc)
{
    TRACE_FUNCTION();

    _NameMapping result;

    // For each nodeDef with an inheritance chain, we populate the 
    // _NameMapping with the shortest name found in the inheritance
    // hierarchy
    //
    //    mix_float_210 (v2.1)
    //      inherits mix_float_200 (v2.0)
    //        inherits mix_float (original version)
    //
    // A versioning inheritance can also choose to keep the latest version with
    // the official name, and tag the earlier versions:
    //
    //    mix_float  (v2.1 latest)
    //      inherits mix_float_200  (v2.0)
    //        inherits mix_float_100  (v1.0)
    //
    // So we need to traverse the hierarchy, and at each point pick the
    // shortest name.
    for (auto&& mtlxNodeDef: doc->getNodeDefs()) {
        if (mtlxNodeDef->hasInheritString()) {
            _MapNodeNamesToBaseForVersioning(mtlxNodeDef, &result);
        }
    }

    return result;
}

// Return the Sdr name for a nodedef name.
std::string
_ChooseName(const std::string& nodeDefName, const _NameMapping& nameMapping)
{
    TRACE_FUNCTION();

    auto i = nameMapping.find(nodeDefName);
    return i == nameMapping.end() ? nodeDefName : i->second;
}

static
void
_DiscoverNodes(
    SdrShaderNodeDiscoveryResultVec* result,
    const mx::ConstDocumentPtr& doc,
    const SdrDiscoveryUri& fileResult,
    const _NameMapping& nameMapping)
{
    TRACE_FUNCTION();

    static const TfToken family = TfToken();

    // Get the node definitions
    for (auto&& nodeDef: doc->getNodeDefs()) {
        bool implicitDefault;
        result->emplace_back(
            SdrIdentifier(nodeDef->getName()),
            UsdMtlxGetVersion(nodeDef, &implicitDefault),
            _ChooseName(nodeDef->getName(), nameMapping),
            TfToken(nodeDef->getNodeString()),
            _tokens->discoveryType,
            _tokens->discoveryType,
            fileResult.uri,
            fileResult.resolvedUri
        );
    }
}

} // anonymous namespace

/// Discovers nodes in MaterialX files.
class UsdMtlxDiscoveryPlugin : public SdrDiscoveryPlugin {
public:
    UsdMtlxDiscoveryPlugin();
    ~UsdMtlxDiscoveryPlugin() override = default;

    /// Discover all of the nodes that appear within the the search paths
    /// provided and match the extensions provided.
    SdrShaderNodeDiscoveryResultVec DiscoverShaderNodes(const Context&) override;

    /// Gets the paths that this plugin is searching for nodes in.
    const SdrStringVec& GetSearchURIs() const override;

private:
    /// The paths (abs) indicating where the plugin should search for nodes.
    SdrStringVec _customSearchPaths;
    SdrStringVec _allSearchPaths;
};

UsdMtlxDiscoveryPlugin::UsdMtlxDiscoveryPlugin()
{
    TRACE_FUNCTION();

    _customSearchPaths = UsdMtlxCustomSearchPaths();
    _allSearchPaths = UsdMtlxSearchPaths();
}

SdrShaderNodeDiscoveryResultVec
UsdMtlxDiscoveryPlugin::DiscoverShaderNodes(const Context& context)
{
    TRACE_FUNCTION();

    SdrShaderNodeDiscoveryResultVec result;

    // Merge all MaterialX standard library files into a single document.
    //
    // These files refer to elements in each other but they're not
    // all included by a single document.  We could construct such
    // a document in memory and parse it but instead we choose to
    // read each document separately and merge them.
    if (auto document = UsdMtlxGetDocument("")) {
        // Identify as the standard library
        _DiscoverNodes(&result, document, {"mtlx", "mtlx"},
                       _ComputeNameMapping(document));
    }

    // Find the mtlx files from other search paths.
    for (auto&& fileResult:
            SdrFsHelpersDiscoverFiles(
                _customSearchPaths,
                UsdMtlxStandardFileExtensions(),
                TfGetenvBool("USDMTLX_PLUGIN_FOLLOW_SYMLINKS", false))) {
        if (auto document = UsdMtlxGetDocument(fileResult.resolvedUri)) {
            _DiscoverNodes(&result, document, fileResult,
                           _ComputeNameMapping(document));
        }
    }

    return result;
}

const SdrStringVec&
UsdMtlxDiscoveryPlugin::GetSearchURIs() const
{
    TRACE_FUNCTION();

    return _allSearchPaths;
}

SDR_REGISTER_DISCOVERY_PLUGIN(UsdMtlxDiscoveryPlugin)

PXR_NAMESPACE_CLOSE_SCOPE
