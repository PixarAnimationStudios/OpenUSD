//
// Copyright 2016 Pixar
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
#ifndef PXR_USD_AR_DEFAULT_RESOLVER_H
#define PXR_USD_AR_DEFAULT_RESOLVER_H

/// \file ar/defaultResolver.h

#include "pxr/pxr.h"
#include "pxr/usd/ar/api.h"
#include "pxr/usd/ar/defaultResolverContext.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/resolver.h"

#include <memory>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class ArDefaultResolver
///
/// Default asset resolution implementation used when no plugin
/// implementation is provided.
///
/// In order to resolve assets specified by relative paths, this resolver
/// implements a simple "search path" scheme. The resolver will anchor the
/// relative path to a series of directories and return the first absolute
/// path where the asset exists.
///
/// The first directory will always be the current working directory. The
/// resolver will then examine the directories specified via the following
/// mechanisms (in order):
///
///    - The currently-bound ArDefaultResolverContext for the calling thread
///    - ArDefaultResolver::SetDefaultSearchPath
///    - The environment variable PXR_AR_DEFAULT_SEARCH_PATH. This is
///      expected to be a list of directories delimited by the platform's 
///      standard path separator.
///
/// ArDefaultResolver supports creating an ArDefaultResolverContext via
/// ArResolver::CreateContextFromString by passing a list of directories
/// delimited by the platform's standard path separator.
class ArDefaultResolver
    : public ArResolver
{
public:
    AR_API 
    ArDefaultResolver();

    AR_API 
    virtual ~ArDefaultResolver();

    /// Set the default search path that will be used during asset
    /// resolution. This must be called before the first call
    /// to \ref ArGetResolver.
    /// The specified paths will be searched *in addition to, and before*
    /// paths specified via the environment variable PXR_AR_DEFAULT_SEARCH_PATH
    AR_API
    static void SetDefaultSearchPath(
        const std::vector<std::string>& searchPath);

protected:
    AR_API
    std::string _CreateIdentifier(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) const override;

    AR_API
    std::string _CreateIdentifierForNewAsset(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) const override;

    AR_API
    ArResolvedPath _Resolve(
        const std::string& assetPath) const override;

    AR_API
    ArResolvedPath _ResolveForNewAsset(
        const std::string& assetPath) const override;

    AR_API
    ArResolverContext _CreateDefaultContext() const override;

    /// Creates a context that adds the directory containing \p assetPath
    /// as a first directory to be searched, when the resulting context is
    /// bound (\see ArResolverContextBinder).  
    ///
    /// If \p assetPath is empty, returns an empty context; otherwise, if
    /// \p assetPath is not an absolute filesystem path, it will first be
    /// anchored to the process's current working directory.
    AR_API
    ArResolverContext _CreateDefaultContextForAsset(
        const std::string& assetPath) const override;

    /// Creates an ArDefaultResolverContext from \p contextStr. This
    /// string is expected to be a list of directories delimited by
    /// the platform's standard path separator.
    AR_API
    ArResolverContext _CreateContextFromString(
        const std::string& contextStr) const override;

    AR_API
    bool _IsContextDependentPath(
        const std::string& assetPath) const override;

    AR_API
    ArTimestamp _GetModificationTimestamp(
        const std::string& path,
        const ArResolvedPath& resolvedPath) const override;

    AR_API
    std::shared_ptr<ArAsset> _OpenAsset(
        const ArResolvedPath& resolvedPath) const override;

    /// Creates an ArFilesystemWriteableAsset for the asset at the
    /// given \p resolvedPath.
    AR_API
    std::shared_ptr<ArWritableAsset> _OpenAssetForWrite(
        const ArResolvedPath& resolvedPath,
        WriteMode writeMode) const override;

private:
    const ArDefaultResolverContext* _GetCurrentContextPtr() const;

    ArDefaultResolverContext _fallbackContext;
    ArResolverContext _defaultContext;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_AR_DEFAULT_RESOLVER_H
