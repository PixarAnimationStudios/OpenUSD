//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "fetchResolver.h"

#include "pxr/usd/ar/defineResolver.h"
#include "pxr/usd/ar/filesystemAsset.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include <stdio.h>

#include <emscripten.h>
#include <emscripten/fetch.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

AR_DEFINE_RESOLVER(FetchResolver, ArResolver);

using EmscriptenFetchPtr = 
    std::unique_ptr<emscripten_fetch_t, decltype(&emscripten_fetch_close)>;

EmscriptenFetchPtr 
_FetchRequest(
    const std::string& verb, 
    const std::string& url)
{
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, verb.c_str());

    attr.attributes = 
        EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS;

    return EmscriptenFetchPtr(
        emscripten_fetch(&attr, url.c_str()), emscripten_fetch_close);
}

static std::string 
_CreateAnchoredIdentifier(
    const std::string& assetPath,
    const ArResolvedPath& anchorAssetPath)
{
    if (anchorAssetPath.IsEmpty()) {
        return assetPath;
    }

    return TfStringCatPaths(
        TfGetPathName(anchorAssetPath.GetPathString()), assetPath);

}

std::string 
FetchResolver::_CreateIdentifier(
    const std::string& assetPath,
    const ArResolvedPath& anchorAssetPath) const
{
    return _CreateAnchoredIdentifier(assetPath, anchorAssetPath);
}

std::string 
FetchResolver::_CreateIdentifierForNewAsset(
    const std::string& assetPath,
    const ArResolvedPath& anchorAssetPath) const
{
    return _CreateAnchoredIdentifier(assetPath, anchorAssetPath);
}

static bool 
_FetchFile(
    const std::string& resolvedPath)
{
    const char* url = resolvedPath.c_str();
    EmscriptenFetchPtr fetch = _FetchRequest("GET", url);

    if (!fetch || fetch->status != 200) {
        //emscripten_fetch_close(fetch); // no-op if fetch is nullptr
        return false;
    }

    const std::string dir = TfGetPathName(resolvedPath);
    if (!TfPathExists(dir)) {
        TfMakeDirs(dir);
    }

    // ensure that directory creation was successful before writing to
    // virtual filesystem.
    bool successful = TfPathExists(dir);
    if (successful) {
        std::ofstream f(url, std::ios::binary);
        if (f) {
            f.write(fetch->data, fetch->numBytes);
        } else {
            successful = false;
        }
    }

    //emscripten_fetch_close(fetch);
    return successful;
}

ArResolvedPath 
FetchResolver::_Resolve(
    const std::string& assetPath) const
{

    const std::string normPath = TfNormPath(assetPath);

    // Check if the supplied absolute path exists as is in the virtual
    // filesystem. This allows us to resolve paths to bundled assets, such as
    // schema definitions.
    if (!TfIsRelativePath(normPath)) {
        return TfPathExists(normPath) ? 
            ArResolvedPath(normPath) : 
            ArResolvedPath();
    }

    // Attempt to resolve a relative path by requesting if it exists
    // on the server.
    const char* url = normPath.c_str();
    EmscriptenFetchPtr fetch = _FetchRequest("HEAD", url);

    return fetch->status == 200 ? ArResolvedPath(normPath) : ArResolvedPath();
}

ArResolvedPath 
FetchResolver::_ResolveForNewAsset(
    const std::string& assetPath) const
{
    return ArResolvedPath();
}

std::shared_ptr<ArAsset> 
FetchResolver::_OpenAsset(
    const ArResolvedPath& resolvedPath) const
{
    const std::string normPath = TfNormPath(resolvedPath.GetPathString());

    bool isDownloading;
    {
        std::lock_guard<std::mutex> lock(_mutex);

        // If the file is being fetched by another thread we will want to
        // wait for that to complete
        isDownloading = _downloads.count(normPath);

        if (!isDownloading) {
            // Check virtual filesystem for the path.
            // If the file has already been fetched then we can return now
            // XXX: Track and check previously tried, but failed downloads
            if (TfPathExists(normPath)) {
                return ArFilesystemAsset::Open(resolvedPath);
            }

            // File does not exist so we will want to fetch it.
            _downloads.insert(normPath);
        }
    }

    if (isDownloading) {
        // Wait for the pending transfer to finish. If the file exists then
        // the fetch was successful.
        std::unique_lock<std::mutex> lock(_mutex);
        _condition.wait(lock,[this, &normPath](){
            return _downloads.count(normPath) == 0;
        });

        return TfPathExists(normPath) ? 
            ArFilesystemAsset::Open(resolvedPath) : nullptr;
    } else {
        // Fetch the file from the server. When complete, notify any other
        // waiting threads.
        const bool result = _FetchFile(normPath);

        {
            std::lock_guard<std::mutex> lock(_mutex);
            _downloads.erase(normPath);
            std::cout << "Fetch " << (result ? "Success" : "Failed")  << ": " 
                << normPath << std::endl;
        }

        _condition.notify_all();
        return result ? ArFilesystemAsset::Open(resolvedPath) : nullptr;
    }
}

std::shared_ptr<ArWritableAsset>
FetchResolver::_OpenAssetForWrite(
    const ArResolvedPath& resolvedPath,
    WriteMode writeMode) const
{
    return nullptr;
}
