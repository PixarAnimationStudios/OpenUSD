//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "httpResolver.h"
#include "httpResolverConfig.h"

#include "pxr/usd/ar/defineResolver.h"
#include "pxr/usd/ar/assetInfo.h"
#include "pxr/usd/ar/defaultResolver.h"
#include "pxr/usd/ar/inMemoryAsset.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/pathUtils.h"

#include <cstdio>
#include <cstring>
#include <limits>

PXR_NAMESPACE_OPEN_SCOPE

AR_DEFINE_RESOLVER(HttpResolver, ArDefaultResolver)

// ---------------------------------------------------------------------------
// Static cache definitions
// ---------------------------------------------------------------------------

tbb::concurrent_hash_map<std::string, HttpPackageInfo>
    HttpResolver::_packageInfoCache;

tbb::concurrent_hash_map<std::string, std::shared_ptr<std::vector<char>>>
    HttpResolver::_downloadCache;

// ---------------------------------------------------------------------------

HttpResolver::HttpResolver() = default;
HttpResolver::~HttpResolver() = default;

// ---------------------------------------------------------------------------
// ArResolver overrides
// ---------------------------------------------------------------------------

std::string HttpResolver::_CreateIdentifier(
    const std::string& assetPath,
    const ArResolvedPath& anchorAssetPath) const
{
    if (assetPath.empty()) {
        return assetPath;
    }

    bool requestPackageContentsResolve = false;
    std::string fullUrl = _MakeFullyQualifiedURL(
        assetPath, anchorAssetPath.GetPathString(),
        requestPackageContentsResolve);

#ifdef PXR_HTTP_USDZ_STREAMING
    if (requestPackageContentsResolve) {
        const bool ok = _GetPackageContents(fullUrl);
        if (ok) {
            // Re-run with the package now in cache so the backend can append
            // the resource= query parameter.
            bool ignored = false;
            fullUrl = _MakeFullyQualifiedURL(
                assetPath, anchorAssetPath.GetPathString(), ignored);
        } else {
            // Mark as non-streamable so we fall through to a full download.
            HttpPackageInfo info{false, ""};
            tbb::concurrent_hash_map<std::string, HttpPackageInfo>::accessor acc;
            _packageInfoCache.insert(acc, fullUrl);
            acc->second = info;
        }
    }
#endif

    if (!fullUrl.empty()) {
        return fullUrl;
    }

    // Fall back to filesystem resolution for non-URL paths.
    if (!_IsUrl(assetPath) && !_IsUrl(anchorAssetPath.GetPathString())) {
        return ArDefaultResolver::_CreateIdentifier(assetPath, anchorAssetPath);
    }

    return "";
}

std::string HttpResolver::_CreateIdentifierForNewAsset(
    const std::string& assetPath,
    const ArResolvedPath& anchorAssetPath) const
{
    // New assets over HTTP are not supported; delegate to filesystem.
    return ArDefaultResolver::_CreateIdentifierForNewAsset(
        assetPath, anchorAssetPath);
}

ArResolvedPath HttpResolver::_Resolve(const std::string& assetPath) const
{
    if (assetPath.empty()) {
        return {};
    }

    if (!_IsUrl(assetPath)) {
        return ArDefaultResolver::_Resolve(assetPath);
    }

    // For HTTP URLs, the identifier itself is the resolved path —
    // we bypass the filesystem existence check the default resolver does.
    return ArResolvedPath(assetPath);
}

ArResolvedPath HttpResolver::_ResolveForNewAsset(
    const std::string& assetPath) const
{
    return ArDefaultResolver::_ResolveForNewAsset(assetPath);
}

