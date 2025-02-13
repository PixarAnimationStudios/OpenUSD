//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_SDR_FILESYSTEM_DISCOVERY_HELPERS_H
#define PXR_USD_SDR_FILESYSTEM_DISCOVERY_HELPERS_H

/// \file sdr/filesystemDiscoveryHelpers.h
///
/// \note
/// All Ndr objects are deprecated in favor of the corresponding Sdr objects
/// in this file. All existing pxr/usd/ndr implementations will be moved to
/// pxr/usd/sdr.

#include "pxr/pxr.h"
#include "pxr/usd/sdr/api.h"
#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/ndr/filesystemDiscoveryHelpers.h"
#include "pxr/usd/sdr/shaderNodeDiscoveryResult.h"

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

class SdrDiscoveryPluginContext;

/// \file filesystemDiscoveryHelpers.h
///
/// Provides utilities that the default filesystem discovery plugin uses. If
/// a custom filesystem discovery plugin is needed, these can be used to fill
/// in a large chunk of the functionality.
///

/// Type of a function that can be used to parse a discovery result's identifier
/// into its family, name, and version.
using SdrParseIdentifierFn = std::function<
    bool (const TfToken &identifier, 
          TfToken *family,
          TfToken *name,
          SdrVersion *version)>;

/// Given a shader's \p identifier token, computes the corresponding 
/// SdrShaderNode's family name, implementation name and shader version 
/// (as SdrVersion).
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
SDR_API 
bool
SdrFsHelpersSplitShaderIdentifier(
    const TfToken &identifier, 
    TfToken *family,
    TfToken *name,
    SdrVersion *version);

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
/// file's discovery result. By default, SdrFsHelpersSplitShaderIdentifier is 
/// used to parse the identifier, but the family/name/version parsing behavior 
/// can be changed by passing a custom parseIdentifierFn. Any identifiers that 
/// cannot be parsed by whatever the parseIdentifierFn will be considered
/// invalid and not added as a discovery result. Note that the version for 
/// every discovery result returned by this function will be naively marked as 
/// being default even if multiple versions with the same name are found.
SDR_API
SdrShaderNodeDiscoveryResultVec
SdrFsHelpersDiscoverShaderNodes(
    const SdrStringVec& searchPaths,
    const SdrStringVec& allowedExtensions,
    bool followSymlinks = true,
    const SdrDiscoveryPluginContext* context = nullptr,
    const SdrParseIdentifierFn &parseIdentifierFn = 
        SdrFsHelpersSplitShaderIdentifier
);

/// Struct for holding a URI and its resolved URI for a file discovered
/// by SdrFsHelpersDiscoverFiles
struct SdrDiscoveryUri 
{
    std::string uri;
    std::string resolvedUri;
};

/// A vector of URI/resolved URI structs.
using SdrDiscoveryUriVec = std::vector<SdrDiscoveryUri>;

/// Returns a vector of discovered URIs (as both the unresolved URI and the 
/// resolved URI) that are found while walking  the given search paths.
///
/// Each path in \p searchPaths is walked recursively, optionally following 
/// symlinks if \p followSymlinks is true, looking for files that match one of 
/// the provided \p allowedExtensions. These files' unresolved and resolved URIs
/// are returned in the result vector.
///
/// This is an alternative to SdrFsHelpersDiscoverNodes for discovery plugins 
/// that want to search for files that are not meant to be returned by discovery
/// themselves, but can be parsed to generate the discovery results.
SDR_API
SdrDiscoveryUriVec
SdrFsHelpersDiscoverFiles(
    const SdrStringVec& searchPaths,
    const SdrStringVec& allowedExtensions,
    bool followSymlinks = true
);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDR_FILESYSTEM_DISCOVERY_HELPERS_H
