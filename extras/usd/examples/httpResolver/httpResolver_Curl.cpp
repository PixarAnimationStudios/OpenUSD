//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// httpResolver_Curl.cpp
//
// CURL backend — implementations of the HttpResolver backend methods for
// Linux, Windows, and macOS (when PXR_HTTP_BACKEND=CURL is set explicitly).
//
#include "httpResolver.h"
#include "httpResolverConfig.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/pathUtils.h"

#include <curl/curl.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>

#ifdef PXR_HTTP_CACHE_MODE_DISK
#  include "pxr/base/arch/fileSystem.h"
#  ifndef PXR_HTTP_DISK_CACHE_PATH
#    define PXR_HTTP_DISK_CACHE_PATH ""
#  endif
#endif

PXR_NAMESPACE_OPEN_SCOPE

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

// Write callback for libcurl — appends received data to a std::string.
size_t _CurlWriteMemory(void* contents, size_t size, size_t nmemb, void* userp)
{
    const size_t realsize = size * nmemb;
    static_cast<std::string*>(userp)->append(
        static_cast<const char*>(contents), realsize);
    return realsize;
}

// Header callback used to capture the Content-Type and X-USD-Local-Path
// response headers.
struct HeaderCapture {
    std::string contentType;
    std::string localPath;
};

size_t _CurlWriteHeader(char* buffer, size_t size, size_t nitems, void* userdata)
{
    const size_t total = size * nitems;
    auto* cap = static_cast<HeaderCapture*>(userdata);
    std::string line(buffer, total);

    auto toLower = [](std::string s) {
        for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    };

    const std::string lower = toLower(line);
    if (lower.rfind("content-type:", 0) == 0) {
        cap->contentType = line.substr(13);
        // trim whitespace
        const size_t start = cap->contentType.find_first_not_of(" \t\r\n");
        const size_t stop  = cap->contentType.find_last_not_of(" \t\r\n");
        cap->contentType = (start == std::string::npos) ? ""
            : cap->contentType.substr(start, stop - start + 1);
    }
    if (lower.rfind("x-usd-local-path:", 0) == 0) {
        cap->localPath = line.substr(17);
        const size_t start = cap->localPath.find_first_not_of(" \t\r\n");
        const size_t stop  = cap->localPath.find_last_not_of(" \t\r\n");
        cap->localPath = (start == std::string::npos) ? ""
            : cap->localPath.substr(start, stop - start + 1);
    }
    return total;
}

// Convenience: strip the query string from a URL to get the base path.
std::string _StripQuery(const std::string& url)
{
    CURLUcode rc;
    CURLU* h = curl_url();
    rc = curl_url_set(h, CURLUPART_URL, url.c_str(), 0);
    if (rc) { curl_url_cleanup(h); return url; }
    rc = curl_url_set(h, CURLUPART_QUERY, nullptr, 0);
    if (rc) { curl_url_cleanup(h); return url; }
    char* out = nullptr;
    rc = curl_url_get(h, CURLUPART_URL, &out, 0);
    curl_url_cleanup(h);
    if (rc || !out) return url;
    std::string result(out);
    curl_free(out);
    return result;
}

// Return the file extension portion of a URL path (no dot), or "".
std::string _ExtFromUrlPath(const std::string& url)
{
    CURLU* h = curl_url();
    CURLUcode rc = curl_url_set(h, CURLUPART_URL, url.c_str(), 0);
    if (rc) { curl_url_cleanup(h); return TfGetExtension(url); }
    char* path = nullptr;
    rc = curl_url_get(h, CURLUPART_PATH, &path, 0);
    curl_url_cleanup(h);
    if (rc || !path) return "";
    const std::string ext = TfGetExtension(std::string(path));
    curl_free(path);
    return ext;
}

