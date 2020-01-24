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

PXR_NAMESPACE_OPEN_SCOPE

namespace {

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
bool
FsHelpersExamineFiles(
    NdrNodeDiscoveryResultVec* foundNodes,
    NdrStringSet* foundNodesWithTypes,
    const NdrStringVec& allowedExtensions,
    const NdrDiscoveryPluginContext* context,
    const std::string& dirPath,
    const NdrStringVec& dirFileNames)
{
    for (const std::string& fileName : dirFileNames) {
        std::string extension = TfStringToLower(TfGetExtension(fileName));

        // Does the extension match one of the known-good extensions?
        NdrStringVec::const_iterator extIter = std::find(
            allowedExtensions.begin(),
            allowedExtensions.end(),
            extension
        );

        if (extIter != allowedExtensions.end()) {
            // Found a node file w/ allowed extension
            std::string uri = TfStringCatPaths(dirPath, fileName);
            std::string identifier = TfStringGetBeforeSuffix(fileName, '.');
            std::string identifierAndType = identifier + "-" + extension;

            // Don't allow duplicates. A "duplicate" is considered to be a node
            // with the same name AND discovery type.
            if (!foundNodesWithTypes->insert(identifierAndType).second) {
                TF_DEBUG(NDR_DISCOVERY).Msg(
                    "Found a duplicate node with identifier [%s] "
                    "and type [%s] at URI [%s]; ignoring.", 
                    identifier.c_str(), extension.c_str(), uri.c_str());
                continue;
            }

            const auto discoveryType = TfToken(extension);
            foundNodes->emplace_back(
                // Identifier
                NdrIdentifier(identifier),

                // Version.  Use a default version for the benefit of
                // naive clients.
                NdrVersion().GetAsDefault(),

                // Name
                identifier,

                // Family
                TfToken(),

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

} // anonymous namespace

NdrNodeDiscoveryResultVec
NdrFsHelpersDiscoverNodes(
    const NdrStringVec& searchPaths,
    const NdrStringVec& allowedExtensions,
    bool followSymlinks,
    const NdrDiscoveryPluginContext* context)
{
    NdrNodeDiscoveryResultVec foundNodes;

    // A map with compound keys (<name>-<type>) indicating the nodes that have
    // been found so far (a key could be, for example, 'Mix-oso')
    NdrStringSet foundNodesWithTypes;

    // Cache the calls to Ar's `Resolve()`
    ArResolverScopedCache resolverCache;

    for (const std::string& searchPath : searchPaths) {
        if (!TfIsDir(searchPath)) {
            continue;
        }

        TfWalkDirs(
            searchPath,
            std::bind(
                &FsHelpersExamineFiles,
                &foundNodes,
                &foundNodesWithTypes,
                std::ref(allowedExtensions),
                context,
                std::placeholders::_1,
                std::placeholders::_3
            ),
            /* topDown = */ true,
            TfWalkIgnoreErrorHandler,
            followSymlinks
        );
    }

    return foundNodes;
}

PXR_NAMESPACE_CLOSE_SCOPE
