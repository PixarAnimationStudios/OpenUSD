//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/writableAsset.h"
#include "pxr/base/vt/value.h"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

// This class provides an example implementation of how the Emscripten Fetch
// API can be used to load a stage from layers located on a web server.
class FetchResolver final
    : public PXR_NS::ArResolver
{
public:
    FetchResolver() = default;
    virtual ~FetchResolver() = default;

protected:
    std::string _CreateIdentifier(
        const std::string& assetPath,
        const PXR_NS::ArResolvedPath& anchorAssetPath) const final;

    std::string _CreateIdentifierForNewAsset(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) const final;

    // Checks the local filesystem for the existence of the supplied path if 
    // it is absolute. Otherwise attempts to resolve the path from the server
    // by issuing a HTTP HEAD request and examining the response.
    ArResolvedPath _Resolve(
        const std::string& assetPath) const final;

    // Currently this resolver is read only. This method will always return an
    // empty asset path.
    ArResolvedPath _ResolveForNewAsset(
        const std::string& assetPath) const final;

    // Opens the path from the local filesystem if it exists, otherwise attemps to
    // download the asset from the server.  If the download is successful, it is
    // stored in the virtual filesystem.
    std::shared_ptr<ArAsset> _OpenAsset(
        const ArResolvedPath& resolvedPath) const final;

    // At the moment, this resolver is read only. This method will always
    // return nullptr.
    std::shared_ptr<ArWritableAsset>
    _OpenAssetForWrite(
        const ArResolvedPath& resolvedPath,
        WriteMode writeMode) const final;

private:
    mutable std::mutex _mutex;
    mutable std::set<std::string> _downloads;
    mutable std::condition_variable _condition;
};