std::shared_ptr<ArAsset> HttpResolver::_OpenAsset(
    const ArResolvedPath& resolvedPath) const
{
    const std::string& path = resolvedPath.GetPathString();
    std::string filePath;
    bool status = true;
    std::shared_ptr<std::vector<char>> buffer;
    uint32_t resourceOffset = 0;

#ifdef PXR_HTTP_USDZ_STREAMING
    std::string resource;
    uint32_t rangeStart = 0, rangeEnd = 0;

    if (_FetchPackageStreamInfo(path, resource, rangeStart, rangeEnd)) {
        // Strip the ?resource= query param — byte ranges are fetched from
        // the base .usdz URL, not a resource-qualified sub-asset URL.
        const std::string baseUrl = path.substr(0, path.find('?'));
        buffer = _FetchByteRange(baseUrl, rangeStart, rangeEnd,
                                 /*closedRange=*/true,
                                 /*checkCache=*/true,
                                 filePath, status);
        if (status && buffer && !buffer->empty()) {
            resourceOffset = _GetResourceOffset(resource, buffer, status);
        } else if (!status) {
            TF_RUNTIME_ERROR(
                "Failed to download byte range for %s", path.c_str());
        }
    } else {
        buffer = _FetchFile(path, filePath, status);
    }
#else
    buffer = _FetchFile(path, filePath, status);
#endif

    if (status) {
        if (buffer && !buffer->empty()) {
#ifndef PXR_HTTP_USDZ_STREAMING
            // Without streaming, USD's package resolver requires a real named
            // file to parse via GetFileUnsafe().  For USDZ assets, write the
            // in-memory bytes to a temp file and open it via ArDefaultResolver.
            if (_GetExtensionFromUrl(path) == "usdz") {
                const std::string tmpPath =
                    ArchMakeTmpFileName("httpResolver", ".usdz");
                FILE* f = fopen(tmpPath.c_str(), "wb");
                if (f) {
                    fwrite(buffer->data(), 1, buffer->size(), f);
                    fclose(f);
                    return ArDefaultResolver::_OpenAsset(
                        ArResolvedPath(tmpPath));
                }
                TF_RUNTIME_ERROR(
                    "Failed to write USDZ temp file for %s", path.c_str());
                return nullptr;
            }
#endif
            // Use the aliasing shared_ptr constructor so the returned asset
            // shares ownership of the underlying buffer without copying.
            std::shared_ptr<const char> data(
                buffer, buffer->data() + resourceOffset);
            return ArInMemoryAsset::FromBuffer(
                std::move(data), buffer->size() - resourceOffset);
        }
        if (!filePath.empty()) {
            return ArDefaultResolver::_OpenAsset(ArResolvedPath(filePath));
        }
    }

    if (!_IsUrl(path)) {
        return ArDefaultResolver::_OpenAsset(resolvedPath);
    }

    TF_RUNTIME_ERROR("Failed to open asset %s", path.c_str());
    return nullptr;
}

ArTimestamp HttpResolver::_GetModificationTimestamp(
    const std::string& path,
    const ArResolvedPath& resolvedPath) const
{
    if (!_IsUrl(resolvedPath.GetPathString())) {
        return ArDefaultResolver::_GetModificationTimestamp(path, resolvedPath);
    }
    return ArTimestamp(_GetModificationTimeFromServer(
        resolvedPath.GetPathString()));
}

std::string HttpResolver::_GetExtension(const std::string& path) const
{
    // For streaming USDZ sub-assets encoded as "base.usdz?resource=inner.usda",
    // return the inner file's extension so SdfLayer picks the correct file
    // format parser (e.g. "usda") rather than the outer package format ("usdz").
    const size_t resPos = path.find("?resource=");
    if (resPos != std::string::npos) {
        const std::string baseUrl = path.substr(0, resPos);
        if (_GetExtensionFromUrl(baseUrl) == "usdz") {
            const std::string resourceName =
                path.substr(resPos + 10, path.find('&', resPos) - resPos - 10);
            const std::string innerExt = TfGetExtension(resourceName);
            if (!innerExt.empty()) {
                return innerExt;
            }
        }
    }
    return _GetExtensionFromUrl(path);
}

// ---------------------------------------------------------------------------
// Platform-agnostic USDZ helpers
// ---------------------------------------------------------------------------

