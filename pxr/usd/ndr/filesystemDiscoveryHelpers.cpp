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

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverScopedCache.h"
#include "pxr/usd/ndr/debugCodes.h"
#include "pxr/usd/ndr/discoveryPlugin.h"
#include "pxr/usd/ndr/filesystemDiscoveryHelpers.h"

#include <algorithm>
#include <cctype>

PXR_NAMESPACE_OPEN_SCOPE

// Examines the specified set of files, and determines if any of the files
// are candidates for being parsed into a node. If a file is determined
// to be a candidate, it is appended to \p foundNodes and
// \p foundNodesWithTypes.
// \param[out] foundNodes The nodes that were discovered
// \param[out] foundNodesWithTypes The identifiers of the nodes that were
//     discovered, along with their types (key format is '<id>-<type>')
// \param[in]  dirPath The abs path to the directory to examine
// \param[in]  dirFileNames The file names in the \p dirPath dir to test
// \return `true` if the search should continue on to other paths in the
//         search path
static bool
_FsHelpersExamineFiles(
    NdrNodeDiscoveryResultVec* foundNodes,
    NdrStringSet* foundNodesWithTypes,
    const NdrStringVec& allowedExtensions,
    const NdrDiscoveryPluginContext* context,
    const std::string& dirPath,
    const NdrStringVec& dirFileNames,
    const NdrParseIdentifierFn &parseIdentifierFn)
{
    for (const std::string& fileName : dirFileNames) {
        std::string extension = TfStringToLowerAscii(TfGetExtension(fileName));

        // Does the extension match one of the known-good extensions?
        NdrStringVec::const_iterator extIter = std::find(
            allowedExtensions.begin(),
            allowedExtensions.end(),
            extension
        );

        if (extIter != allowedExtensions.end()) {
            // Found a node file w/ allowed extension
            std::string uri = TfStringCatPaths(dirPath, fileName);
            TfToken identifier(TfStringGetBeforeSuffix(fileName, '.'));
            std::string identifierAndType = 
                identifier.GetString() + "-" + extension;

            // Don't allow duplicates. A "duplicate" is considered to be a node
            // with the same name AND discovery type.
            if (!foundNodesWithTypes->insert(identifierAndType).second) {
                TF_DEBUG(NDR_DISCOVERY).Msg(
                    "Found a duplicate node with identifier [%s] "
                    "and type [%s] at URI [%s]; ignoring.\n", 
                    identifier.GetText(), extension.c_str(), uri.c_str());
                continue;
            }

            TfToken family, name;
            NdrVersion version;
            const bool parsed = parseIdentifierFn ?
                parseIdentifierFn(identifier, &family, &name, &version) :
                NdrFsHelpersSplitShaderIdentifier(
                    identifier, &family, &name, &version);
            if (!parsed) {
                TF_WARN("Could not parse the family, name, and version "
                        "from shader indentifier '%s' for shader file '%s'. "
                        "Skipping.", 
                        identifier.GetText(), uri.c_str());
                continue;
            }

            const auto discoveryType = TfToken(extension);
            foundNodes->emplace_back(
                // Identifier
                identifier,

                // Version.  Use a default version for the benefit of
                // naive clients.
                version.GetAsDefault(),

                // Name
                name,

                // Family
                family,

                // Discovery type
                discoveryType,

                // Source type
                context ? context->GetSourceType(discoveryType) : TfToken(),

                // URI
                uri,

                // Resolved URI
                ArGetResolver().Resolve(uri)
            );
        }
    }

    // Continue walking directories
    return true;
}

static bool 
_IsNumber(const std::string& s)
{
    return !s.empty() &&
        std::find_if(s.begin(), s.end(),
                     [](unsigned char c) { return !std::isdigit(c); })
        == s.end();
}

