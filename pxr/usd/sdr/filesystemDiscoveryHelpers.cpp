//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverScopedCache.h"
#include "pxr/usd/ndr/filesystemDiscoveryHelpers.h"
#include "pxr/usd/sdr/debugCodes.h"
#include "pxr/usd/sdr/discoveryPlugin.h"
#include "pxr/usd/sdr/filesystemDiscoveryHelpers.h"

#include <algorithm>
#include <cctype>

PXR_NAMESPACE_OPEN_SCOPE

bool
SdrFsHelpersSplitShaderIdentifier(
    const TfToken &identifier, 
    TfToken *family,
    TfToken *name,
    SdrVersion *version)
{
    NdrVersion ndrVersion = SdrToNdrVersion(*version);
    bool res = NdrFsHelpersSplitShaderIdentifier(
        identifier,
        family,
        name,
        &ndrVersion);
    *version = NdrToSdrVersion(ndrVersion);
    return res;
}

SdrShaderNodeDiscoveryResultVec
SdrFsHelpersDiscoverShaderNodes(
    const SdrStringVec& searchPaths,
    const SdrStringVec& allowedExtensions,
    bool followSymlinks,
    const SdrDiscoveryPluginContext* context,
    const SdrParseIdentifierFn &parseIdentifierFn)
{
    NdrParseIdentifierFn ndrParseIdentifierFn = [&parseIdentifierFn](
        const TfToken &identifier, 
        TfToken *family,
        TfToken *name,
        NdrVersion *version) {
            SdrVersion sdrVersion = NdrToSdrVersion(*version);
            bool res = parseIdentifierFn(identifier,
                                     family,
                                     name,
                                     &sdrVersion);
            *version = SdrToNdrVersion(sdrVersion);
            return res;
        };

    NdrNodeDiscoveryResultVec ndrResults = NdrFsHelpersDiscoverNodes(
        searchPaths,
        allowedExtensions,
        followSymlinks,
        context,
        ndrParseIdentifierFn
    );

    SdrShaderNodeDiscoveryResultVec foundNodes;
    for (const NdrNodeDiscoveryResult& result : ndrResults) {
        SdrShaderNodeDiscoveryResult sdrResult =
            SdrShaderNodeDiscoveryResult::FromNdrNodeDiscoveryResult(result);
        foundNodes.push_back(sdrResult);
    }
    return foundNodes;
}

SdrDiscoveryUriVec
SdrFsHelpersDiscoverFiles(
    const SdrStringVec& searchPaths,
    const SdrStringVec& allowedExtensions,
    bool followSymlinks)
{
    NdrDiscoveryUriVec ndrUris = NdrFsHelpersDiscoverFiles(
        searchPaths, allowedExtensions, followSymlinks);
    
    SdrDiscoveryUriVec uris;
    for (const NdrDiscoveryUri& uri : ndrUris) {
        uris.push_back(
            {uri.uri, uri.resolvedUri});
    }
    return uris;
}

PXR_NAMESPACE_CLOSE_SCOPE