bool HttpResolver::_GetPackageContents(const std::string& url) const
{
    // Fetch the last 128 bytes to find the End-of-Central-Directory record.
    std::string filePath;
    bool status = true;
    auto endData = _FetchByteRange(url, -128, 0,
                                   /*closedRange=*/false,
                                   /*checkCache=*/false,
                                   filePath, status);
    if (!status || !endData || endData->size() < 20) {
        return false;
    }

    const size_t maxSizeT = std::numeric_limits<size_t>::max();
    size_t offset = 0;
    bool found = false;

    // Locate the EOCD signature: PK\x05\x06
    for (; offset + 20 < endData->size(); ++offset) {
        if ((*endData)[offset]     == 0x50 &&
            (*endData)[offset + 1] == 0x4b &&
            (*endData)[offset + 2] == 0x05 &&
            (*endData)[offset + 3] == 0x06) {
            found = true;
            break;
        }
    }

    if (!found) {
        TF_RUNTIME_ERROR(
            "Failed to find End-of-Central-Directory record for %s",
            url.c_str());
        return false;
    }

    offset += 10; // skip to "total entries" field

    uint16_t numEntries = 0;
    std::memcpy(&numEntries, endData->data() + offset, sizeof(uint16_t));
    offset += 2;

    uint32_t dirSize = 0;
    std::memcpy(&dirSize, endData->data() + offset, sizeof(uint32_t));
    offset += 4;

    uint32_t dirOffset = 0;
    std::memcpy(&dirOffset, endData->data() + offset, sizeof(uint32_t));

    // Fetch the Central Directory in full.
    auto dirData = _FetchByteRange(url,
                                   static_cast<int32_t>(dirOffset),
                                   static_cast<int32_t>(dirOffset + dirSize),
                                   /*closedRange=*/true,
                                   /*checkCache=*/false,
                                   filePath, status);
    if (!status || !dirData) {
        TF_RUNTIME_ERROR("Failed to fetch central directory for %s", url.c_str());
        return false;
    }

    HttpPackageInfo packageInfo{true, ""};
    uint16_t entryIdx = 0;

    for (offset = 0; offset + 30 < dirData->size(); ++offset) {
        // Central directory file header signature: PK\x01\x02
        if ((*dirData)[offset]     != 0x50 ||
            (*dirData)[offset + 1] != 0x4b ||
            (*dirData)[offset + 2] != 0x01 ||
            (*dirData)[offset + 3] != 0x02) {
            continue;
        }

        offset += 24;
        if (offset + 4 > dirData->size()) break;
        uint32_t entrySize = 0;
        std::memcpy(&entrySize, dirData->data() + offset, sizeof(uint32_t));

        offset += 4;
        if (offset + 2 > dirData->size()) break;
        uint16_t nameLen = 0;
        std::memcpy(&nameLen, dirData->data() + offset, sizeof(uint16_t));

        offset += 2;
        if (offset + 2 > dirData->size()) break;
        uint16_t extraLen = 0;
        std::memcpy(&extraLen, dirData->data() + offset, sizeof(uint16_t));

        // Overflow guard
        if (30 > maxSizeT - extraLen ||
            30 + extraLen > maxSizeT - nameLen) {
            TF_RUNTIME_ERROR("Overflow while parsing central directory.");
            return false;
        }

        offset += 12;
        if (offset + 4 > dirData->size()) break;
        uint32_t fileOffset = 0;
        std::memcpy(&fileOffset, dirData->data() + offset, sizeof(uint32_t));

        offset += 4;
        if (offset + nameLen > dirData->size()) break;
        std::string entryName(
            reinterpret_cast<const char*>(dirData->data() + offset), nameLen);

        if (entryIdx == 0) {
            packageInfo.entrypoint = entryName;
        }
        ++entryIdx;

        HttpPackageEntry entry{entrySize + 30u + nameLen + extraLen, fileOffset};
        packageInfo.entries.insert({entryName, entry});
    }

    if (packageInfo.entries.empty()) {
        return false;
    }

    tbb::concurrent_hash_map<std::string, HttpPackageInfo>::accessor acc;
    _packageInfoCache.insert(acc, url);
    acc->second = packageInfo;
    return true;
}