bool
NdrFsHelpersSplitShaderIdentifier(
    const TfToken &identifier, 
    TfToken *family,
    TfToken *name,
    NdrVersion *version)
{
    const std::vector<std::string> tokens = 
        TfStringTokenize(identifier.GetString(), "_");

    if (tokens.empty()) {
        return false;
    }

    *family = TfToken(tokens[0]);

    if (tokens.size() == 1) {
        *family = identifier;
        *name = identifier;
        *version = NdrVersion();
        return true;
    }

    if (tokens.size() == 2) {
        if (_IsNumber(tokens.back())) {
            const int major = std::stoi(tokens.back());
            *version = NdrVersion(major);
            *name = *family;
        } else {
            *version = NdrVersion();
            *name = identifier;
        }
        return true;
    } 

    const bool lastTokenIsNumber = _IsNumber(*(tokens.end() - 1));
    const bool penultimateTokenIsNumber = _IsNumber(*(tokens.end() - 2));

    if (penultimateTokenIsNumber) {
        if (!lastTokenIsNumber) {
            TF_WARN("Invalid shader identifier '%s'.", identifier.GetText()); 
            return false;
        }
        // Has a major and minor version
        *version = NdrVersion(std::stoi(*(tokens.end() - 2)), 
                              std::stoi(*(tokens.end() - 1)));
        *name = TfToken(TfStringJoin(tokens.begin(), tokens.end() - 2, "_"));
    } else if (lastTokenIsNumber) {
        // Has just a major version
        *version = NdrVersion(std::stoi(tokens[tokens.size()-1]));
        *name = TfToken(TfStringJoin(tokens.begin(), tokens.end() - 1, "_"));
    } else {
        // No version information is available. 
        *name = identifier;
        *version = NdrVersion();
    }

    return true;
}

static void
_WalkDirs(const NdrStringVec &searchPaths,
          const TfWalkFunction &fn,
          bool followSymlinks)
{
    for (const std::string& searchPath : searchPaths) {
        if (!TfIsDir(searchPath)) {
            continue;
        }

        TfWalkDirs(
            searchPath,
            fn,
            /* topDown = */ true,
            TfWalkIgnoreErrorHandler,
            followSymlinks
        );
    }
}

NdrNodeDiscoveryResultVec
NdrFsHelpersDiscoverNodes(
    const NdrStringVec& searchPaths,
    const NdrStringVec& allowedExtensions,
    bool followSymlinks,
    const NdrDiscoveryPluginContext* context,
    const NdrParseIdentifierFn &parseIdentifierFn)
{
    NdrNodeDiscoveryResultVec foundNodes;

    // A map with compound keys (<name>-<type>) indicating the nodes that have
    // been found so far (a key could be, for example, 'Mix-oso')
    NdrStringSet foundNodesWithTypes;

    // Cache the calls to Ar's `Resolve()`
    ArResolverScopedCache resolverCache;

    auto discoverNodesFn = [&](const std::string& dirPath,
                               NdrStringVec *unused, 
                               const NdrStringVec& dirFileNames) {
        return _FsHelpersExamineFiles(
            &foundNodes, &foundNodesWithTypes, allowedExtensions,
            context, dirPath, dirFileNames, parseIdentifierFn);
    };

    _WalkDirs(searchPaths, discoverNodesFn, followSymlinks);

    return foundNodes;
}

NdrDiscoveryUriVec
NdrFsHelpersDiscoverFiles(
    const NdrStringVec& searchPaths,
    const NdrStringVec& allowedExtensions,
    bool followSymlinks)
{
    NdrDiscoveryUriVec foundUris;

    // Cache the calls to Ar's `Resolve()`
    ArResolverScopedCache resolverCache;

    auto findUrisFn = [&](const std::string& dirPath,
                          NdrStringVec *unused, 
                          const NdrStringVec& dirFileNames) {

        for (const std::string& fileName : dirFileNames) {
            const std::string extension = 
                TfStringToLowerAscii(TfGetExtension(fileName));

            // Does the extension match one of the known-good extensions?
            if (std::find(allowedExtensions.begin(), allowedExtensions.end(), 
                          extension) != allowedExtensions.end()) {
                // Found a node file w/ allowed extension
                NdrDiscoveryUri discoveryUri;
                discoveryUri.uri = TfStringCatPaths(dirPath, fileName);
                discoveryUri.resolvedUri = 
                    ArGetResolver().Resolve(discoveryUri.uri);
                foundUris.push_back(std::move(discoveryUri));
            }
        }

        // Continue walking directories
        return true;
    };

    _WalkDirs(searchPaths, findUrisFn, followSymlinks);

    return foundUris;
}

PXR_NAMESPACE_CLOSE_SCOPE
