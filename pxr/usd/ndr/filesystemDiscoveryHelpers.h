//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_NDR_FILESYSTEM_DISCOVERY_HELPERS_H
#define PXR_USD_NDR_FILESYSTEM_DISCOVERY_HELPERS_H

/// \file ndr/filesystemDiscoveryHelpers.h
///
/// \deprecated
/// All Ndr objects are deprecated in favor of the corresponding Sdr objects
/// in sdr/filesystemDiscoveryHelpers.h

#include "pxr/pxr.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/usd/ndr/api.h"
#include "pxr/usd/ndr/declare.h"
#include "pxr/usd/ndr/nodeDiscoveryResult.h"

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

class NdrDiscoveryPluginContext;

/// \file filesystemDiscoveryHelpers.h
///
/// Provides utilities that the default filesystem discovery plugin uses. If
/// a custom filesystem discovery plugin is needed, these can be used to fill
/// in a large chunk of the functionality.
///

/// \deprecated in favor of PXR_SDR_FS_PLUGIN_SEARCH_PATHS
NDR_API
extern TfEnvSetting<std::string> PXR_NDR_FS_PLUGIN_SEARCH_PATHS;

/// \deprecated in favor of PXR_SDR_FS_PLUGIN_SEARCH_PATHS
NDR_API
extern TfEnvSetting<std::string> PXR_NDR_FS_PLUGIN_ALLOWED_EXTS;

/// \deprecated in favor of PXR_NDR_FS_PLUGIN_FOLLOW_SYMLINKS
NDR_API
extern TfEnvSetting<bool> PXR_NDR_FS_PLUGIN_FOLLOW_SYMLINKS;

/// Type of a function that can be used to parse a discovery result's identifier
/// into its family, name, and version.
using NdrParseIdentifierFn = std::function<
    bool (const TfToken &identifier, 
          TfToken *family,
          TfToken *name,
          NdrVersion *version)>;

/// Given a shader's \p identifier token, computes the corresponding 
/// NdrNode's family name, implementation name and shader version 
/// (as NdrVersion).
/// 
/// * \p family is the prefix of \p identifier up to and not 
/// including the first underscore. 
/// * \p version is the suffix of \p identifier comprised of one or 
/// two integers representing the major and minor version numbers.
/// * \p name is the string we get by joining 
/// <i>family</i> with everything that's in between <i>family</i> 
/// and <i>version</i> with an underscore.
/// 
/// Returns true if \p identifier is valid and was successfully split 
/// into the different components. 
/// 
/// \note The python version of this function returns a tuple containing
/// (famiyName, implementationName, version).
///
/// \deprecated
/// Deprecated in favor of SdrFsHelpersSplitShaderIdentifier
NDR_API 
bool
NdrFsHelpersSplitShaderIdentifier(
    const TfToken &identifier, 
    TfToken *family,
    TfToken *name,
    NdrVersion *version);

/// Returns a vector of discovery results that have been found while walking
/// the given search paths.
///
/// Each path in \p searchPaths is walked recursively, optionally following 
/// symlinks if \p followSymlinks is true, looking for files that match one of 
/// the provided \p allowedExtensions. These files are represented in the 
/// discovery results that are returned.
///
/// The identifier for each discovery result is the base name of the represented
/// file with the extension removed. The \p parseIdentifierFn is used to parse 
/// the family, name, and version from the identifier that will set in the 
/// file's discovery result. By default, NdrFsHelpersSplitShaderIdentifier is 
/// used to parse the identifier, but the family/name/version parsing behavior 
/// can be changed by passing a custom parseIdentifierFn. Any identifiers that 
/// cannot be parsed by whatever the parseIdentifierFn will be considered
/// invalid and not added as a discovery result. Note that the version for 
/// every discovery result returned by this function will be naively marked as 
/// being default even if multiple versions with the same name are found.
///
/// \deprecated
/// Deprecated in favor of SdrFsHelpersDiscoverShaderNodes
NDR_API
NdrNodeDiscoveryResultVec
NdrFsHelpersDiscoverNodes(
    const NdrStringVec& searchPaths,
    const NdrStringVec& allowedExtensions,
    bool followSymlinks = true,
    const NdrDiscoveryPluginContext* context = nullptr,
    const NdrParseIdentifierFn &parseIdentifierFn = 
        NdrFsHelpersSplitShaderIdentifier
);

/// Struct for holding a URI and its resolved URI for a file discovered
/// by NdrFsHelpersDiscoverFiles
///
/// \deprecated
/// Deprecated in favor of SdrDiscoveryUri
struct NdrDiscoveryUri 
{
    std::string uri;
    std::string resolvedUri;
};

/// A vector of URI/resolved URI structs.
using NdrDiscoveryUriVec = std::vector<NdrDiscoveryUri>;

/// Returns a vector of discovered URIs (as both the unresolved URI and the 
/// resolved URI) that are found while walking  the given search paths.
///
/// Each path in \p searchPaths is walked recursively, optionally following 
/// symlinks if \p followSymlinks is true, looking for files that match one of 
/// the provided \p allowedExtensions. These files' unresolved and resolved URIs
/// are returned in the result vector.
///
/// This is an alternative to NdrFsHelpersDiscoverNodes for discovery plugins 
/// that want to search for files that are not meant to be returned by discovery
/// themselves, but can be parsed to generate the discovery results.
///
/// \deprecated
/// Deprecated in favor of SdrFsHelpersDiscoverFiles
NDR_API
NdrDiscoveryUriVec
NdrFsHelpersDiscoverFiles(
    const NdrStringVec& searchPaths,
    const NdrStringVec& allowedExtensions,
    bool followSymlinks = true
);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_NDR_FILESYSTEM_DISCOVERY_HELPERS_H