uint32_t HttpResolver::_GetResourceOffset(
    const std::string& resource,
    const std::shared_ptr<std::vector<char>>& buf,
    bool& status) const
{
    const size_t maxSizeT = std::numeric_limits<size_t>::max();

    for (size_t offset = 0; offset + 30 < buf->size(); ++offset) {
        // Local file header signature: PK\x03\x04
        if ((*buf)[offset]     != 0x50 ||
            (*buf)[offset + 1] != 0x4b ||
            (*buf)[offset + 2] != 0x03 ||
            (*buf)[offset + 3] != 0x04) {
            continue;
        }

        offset += 22;
        if (offset + 4 > buf->size()) break;
        uint32_t entrySize = 0;
        std::memcpy(&entrySize, buf->data() + offset, sizeof(uint32_t));

        offset += 4;
        if (offset + 2 > buf->size()) break;
        uint16_t nameLen = 0;
        std::memcpy(&nameLen, buf->data() + offset, sizeof(uint16_t));

        offset += 2;
        if (offset + 2 > buf->size()) break;
        uint16_t extraLen = 0;
        std::memcpy(&extraLen, buf->data() + offset, sizeof(uint16_t));

        if (30 > maxSizeT - extraLen ||
            30 + extraLen > maxSizeT - nameLen) {
            TF_RUNTIME_ERROR("Overflow while parsing local file header.");
            status = false;
            return 0;
        }

        offset += 2;
        if (offset + nameLen > buf->size()) break;
        std::string fileName(
            reinterpret_cast<const char*>(buf->data() + offset), nameLen);

        if (fileName != resource) {
            TF_RUNTIME_ERROR("Found '%s' but expected '%s'",
                             fileName.c_str(), resource.c_str());
            status = false;
            return 0;
        }

        status = true;
        return 30 + nameLen + extraLen;
    }

    status = false;
    return 0;
}

bool HttpResolver::_FetchPackageStreamInfo(
    const std::string& assetPath,
    std::string& resource,
    uint32_t& start,
    uint32_t& end) const
{
#ifndef PXR_HTTP_USDZ_STREAMING
    return false;
#else
    // Split on '?' to get the base URL and query string.
    const size_t queryPos = assetPath.find('?');
    if (queryPos == std::string::npos) {
        return false;
    }

    const std::string baseUrl  = assetPath.substr(0, queryPos);
    const std::string query    = assetPath.substr(queryPos + 1);

    // Only handle .usdz packages.
    if (_GetExtensionFromUrl(baseUrl) != "usdz") {
        return false;
    }

    // Extract resource= value (simple percent-decode for %XX sequences).
    const std::string key = "resource=";
    const size_t keyPos = query.find(key);
    if (keyPos == std::string::npos) {
        return false;
    }

    std::string encoded = query.substr(keyPos + key.size());
    // Trim to the first '&' if there are additional query params.
    const size_t ampPos = encoded.find('&');
    if (ampPos != std::string::npos) {
        encoded.resize(ampPos);
    }

    // Minimal percent-decode.
    resource.clear();
    resource.reserve(encoded.size());
    for (size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.size()) {
            const char hex[3] = {encoded[i + 1], encoded[i + 2], '\0'};
            resource.push_back(static_cast<char>(std::strtol(hex, nullptr, 16)));
            i += 2;
        } else if (encoded[i] == '+') {
            resource.push_back(' ');
        } else {
            resource.push_back(encoded[i]);
        }
    }

    if (resource.empty()) {
        return false;
    }

    // Look up the package info cache.
    tbb::concurrent_hash_map<std::string, HttpPackageInfo>::const_accessor acc;
    if (!_packageInfoCache.find(acc, baseUrl)) {
        return false;
    }

    const auto& entries = acc->second.entries;
    const auto it = entries.find(resource);
    if (it == entries.end()) {
        TF_RUNTIME_ERROR("Resource '%s' not found in package '%s'",
                         resource.c_str(), baseUrl.c_str());
        return false;
    }

    start = it->second.offset;
    end   = it->second.offset + it->second.size;
    return true;
#endif
}

PXR_NAMESPACE_CLOSE_SCOPE
