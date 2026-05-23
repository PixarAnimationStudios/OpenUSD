//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// httpResolver_Foundation.mm
//
// Foundation/NSURL backend — implementations of the HttpResolver backend
// methods for macOS and iOS (the default on Apple platforms).
//
#include "httpResolver.h"
#include "httpResolverConfig.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/pathUtils.h"

#import <Foundation/Foundation.h>

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

#ifdef PXR_HTTP_CACHE_MODE_DISK
std::string _DiskCachePath(const std::string& url)
{
    std::string root = PXR_HTTP_DISK_CACHE_PATH;
    if (root.empty()) {
        root = ArchGetTmpDir();
    }
    const size_t h = std::hash<std::string>{}(url);
    std::ostringstream oss;
    oss << root << "/httpResolverCache/" << std::hex << h;
    return oss.str();
}
#endif

// Synchronously download data from a URL using NSURLSession.
// Returns the data bytes, or empty on error.  statusCode is set to the
// HTTP status code received (0 on network error).
std::shared_ptr<std::vector<char>>
_NSURLFetch(NSURL* url, NSDictionary* headers,
            NSInteger* outStatusCode, NSString** outContentType,
            NSString** outLocalPath)
{
    auto buffer = std::make_shared<std::vector<char>>();
    if (outStatusCode) *outStatusCode = 0;

    NSMutableURLRequest* req =
        [NSMutableURLRequest requestWithURL:url];
    if (headers) {
        for (NSString* key in headers) {
            [req setValue:headers[key] forHTTPHeaderField:key];
        }
    }

    dispatch_group_t group = dispatch_group_create();
    dispatch_group_enter(group);

    __block NSData* responseData = nil;
    __block NSHTTPURLResponse* httpResponse = nil;
    __block NSError* responseError = nil;

    NSURLSessionDataTask* task =
        [[NSURLSession sharedSession]
            dataTaskWithRequest:req
              completionHandler:^(NSData* data,
                                  NSURLResponse* resp,
                                  NSError* error) {
                responseData   = data;
                httpResponse   = (NSHTTPURLResponse*)resp;
                responseError  = error;
                dispatch_group_leave(group);
              }];
    [task resume];
    dispatch_group_wait(group, DISPATCH_TIME_FOREVER);

    if (responseError) {
        TF_RUNTIME_ERROR("NSURLSession error: %s",
            [[responseError localizedDescription] UTF8String]);
        return buffer;
    }

    const NSInteger code = [httpResponse statusCode];
    if (outStatusCode) *outStatusCode = code;

    if (outContentType) {
        *outContentType = [httpResponse valueForHTTPHeaderField:@"Content-Type"];
    }
    if (outLocalPath) {
        *outLocalPath = [httpResponse valueForHTTPHeaderField:@"X-USD-Local-Path"];
    }

    if (responseData && [responseData length] > 0) {
        const char* bytes = static_cast<const char*>([responseData bytes]);
        buffer->assign(bytes, bytes + [responseData length]);
    }
    return buffer;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Backend method implementations
// ---------------------------------------------------------------------------

bool HttpResolver::_IsUrl(const std::string& path) const
{
    NSString* ns = [NSString stringWithUTF8String:path.c_str()];
    NSURL* url   = [NSURL URLWithString:ns];
    return url != nil && [url scheme] != nil;
}

std::string HttpResolver::_GetExtensionFromUrl(const std::string& url) const
{
    NSString* ns   = [NSString stringWithUTF8String:url.c_str()];
    NSURL* nsurl   = [NSURL URLWithString:ns];
    if (nsurl) {
        NSString* ext = [nsurl pathExtension];
        if (ext && [ext length] > 0) {
            return std::string([ext UTF8String]);
        }
    }
    return TfGetExtension(url);
}

std::string HttpResolver::_MakeFullyQualifiedURL(
    const std::string& assetPath,
    const std::string& anchorPath,
    bool& requestPackageContentsResolve) const
{
    requestPackageContentsResolve = false;

    NSString* ns_asset = [NSString stringWithUTF8String:assetPath.c_str()];
    NSURL* url         = [NSURL URLWithString:ns_asset];

    // ---- Already a URL? ------------------------------------------------
    if (url && [url scheme]) {
        if ([[url pathExtension] isEqualToString:@"usdz"]) {
            NSURLComponents* stripped =
                [NSURLComponents componentsWithString:ns_asset];
            stripped.query = nil;
            const std::string baseUrl =
                std::string([stripped.URL.absoluteString UTF8String]);

            tbb::concurrent_hash_map<std::string, HttpPackageInfo>::const_accessor acc;
            if (_packageInfoCache.find(acc, baseUrl)) {
                if (acc->second.streamable) {
                    NSURLComponents* comps =
                        [NSURLComponents componentsWithString:ns_asset];
                    bool hasResource = false;
                    for (NSURLQueryItem* item in comps.queryItems) {
                        if ([item.name isEqualToString:@"resource"]) {
                            hasResource = true;
                            break;
                        }
                    }
                    if (!hasResource) {
                        NSString* ep = [NSString stringWithUTF8String:
                                        acc->second.entrypoint.c_str()];
                        ep = [ep stringByAddingPercentEncodingWithAllowedCharacters:
                              NSCharacterSet.URLQueryAllowedCharacterSet];
                        [comps setQuery:[NSString stringWithFormat:
                                         @"resource=%@", ep]];
                        return std::string([comps.URL.absoluteString UTF8String]);
                    }
                }
            } else {
                requestPackageContentsResolve = true;
            }
        }
        return assetPath;
    }

    // ---- Relative path — resolve against anchor -----------------------
    if (!TfIsRelativePath(assetPath)) {
        return "";
    }

    NSString* ns_anchor = [NSString stringWithUTF8String:anchorPath.c_str()];
    NSURL* anchorUrl    = [NSURL URLWithString:ns_anchor];
    if (!anchorUrl || ![anchorUrl scheme]) {
        return "";
    }

    if ([[anchorUrl pathExtension] isEqualToString:@"usdz"]) {
        // Encode the sub-asset as resource=.
        NSURLComponents* comps =
            [NSURLComponents componentsWithString:ns_anchor];
        NSString* escaped = [ns_asset
            stringByAddingPercentEncodingWithAllowedCharacters:
            NSCharacterSet.URLQueryAllowedCharacterSet];
        [comps setQuery:[NSString stringWithFormat:@"resource=%@", escaped]];
        return std::string([comps.URL.absoluteString UTF8String]);
    }

    NSURL* resolved = [NSURL URLWithString:ns_asset relativeToURL:anchorUrl];
    resolved = [resolved absoluteURL];
    return std::string([resolved.absoluteString UTF8String]);
}

double HttpResolver::_GetModificationTimeFromServer(
    const std::string& url) const
{
    NSString* ns  = [NSString stringWithUTF8String:url.c_str()];
    NSURL* nsurl  = [NSURL URLWithString:ns];
    if (!nsurl) return 0.0;

    NSMutableURLRequest* req =
        [NSMutableURLRequest requestWithURL:nsurl];
    [req setHTTPMethod:@"HEAD"];

    dispatch_group_t group = dispatch_group_create();
    dispatch_group_enter(group);
    __block double mtime = 0.0;

    NSURLSessionDataTask* task =
        [[NSURLSession sharedSession]
            dataTaskWithRequest:req
              completionHandler:^(NSData*,
                                  NSURLResponse* resp,
                                  NSError*) {
                NSHTTPURLResponse* http = (NSHTTPURLResponse*)resp;
                NSString* lastMod =
                    [http valueForHTTPHeaderField:@"Last-Modified"];
                if (lastMod) {
                    NSDateFormatter* fmt =
                        [[NSDateFormatter alloc] init];
                    fmt.locale =
                        [[NSLocale alloc] initWithLocaleIdentifier:@"en_US_POSIX"];
                    fmt.dateFormat = @"EEE, dd MMM yyyy HH:mm:ss zzz";
                    NSDate* date = [fmt dateFromString:lastMod];
                    if (date) {
                        mtime = [date timeIntervalSince1970];
                    }
                }
                dispatch_group_leave(group);
              }];
    [task resume];
    dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
    return mtime;
}

std::shared_ptr<std::vector<char>> HttpResolver::_FetchFile(
    const std::string& url,
    std::string& filePath,
    bool& status) const
{
    auto empty = std::make_shared<std::vector<char>>();
    status = false;

#ifdef PXR_HTTP_CACHE_MODE_DISK
    const std::string cachePath = _DiskCachePath(url);
    {
        std::ifstream ifs(cachePath, std::ios::binary);
        if (ifs.good()) {
            filePath = cachePath;
            status = true;
            return empty;
        }
    }
#else
    {
        tbb::concurrent_hash_map<
            std::string, std::shared_ptr<std::vector<char>>>::const_accessor acc;
        if (_downloadCache.find(acc, url)) {
            status = true;
            return acc->second;
        }
    }
#endif

    NSString* ns   = [NSString stringWithUTF8String:url.c_str()];
    NSURL* nsurl   = [NSURL URLWithString:ns];
    if (!nsurl || ![nsurl scheme]) {
        return empty;
    }

    NSInteger statusCode = 0;
    NSString* contentType = nil;
    NSString* localPath   = nil;
    auto buffer = _NSURLFetch(nsurl, nil, &statusCode, &contentType, &localPath);

    if (statusCode < 200 || statusCode > 299) {
        TF_RUNTIME_ERROR("HTTP %ld fetching '%s'",
                         static_cast<long>(statusCode), url.c_str());
        return empty;
    }

#ifdef PXR_HTTP_SERVER_FILEPATH
    if (contentType &&
        [contentType isEqualToString:@"application/x-usd-filepath"] &&
        localPath && [localPath length] > 0) {
        filePath = std::string([localPath UTF8String]);
        status = true;
        return empty;
    }
#endif

#ifdef PXR_HTTP_CACHE_MODE_DISK
    {
        const size_t sep = cachePath.rfind('/');
        if (sep != std::string::npos) {
            const std::string dir = cachePath.substr(0, sep);
            std::string cmd = "mkdir -p " + dir;
            (void)std::system(cmd.c_str());
        }
        std::ofstream ofs(cachePath, std::ios::binary);
        if (ofs.good()) {
            ofs.write(buffer->data(),
                      static_cast<std::streamsize>(buffer->size()));
            filePath = cachePath;
            status = true;
            return empty;
        }
    }
#else
    if (!buffer->empty()) {
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
    auto empty = std::make_shared<std::vector<char>>();
    status = false;

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

    NSString* ns   = [NSString stringWithUTF8String:url.c_str()];
    NSURL* nsurl   = [NSURL URLWithString:ns];
    if (!nsurl || ![nsurl scheme]) {
        return empty;
    }

    // Build the Range header value.
    NSString* rangeValue;
    if (start < 0) {
        rangeValue = [NSString stringWithFormat:@"bytes=%d", start];
    } else if (closedRange) {
        rangeValue = [NSString stringWithFormat:@"bytes=%d-%d", start, end - 1];
    } else {
        rangeValue = [NSString stringWithFormat:@"bytes=%d-", start];
    }

    NSDictionary* headers = @{@"Range": rangeValue};
    NSInteger statusCode = 0;
    auto buffer = _NSURLFetch(nsurl, headers, &statusCode, nil, nil);

    // 206 = Partial Content (expected for byte-range)
    // 200 = server ignored range header and returned full content
    if (statusCode != 206 && statusCode != 200) {
        TF_RUNTIME_ERROR("HTTP %ld fetching range '%s' of '%s'",
                         static_cast<long>(statusCode),
                         [rangeValue UTF8String], url.c_str());
        return empty;
    }

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
