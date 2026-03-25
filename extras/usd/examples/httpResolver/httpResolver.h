//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#pragma once

#include "packageData.h"

#include "pxr/usd/ar/defaultResolver.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/timestamp.h"

#include <tbb/concurrent_hash_map.h>

#include <memory>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HttpResolver
///
/// Example URI resolver that handles http:// and https:// asset paths.
///
/// HTTP transport is provided by one of two compile-time backends:
///   - Foundation/NSURL  (default on Apple platforms)
///   - libcurl           (default on Linux/Windows; selectable on Apple via
///                        -DPXR_HTTP_BACKEND=CURL)
///
/// Optional compile-time features (see CMakeLists.txt):
///   PXR_HTTP_USDZ_STREAMING  — byte-range streaming for USDZ sub-assets (default ON)
///   PXR_HTTP_CACHE_MODE_DISK — persist downloads to disk instead of memory
///   PXR_HTTP_SERVER_FILEPATH — accept server-redirected local file paths
///
class HttpResolver final : public ArDefaultResolver {
public:
    HttpResolver();
    ~HttpResolver() override;

protected:
    // ------------------------------------------------------------------ //
    // ArResolver overrides — all platform-agnostic, live in httpResolver.cpp
    // ------------------------------------------------------------------ //

    std::string _CreateIdentifier(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) const override;

    std::string _CreateIdentifierForNewAsset(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) const override;

    ArResolvedPath _Resolve(
        const std::string& assetPath) const override;

    ArResolvedPath _ResolveForNewAsset(
        const std::string& assetPath) const override;

    std::shared_ptr<ArAsset> _OpenAsset(
        const ArResolvedPath& resolvedPath) const override;

    /// Read-only resolver; write operations are not supported.
    std::shared_ptr<ArWritableAsset> _OpenAssetForWrite(
        const ArResolvedPath& resolvedPath,
        WriteMode writeMode) const override
    { return nullptr; }

    ArTimestamp _GetModificationTimestamp(
        const std::string& path,
        const ArResolvedPath& resolvedPath) const override;

    std::string _GetExtension(
        const std::string& path) const override;

private:
    // ------------------------------------------------------------------ //
    // Backend methods — declared here, implemented in exactly one of:
    //   httpResolver_Curl.cpp  or  httpResolver_Foundation.mm
    // ------------------------------------------------------------------ //

    /// Return true if \p path looks like an absolute http(s) URL.
    bool _IsUrl(const std::string& path) const;

    /// Return the file extension of \p url, stripping any query string.
    std::string _GetExtensionFromUrl(const std::string& url) const;

    /// Resolve \p assetPath relative to \p anchorPath and return a fully
    /// qualified URL.  For a .usdz anchor, the sub-resource is encoded as
    /// a "resource=" query parameter.
    /// Sets \p requestPackageContentsResolve to true when the package table
    /// of contents must be fetched before a final identifier can be formed.
    std::string _MakeFullyQualifiedURL(
        const std::string& assetPath,
        const std::string& anchorPath,
        bool& requestPackageContentsResolve) const;

    /// Issue an HTTP HEAD/GET and return the Last-Modified timestamp as
    /// seconds since epoch (0.0 if unavailable).
    double _GetModificationTimeFromServer(const std::string& url) const;

    /// Download the entire content of \p url.
    /// On success the buffer is populated.
    /// When PXR_HTTP_CACHE_MODE_DISK is defined the data is written to disk
    /// and \p filePath is set to that path; the returned buffer may be empty.
    /// When PXR_HTTP_SERVER_FILEPATH is defined and the server responds with
    /// Content-Type: application/x-usd-filepath the buffer will be empty and
    /// \p filePath will hold the server-provided local path.
    std::shared_ptr<std::vector<char>> _FetchFile(
        const std::string& url,
        std::string& filePath,
        bool& status) const;

    /// Download a byte range [\p start, \p end) of \p url.
    /// A negative \p start (e.g. -128) requests that many bytes from the end.
    /// When \p closedRange is false only \p start is used (suffix request).
    /// When \p checkCache is true the download cache is consulted first.
    std::shared_ptr<std::vector<char>> _FetchByteRange(
        const std::string& url,
        int32_t start,
        int32_t end,
        bool closedRange,
        bool checkCache,
        std::string& filePath,
        bool& status) const;

    // ------------------------------------------------------------------ //
    // Platform-agnostic helpers — implemented in httpResolver.cpp
    // ------------------------------------------------------------------ //

    /// Parse the USDZ central directory and populate _packageInfoCache.
    bool _GetPackageContents(const std::string& url) const;

    /// Given a downloaded byte range \p buf, locate the local file header
    /// for \p resource and return the offset to the start of its data.
    uint32_t _GetResourceOffset(
        const std::string& resource,
        const std::shared_ptr<std::vector<char>>& buf,
        bool& status) const;

    /// Look up byte-range info for a packaged sub-asset encoded as
    /// "baseUrl?resource=name".  Returns false if the path is not a
    /// streaming USDZ sub-asset or if the resource is not in the cache.
    bool _FetchPackageStreamInfo(
        const std::string& assetPath,
        std::string& resource,
        uint32_t& start,
        uint32_t& end) const;

    // ------------------------------------------------------------------ //
    // Shared caches
    // ------------------------------------------------------------------ //

    /// USDZ package structure cache: stripped URL → HttpPackageInfo.
    static tbb::concurrent_hash_map<std::string, HttpPackageInfo> _packageInfoCache;

    /// In-memory download cache: URL (or "URL|start-end" for ranges) → bytes.
    /// Only used when HTTP_CACHE_MODE_DISK is not defined.
    static tbb::concurrent_hash_map<
        std::string, std::shared_ptr<std::vector<char>>> _downloadCache;
};

PXR_NAMESPACE_CLOSE_SCOPE
