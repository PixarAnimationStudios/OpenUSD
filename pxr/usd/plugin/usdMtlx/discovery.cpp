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
#include "pxr/usd/plugin/usdMtlx/utils.h"
#include "pxr/usd/ndr/declare.h"
#include "pxr/usd/ndr/discoveryPlugin.h"
#include "pxr/usd/ndr/filesystemDiscoveryHelpers.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include <algorithm>
#include <cctype>

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((discoveryType, "mtlx"))
);

// Maps a nodedef name to its NdrNode name.
using _NameMapping = std::map<std::string, std::string>;

// Fill the name mapping with the shortest name found in the inheritance
// hierarchy:
void _FindAncestralMappings(mx::ConstElementPtr mtlx, _NameMapping& mappping)
{
    static const std::string inheritAttr("inherit");

    const std::string* shortestName = &mtlx->getName();

    // Find shortest:
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
    mappping.emplace(mtlx->getName(), *shortestName);
    while (true) {
        const std::string& inherit = mtlx->getAttribute(inheritAttr);
        if (inherit.empty()) {
            break;
        }
        if (auto inherited = mtlx->getRoot()->getChild(inherit)) {
            mtlx = inherited;
            mappping.emplace(mtlx->getName(), *shortestName);
        }
        else {
            break;
        }
    }
}

// Choose an Ndr name based on compatible MaterialX nodedef names.
_NameMapping
_ComputeNameMapping(const mx::ConstDocumentPtr& doc)
{
    _NameMapping result;

    // We use the simple heuristic of using the name of the top-most
    // nodedef on the inheritance chain where top-most is the one
    // that doesn't itself inherit anything.  The 1.36 spec gives
    // guidance that this should be sufficient.
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
            _FindAncestralMappings(mtlxNodeDef, result);
        }
    }

    return result;
}

// Return the Ndr name for a nodedef name.
std::string
_ChooseName(const std::string& nodeDefName, const _NameMapping& nameMapping)
{
    auto i = nameMapping.find(nodeDefName);
    return i == nameMapping.end() ? nodeDefName : i->second;
}

static
void
_DiscoverNodes(
    NdrNodeDiscoveryResultVec* result,
    const mx::ConstDocumentPtr& doc,
    const NdrNodeDiscoveryResult& fileResult,
    const _NameMapping& nameMapping)
{
    static const TfToken family = TfToken();

    // Get the node definitions
    for (auto&& nodeDef: doc->getNodeDefs()) {
        bool implicitDefault;
        result->emplace_back(
            NdrIdentifier(nodeDef->getName()),
            UsdMtlxGetVersion(nodeDef, &implicitDefault),
            _ChooseName(nodeDef->getName(), nameMapping),
            TfToken(nodeDef->getNodeString()),
            fileResult.discoveryType,
            fileResult.sourceType,
            fileResult.uri,
            fileResult.resolvedUri
        );
    }
}

} // anonymous namespace

/// Discovers nodes in MaterialX files.
class UsdMtlxDiscoveryPlugin : public NdrDiscoveryPlugin {
public:
    UsdMtlxDiscoveryPlugin();
    ~UsdMtlxDiscoveryPlugin() override = default;

    /// Discover all of the nodes that appear within the the search paths
    /// provided and match the extensions provided.
    NdrNodeDiscoveryResultVec DiscoverNodes(const Context&) override;

    /// Gets the paths that this plugin is searching for nodes in.
    const NdrStringVec& GetSearchURIs() const override;

private:
    /// The paths (abs) indicating where the plugin should search for nodes.
    NdrStringVec _searchPaths;
    NdrStringVec _allSearchPaths;
};

UsdMtlxDiscoveryPlugin::UsdMtlxDiscoveryPlugin()
{
    static const auto searchPaths =
        UsdMtlxGetSearchPathsFromEnvVar("PXR_USDMTLX_PLUGIN_SEARCH_PATHS");

    _searchPaths = searchPaths;
    _allSearchPaths =
        UsdMtlxMergeSearchPaths(_searchPaths, UsdMtlxStandardLibraryPaths());
}

NdrNodeDiscoveryResultVec
UsdMtlxDiscoveryPlugin::DiscoverNodes(const Context& context)
{
    NdrNodeDiscoveryResultVec result;

    // Merge all MaterialX standard library files into a single document.
    //
    // These files refer to elements in each other but they're not
    // all included by a single document.  We could construct such
    // a document in memory and parse it but instead we choose to
    // read each document separately and merge them.
    if (auto document = UsdMtlxGetDocument("")) {
        auto standardResult =
            NdrNodeDiscoveryResult(
                NdrIdentifier(),// identifier unused
                NdrVersion(),   // version unused
                "",             // name unused
                TfToken(),      // family unused
                _tokens->discoveryType,
                _tokens->discoveryType,
                "mtlx",
                "mtlx"          // identify as the standard library
            );
        _DiscoverNodes(&result, document, standardResult,
                       _ComputeNameMapping(document));
    }

    // Find the mtlx files from other search paths.
    for (auto&& fileResult:
            NdrFsHelpersDiscoverNodes(
                _searchPaths,
                UsdMtlxStandardFileExtensions(),
                TfGetenvBool("USDMTLX_PLUGIN_FOLLOW_SYMLINKS", false))) {
        if (auto document = UsdMtlxGetDocument(fileResult.resolvedUri)) {
            _DiscoverNodes(&result, document, fileResult,
                           _ComputeNameMapping(document));
        }
    }

    return result;
}

const NdrStringVec&
UsdMtlxDiscoveryPlugin::GetSearchURIs() const
{
    return _allSearchPaths;
}

NDR_REGISTER_DISCOVERY_PLUGIN(UsdMtlxDiscoveryPlugin)

PXR_NAMESPACE_CLOSE_SCOPE