#ifdef PXR_HTTP_CACHE_MODE_DISK
std::string _DiskCachePath(const std::string& url)
{
    // Build a stable disk path: <cacheRoot>/<hash>
    std::string root = PXR_HTTP_DISK_CACHE_PATH;
    if (root.empty()) {
        root = ArchGetTmpDir();
    }
    // Simple hash using std::hash
    const size_t h = std::hash<std::string>{}(url);
    std::ostringstream oss;
    oss << root << "/httpResolverCache/" << std::hex << h;
    return oss.str();
}
#endif

} // anonymous namespace

// ---------------------------------------------------------------------------
// Backend method implementations
// ---------------------------------------------------------------------------

bool HttpResolver::_IsUrl(const std::string& path) const
{
    CURLU* h = curl_url();
    const CURLUcode rc = curl_url_set(h, CURLUPART_URL, path.c_str(), 0);
    curl_url_cleanup(h);
    if (rc) return false;

    // Confirm there is actually a scheme (not just a parseable string).
    CURLU* h2 = curl_url();
    curl_url_set(h2, CURLUPART_URL, path.c_str(), 0);
    char* scheme = nullptr;
    const CURLUcode rc2 = curl_url_get(h2, CURLUPART_SCHEME, &scheme, 0);
    curl_url_cleanup(h2);
    if (rc2 || !scheme) return false;
    curl_free(scheme);
    return true;
}

std::string HttpResolver::_GetExtensionFromUrl(const std::string& url) const
{
    // Try full URL parsing first; fall back to TfGetExtension for non-URLs.
    const std::string ext = _ExtFromUrlPath(url);
    if (!ext.empty()) return ext;
    return TfGetExtension(url);
}

std::string HttpResolver::_MakeFullyQualifiedURL(
    const std::string& assetPath,
    const std::string& anchorPath,
    bool& requestPackageContentsResolve) const
{
    requestPackageContentsResolve = false;

    // ---- Already a URL? ------------------------------------------------
    CURLU* urlH = curl_url();
    CURLUcode rc = curl_url_set(urlH, CURLUPART_URL, assetPath.c_str(), 0);
    if (!rc) {
        char* scheme = nullptr;
        rc = curl_url_get(urlH, CURLUPART_SCHEME, &scheme, 0);
        if (!rc && scheme) {
            curl_free(scheme);
            // If it's a .usdz, check the package cache.
            char* path = nullptr;
            curl_url_get(urlH, CURLUPART_PATH, &path, 0);
            const bool isUsdz = path &&
                std::string(TfGetExtension(path)) == "usdz";
            if (path) curl_free(path);

            if (isUsdz) {
                // Use simple string stripping to ensure the key matches what
                // _GetPackageContents stores (the raw URL without query string).
                const size_t qPos = assetPath.find('?');
                const std::string baseUrl = (qPos != std::string::npos)
                    ? assetPath.substr(0, qPos) : assetPath;
                tbb::concurrent_hash_map<std::string, HttpPackageInfo>::const_accessor acc;
                if (_packageInfoCache.find(acc, baseUrl)) {
                    if (acc->second.streamable) {
                        // Check if resource= already present.
                        char* query = nullptr;
                        rc = curl_url_get(urlH, CURLUPART_QUERY, &query, 0);
                        const bool hasQuery = (!rc && query);
                        bool hasResource = false;
                        if (hasQuery) {
                            hasResource = (std::string(query).find("resource=") !=
                                           std::string::npos);
                        }
                        if (query) curl_free(query);

                        if (!hasResource) {
                            CURL* easy = curl_easy_init();
                            const std::string& ep = acc->second.entrypoint;
                            char* escaped = curl_easy_escape(
                                easy, ep.c_str(), static_cast<int>(ep.size()));
                            curl_easy_cleanup(easy);
                            const std::string resourceParam =
                                "resource=" + std::string(escaped);
                            curl_free(escaped);
                            curl_url_set(urlH, CURLUPART_QUERY,
                                         resourceParam.c_str(), 0);
                            char* finalUrl = nullptr;
                            curl_url_get(urlH, CURLUPART_URL, &finalUrl, 0);
                            curl_url_cleanup(urlH);
                            if (!finalUrl) return assetPath;
                            std::string result(finalUrl);
                            curl_free(finalUrl);
                            return result;
                        }
                    }
                } else {
                    requestPackageContentsResolve = true;
                }
            }

            curl_url_cleanup(urlH);
            return assetPath;
        }
    }
    curl_url_cleanup(urlH);

    // ---- Relative path — resolve against anchor -----------------------
    if (!TfIsRelativePath(assetPath)) {
        return "";
    }

    CURLU* anchorH = curl_url();
    rc = curl_url_set(anchorH, CURLUPART_URL, anchorPath.c_str(), 0);
    if (rc) {
        TF_RUNTIME_ERROR("Could not parse anchorPath '%s': %d",
                         anchorPath.c_str(), rc);
        curl_url_cleanup(anchorH);
        return "";
    }

    // If the anchor itself is a .usdz, encode assetPath as resource=.
    char* anchorPathPart = nullptr;
    curl_url_get(anchorH, CURLUPART_PATH, &anchorPathPart, 0);
    const bool anchorIsUsdz = anchorPathPart &&
        std::string(TfGetExtension(anchorPathPart)) == "usdz";
    if (anchorPathPart) curl_free(anchorPathPart);

    if (anchorIsUsdz) {
        CURL* easy = curl_easy_init();
        char* escaped = curl_easy_escape(easy, assetPath.c_str(),
                                         static_cast<int>(assetPath.size()));
        curl_easy_cleanup(easy);
        const std::string resourceParam = "resource=" + std::string(escaped);
        curl_free(escaped);
        curl_url_set(anchorH, CURLUPART_QUERY, resourceParam.c_str(), 0);
    } else {
        // Resolve relative path against anchor directory.
        rc = curl_url_set(anchorH, CURLUPART_URL, assetPath.c_str(), 0);
        if (rc) {
            TF_RUNTIME_ERROR("Failed to resolve '%s' relative to '%s': %d",
                             assetPath.c_str(), anchorPath.c_str(), rc);
            curl_url_cleanup(anchorH);
            return "";
        }
    }

    char* finalUrl = nullptr;
    curl_url_get(anchorH, CURLUPART_URL, &finalUrl, 0);
    curl_url_cleanup(anchorH);
    if (!finalUrl) return "";
    std::string result(finalUrl);
    curl_free(finalUrl);
    return result;
}

double HttpResolver::_GetModificationTimeFromServer(
    const std::string& url) const
{
    CURL* curl = curl_easy_init();
    if (!curl) return 0.0;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);         // HEAD-like
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FILETIME, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "usdHttpResolver/1.0");

    const CURLcode res = curl_easy_perform(curl);
    double mtime = 0.0;
    if (res == CURLE_OK) {
        long fileTime = -1;
        curl_easy_getinfo(curl, CURLINFO_FILETIME, &fileTime);
        if (fileTime >= 0) {
            mtime = static_cast<double>(fileTime);
        }
    }
    curl_easy_cleanup(curl);
    return mtime;
}

std::shared_ptr<std::vector<char>> HttpResolver::_FetchFile(
    const std::string& url,
    std::string& filePath,
    bool& status) const
{
    auto buffer = std::make_shared<std::vector<char>>();
    status = false;

#ifdef PXR_HTTP_CACHE_MODE_DISK
    // Check disk cache first.
    const std::string cachePath = _DiskCachePath(url);
    {
        std::ifstream ifs(cachePath, std::ios::binary);
        if (ifs.good()) {
            filePath = cachePath;
            status = true;
            return buffer; // empty; caller opens from filePath
        }
    }
#else
    // Check in-memory cache.
    {
        tbb::concurrent_hash_map<
            std::string, std::shared_ptr<std::vector<char>>>::const_accessor acc;
        if (_downloadCache.find(acc, url)) {
            status = true;
            return acc->second;
        }
    }
#endif

    // Issue the HTTP request.
    CURL* curl = curl_easy_init();
    if (!curl) return buffer;

    HeaderCapture headers;
    std::string chunk;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _CurlWriteMemory);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, _CurlWriteHeader);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headers);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "usdHttpResolver/1.0");

    const CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        TF_RUNTIME_ERROR("CURL error fetching '%s': %s",
                         url.c_str(), curl_easy_strerror(res));
        return buffer;
    }

#ifdef PXR_HTTP_SERVER_FILEPATH
    // If the server returned a local file path, use it directly.
    if (headers.contentType == "application/x-usd-filepath" &&
        !headers.localPath.empty()) {
        filePath = headers.localPath;
        status = true;
        return buffer; // empty; caller opens from filePath
    }
#endif

    buffer->assign(chunk.begin(), chunk.end());

#ifdef PXR_HTTP_CACHE_MODE_DISK
    // Write to disk cache.
    {
        // Ensure directory exists.
        const size_t sep = cachePath.rfind('/');
        if (sep != std::string::npos) {
            const std::string dir = cachePath.substr(0, sep);
            // Best-effort mkdir; ArchMakeDirs not available in all builds.
            std::string cmd = "mkdir -p " + dir;
            (void)std::system(cmd.c_str());
        }
        std::ofstream ofs(cachePath, std::ios::binary);
        if (ofs.good()) {
            ofs.write(buffer->data(), static_cast<std::streamsize>(buffer->size()));
            filePath = cachePath;
            status = true;
            return {}; // let caller open from disk
        }
    }
#else
    // Store in memory cache.
    {
        tbb::concurrent_hash_map<
            std::string, std::shared_ptr<std::vector<char>>>::accessor acc;
        if (_downloadCache.insert(acc, url)) {
            acc->second = buffer;
        }
    }
#endif

    status = !buffer->empty();
    return buffer;
}

std::shared_ptr<std::vector<char>> HttpResolver::_FetchByteRange(
    const std::string& url,
    int32_t start,
    int32_t end,
    bool closedRange,
    bool checkCache,
    std::string& filePath,
    bool& status) const
{
    auto buffer = std::make_shared<std::vector<char>>();
    status = false;

    // Build a cache key that encodes the range.
    const std::string cacheKey = url + "|" +
        std::to_string(start) + "-" + std::to_string(end);

#ifndef PXR_HTTP_CACHE_MODE_DISK
    if (checkCache) {
        tbb::concurrent_hash_map<
            std::string, std::shared_ptr<std::vector<char>>>::const_accessor acc;
        if (_downloadCache.find(acc, cacheKey)) {
            status = true;
            return acc->second;
        }
    }
#endif

    // Build the range value for CURLOPT_RANGE.
    // libcurl automatically adds the "Range: bytes=" header prefix, so we
    // pass only the range spec itself (e.g. "-128", "0-1023", "256-").
    std::string rangeValue;
    if (start < 0) {
        // Suffix range: last |start| bytes.
        rangeValue = std::to_string(start);
    } else if (closedRange) {
        // Closed range: start-(end-1)  (HTTP ranges are inclusive)
        rangeValue = std::to_string(start) + "-" + std::to_string(end - 1);
    } else {
        // Open-ended range from start
        rangeValue = std::to_string(start) + "-";
    }

    CURL* curl = curl_easy_init();
    if (!curl) return buffer;

    std::string chunk;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _CurlWriteMemory);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
    curl_easy_setopt(curl, CURLOPT_RANGE, rangeValue.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "usdHttpResolver/1.0");

    const CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        TF_RUNTIME_ERROR("CURL error fetching range '%s' of '%s': %s",
                         rangeValue.c_str(), url.c_str(),
                         curl_easy_strerror(res));
        return buffer;
    }

    buffer->assign(chunk.begin(), chunk.end());

#ifndef PXR_HTTP_CACHE_MODE_DISK
    if (checkCache && !buffer->empty()) {
        tbb::concurrent_hash_map<
            std::string, std::shared_ptr<std::vector<char>>>::accessor acc;
        if (_downloadCache.insert(acc, cacheKey)) {
            acc->second = buffer;
        }
    }
#endif

    status = !buffer->empty();
    return buffer;
}

PXR_NAMESPACE_CLOSE_SCOPE
