//
// Copyright 2021 Pixar
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
#include "pxr/pxr.h"

#include "resolver.h"
#include "resolverContext.h"

#include "pxr/usd/ar/assetInfo.h"
#include "pxr/usd/ar/defineResolver.h"
#include "pxr/usd/ar/filesystemAsset.h"
#include "pxr/usd/ar/filesystemWritableAsset.h"
#include "pxr/usd/ar/notice.h"
#include "pxr/base/js/json.h"
#include "pxr/base/js/value.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/dictionary.h"

#include <fstream>
#include <iostream>
#include <shared_mutex>
#include <thread>
#include <unordered_map>

PXR_NAMESPACE_USING_DIRECTIVE

PXR_NAMESPACE_OPEN_SCOPE

AR_DEFINE_RESOLVER(UsdResolverExampleResolver, ArResolver);

TF_DEBUG_CODES(
    USD_RESOLVER_EXAMPLE
);

TF_DEFINE_ENV_SETTING(
    USD_RESOLVER_EXAMPLE_ASSET_DIR, ".",
    "Root of asset directory used by UsdResolverExampleResolver.")

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    
    (asset)
    (latest)
    ((version, "{$VERSION}"))
);

PXR_NAMESPACE_CLOSE_SCOPE

// ------------------------------------------------------------

// Table of asset name to version string used for substitutions
// during asset path resolution.
//
// Supports reading mappings from .json files with a dictionary
// of asset name -> version string mappings, like:
// 
// {
//    "Woody" : "1",
//    "Buzz" : "2"
// }
//
class _VersionTable
{
public:
    static std::unique_ptr<_VersionTable>
    ReadFromFile(const std::string& mappingFile);

    bool operator==(const _VersionTable& rhs) const;
    bool operator!=(const _VersionTable& rhs) const;

    std::string GetVersionForAsset(const std::string& modelName) const;
    std::string GetDebugString(size_t indent = 0) const;

private:
    using _AssetNameToVersionMap =
        std::unordered_map<std::string, std::string>;

    _AssetNameToVersionMap _versionMap;
};

std::unique_ptr<_VersionTable>
_VersionTable::ReadFromFile(const std::string& mappingFile)
{
    std::unique_ptr<_VersionTable> result;

    std::ifstream fs(mappingFile);
    if (!fs) {
        TF_RUNTIME_ERROR(
            "Unable to open mapping file %s", mappingFile.c_str());
        return result;
    }

    JsParseError err;
    const JsValue value = JsParseStream(fs, &err);
    if (value.IsNull()) {
        TF_RUNTIME_ERROR(
            "Syntax error in %s:%d:%d: %s\n",
            mappingFile.c_str(), err.line, err.column, err.reason.c_str());
        return result;
    }

    if (!value.IsObject()) {
        TF_RUNTIME_ERROR(
            "Syntax error in %s: must be dictionary",
            mappingFile.c_str());
        return result;
    }

    _AssetNameToVersionMap versionMap;

    const JsObject& obj = value.GetJsObject();
    for (const auto& entry : obj) {
        const std::string& assetName = entry.first;
        const JsValue& assetVersion = entry.second;

        if (!assetVersion.IsString()) {
            TF_RUNTIME_ERROR(
                "Syntax error in %s: version for '%s' must be a string",
                mappingFile.c_str(), entry.first.c_str());
            continue;
        }

        versionMap[assetName] = assetVersion.GetString();
    }

    result.reset(new _VersionTable);
    result->_versionMap.swap(versionMap);
    return result;
}

bool
_VersionTable::operator==(const _VersionTable& rhs) const
{
    return _versionMap == rhs._versionMap;
}

bool
_VersionTable::operator!=(const _VersionTable& rhs) const
{
    return !(*this == rhs);
}

std::string
_VersionTable::GetVersionForAsset(const std::string& modelName) const
{
    return TfMapLookupByValue(_versionMap, modelName, std::string());
}

std::string
_VersionTable::GetDebugString(size_t indent) const
{
    std::string rval;
    for (const auto& entry : _versionMap) {
        rval.append(indent, ' ')
            .append(entry.first).append(" -> ").append(entry.second)
            .append(1, '\n');
    }
    return rval;
}

// ------------------------------------------------------------

// Registry of version tables that manages reading and caching
// data from mapping files.
class _VersionTableRegistry
{
public:
    _VersionTableRegistry() = default;

    _VersionTableRegistry(const _VersionTableRegistry&) = delete;
    _VersionTableRegistry(_VersionTableRegistry&&) = delete;
    _VersionTableRegistry& operator==(const _VersionTableRegistry&) = delete;
    _VersionTableRegistry& operator==(_VersionTableRegistry&&) = delete;

    std::string GetVersionForAsset(
        const std::string& mappingFile,
        const std::string& modelName) const;

    bool Refresh(const std::string& mappingFile);

private:
    mutable std::shared_timed_mutex _mutex;
    mutable std::unordered_map<
        std::string, std::unique_ptr<_VersionTable>> _maps;
};

std::string
_VersionTableRegistry::GetVersionForAsset(
    const std::string& mappingFile,
    const std::string& modelName) const
{
    {
        std::shared_lock<std::shared_timed_mutex> readLock(_mutex);
        const std::unique_ptr<_VersionTable>* mappings =
            TfMapLookupPtr(_maps, mappingFile);
        if (mappings) {
            return (*mappings)->GetVersionForAsset(modelName);
        }
    }

    std::unique_ptr<_VersionTable> mapping =
        _VersionTable::ReadFromFile(mappingFile);
    
    std::shared_lock<std::shared_timed_mutex> writeLock(_mutex,std::defer_lock);
    writeLock.lock();
        
    auto entry = _maps.emplace(mappingFile, std::move(mapping));
    return entry.first->second->GetVersionForAsset(modelName);
}

bool
_VersionTableRegistry::Refresh(const std::string& mappingFile)
{
    {
        std::shared_lock<std::shared_timed_mutex> readLock(_mutex);
        const std::unique_ptr<_VersionTable>* mappings =
            TfMapLookupPtr(_maps, mappingFile);
        if (!mappings) {
            return false;
        }
    }

    std::unique_ptr<_VersionTable> mapping =
        _VersionTable::ReadFromFile(mappingFile);

    std::shared_lock<std::shared_timed_mutex> writeLock(_mutex,std::defer_lock);
    writeLock.lock();

    auto entry = _maps.find(mappingFile);
    if (entry == _maps.end() || *entry->second == *mapping) {
        return false;
    }

    entry->second = std::move(mapping);
    return true;
}

static _VersionTableRegistry&
_GetVersionTableRegistry()
{
    static _VersionTableRegistry reg;
    return reg;
}

// ------------------------------------------------------------

// Simple class to represent a URI. This is just a helper for this example
// and should not be considered an RFC-compliant implementation. A real
// resolver implementation might want to use an external URI library instead.
class _URI
{
public:
    _URI(const std::string& uri)
    {
        const size_t index = uri.find(":");
        if (index == std::string::npos) {
            _path.push_back(uri);
        }
        else {
            _scheme = uri.substr(0, index);

            std::vector<std::string> path = 
                TfStringSplit(uri.substr(index + 1), "/");
            if (!path.empty()) {
                _assetName = std::move(path.front());
                
                _path.resize(path.size() - 1);
                std::move(path.begin() + 1, path.end(), _path.begin());
            }
        }
    }

    _URI(const ArResolvedPath& resolvedPath)
        : _URI(resolvedPath.GetPathString())
    {
    }

    const std::string& GetScheme() const { return _scheme; }
    const std::string& GetAssetName() const { return _assetName; }

    std::string GetPath() const
    {
        if (_scheme.empty()) {
            return TfNormPath(_path.back());
        }

        return TfNormPath(_assetName + "/" + TfStringJoin(_path, "/"));
    }

    std::string GetNormalized() const
    {
        if (_scheme.empty()) {
            return TfNormPath(_path.back());
        }

        return _scheme + ":" + 
            TfNormPath(_assetName + "/" + TfStringJoin(_path, "/"));
    }

    _URI& Anchor(const std::string& relativePath)
    {
        std::vector<std::string> relativeParts =
            TfStringSplit(relativePath, "/");

        if (!_path.empty()) {
            _path.pop_back();
        }

        _path.insert(_path.end(), relativeParts.begin(), relativeParts.end());
        return *this;
    }

    _URI& Replace(const std::string& replaceStr, const std::string& replaceWith)
    {
        for (std::string& elem : _path) {
            elem = TfStringReplace(elem, replaceStr, replaceWith);
        }

        return *this;
    }

private:
    std::string _scheme;
    std::string _assetName;
    std::vector<std::string> _path;
};

// ------------------------------------------------------------

static std::string
_GetFilesystemPath(const _URI& assetURI)
{
    return TfAbsPath(TfStringCatPaths(
        TfGetEnvSetting(USD_RESOLVER_EXAMPLE_ASSET_DIR),
        assetURI.GetPath()));
}

static std::string
_GetFilesystemPath(const ArResolvedPath& resolvedPath)
{
    const _URI assetURI(resolvedPath.GetPathString());
    return _GetFilesystemPath(assetURI);
}

UsdResolverExampleResolver::UsdResolverExampleResolver()
{
}

UsdResolverExampleResolver::~UsdResolverExampleResolver() = default;

static std::string
_CreateIdentifierHelper(
    const std::string& assetPath,
    const ArResolvedPath& anchorAssetPath)
{
    // Ar will call this function if either assetPath or anchorAssetPath
    // have a URI scheme that is associated with this resolver.

    // If assetPath has a URI scheme it must be an absolute URI so we 
    // just return the normalized URI as the asset's identifier.
    const _URI assetURI(assetPath);
    if (!assetURI.GetScheme().empty()) {
        TF_AXIOM(assetURI.GetScheme() == _tokens->asset);
        return assetURI.GetNormalized();
    }

    // Otherwise anchor assetPath to anchorAssetPath and return the
    // normalized URI.
    return _URI(anchorAssetPath).Anchor(assetPath).GetNormalized();
}

std::string
UsdResolverExampleResolver::_CreateIdentifier(
    const std::string& assetPath,
    const ArResolvedPath& anchorAssetPath) const
{
    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg(
        "UsdResolverExampleResolver::_CreateIdentifier('%s', '%s')\n",
        assetPath.c_str(), anchorAssetPath.GetPathString().c_str());

    return _CreateIdentifierHelper(assetPath, anchorAssetPath);
}

std::string
UsdResolverExampleResolver::_CreateIdentifierForNewAsset(
    const std::string& assetPath,
    const ArResolvedPath& anchorAssetPath) const
{
    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg(
        "UsdResolverExampleResolver::_CreateIdentifierForNewAsset"
        "('%s', '%s')\n",
        assetPath.c_str(), anchorAssetPath.GetPathString().c_str());

    return _CreateIdentifierHelper(assetPath, anchorAssetPath);
}

ArResolvedPath
UsdResolverExampleResolver::_ResolveHelper(
    const std::string& assetPath,
    bool forNewAsset) const
{
    _URI assetURI(assetPath);
    TF_AXIOM(assetURI.GetScheme() == _tokens->asset);

    // Substitute "{$VERSION}" variables in the asset path with
    // the version specified for the asset in the currently-bound
    // context object.
    if (TfStringContains(assetPath, _tokens->version)) {
        std::string version;
        if (const UsdResolverExampleResolverContext* ctx =
            _GetCurrentContextObject<UsdResolverExampleResolverContext>()) {
            version = _GetVersionTableRegistry().GetVersionForAsset(
                ctx->GetMappingFile(), assetURI.GetAssetName());
        }

        assetURI.Replace(
            _tokens->version, version.empty() ? _tokens->latest : version);
    }

    // If we're resolving for a new asset, a file may not yet exist at the
    // corresponding filesystem path for this URI so we don't do any existence
    // checks.
    //
    // If this is not for a new asset, we need to check if a file exists at the
    // corresponding filesystem path for this URI. If so, we just return an
    // ArResolvedPath with the original URI to indicate that the asset exists.
    if (!forNewAsset) {
        const std::string filesystemPath = _GetFilesystemPath(assetURI);
        if (!TfIsFile(filesystemPath)) {
            TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg(
                "  - Asset does not exist at filesystem path %s\n",
                filesystemPath.c_str());
            return ArResolvedPath();
        }
        else {
            TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg(
                "  - Asset found at filesystem path %s\n",
                filesystemPath.c_str());
        }
    }

    // We use the (substituted) "asset:" URI as the resolved path so that Ar
    // will forward calls to other APIs that take an ArResolvedPath (like
    // ArResolver::OpenAsset) back to this resolver for handling.
    // 
    // We could have used the filesystem path as the resolved path instead. If
    // we had, those calls to other APIs would have been forwarded to the
    // primary resolver since it's responsible for handling non-URI paths.
    return ArResolvedPath(assetURI.GetNormalized());
}

ArResolvedPath
UsdResolverExampleResolver::_Resolve(
    const std::string& assetPath) const
{
    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg(
        "UsdResolverExampleResolver::_Resolve('%s')\n",
        assetPath.c_str());
    
    return _ResolveHelper(assetPath, /* forNewAsset = */ false);
}

ArResolvedPath
UsdResolverExampleResolver::_ResolveForNewAsset(
    const std::string& assetPath) const
{
    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg(
        "UsdResolverExampleResolver::_ResolveForNewAsset('%s')\n",
        assetPath.c_str());

    return _ResolveHelper(assetPath, /* forNewAsset = */ true);
}

ArResolverContext
UsdResolverExampleResolver::_CreateDefaultContext() const
{
    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg(
        "UsdResolverExampleResolver::_CreateDefaultContext()\n");

    const std::string defaultMappingFile = TfAbsPath("versions.json");
    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg(
        "  - Looking for default mapping at %s...", defaultMappingFile.c_str());

    if (TfIsFile(defaultMappingFile)) {
        TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg(" found\n");
        return ArResolverContext(
            UsdResolverExampleResolverContext(defaultMappingFile));
    }
    
    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg(" not found\n");
    return ArResolverContext();
}

ArResolverContext
UsdResolverExampleResolver::_CreateDefaultContextForAsset(
    const std::string& assetPath) const
{
    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg(
        "UsdResolverExampleResolver::_CreateDefaultContextForAsset('%s')\n",
        assetPath.c_str());

    const std::string assetDir = TfGetPathName(assetPath);
    const std::string mappingFile = 
        TfAbsPath(TfStringCatPaths(assetDir, "versions.json"));

    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg(
        "  - Looking for mapping at %s...", mappingFile.c_str());

    if (TfIsFile(mappingFile)) {
        TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg(" found\n");
        return ArResolverContext(
            UsdResolverExampleResolverContext(mappingFile));
    }

    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg(" not found\n");
    return ArResolverContext();
}

ArResolverContext
UsdResolverExampleResolver::_CreateContextFromString(
    const std::string& contextStr) const
{
    // This resolver assumes the given context string will be a path to a
    // mapping file. This allows client code to call
    // ArGetResolver()->CreateContextFromString("asset", <filepath>) to create a
    // UsdResolverExampleResolverContext without having to link against this
    // library directly.
    if (TfIsFile(contextStr)) {
        return ArResolverContext(
            UsdResolverExampleResolverContext(contextStr));
    }

    return ArResolverContext();
}

bool
UsdResolverExampleResolver::_IsContextDependentPath(
    const std::string& assetPath) const
{
    // URIs that contain the "{$VERSION}" subtitution token are
    // context-dependent since they may resolve to different paths depending on
    // what resolver context is bound when Resolve is called.
    //
    // All other paths are not context-dependent since they will always resolve
    // to the same resolved path no matter what context is bound.
    return TfStringContains(assetPath, _tokens->version);
}

void
UsdResolverExampleResolver::_RefreshContext(
    const ArResolverContext& context)
{
    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg(
        "UsdResolverExample::_RefreshContext()\n");

    // If the given ArResolverContext isn't holding a context object
    // used by this resolver, there's nothing to do so we can exit.    
    const UsdResolverExampleResolverContext* ctx = 
        context.Get<UsdResolverExampleResolverContext>();
    if (!ctx) {
        return;
    }

    // Attempt to re-read the mapping file on disk. If nothing
    // has changed, we can exit.
    if (!_GetVersionTableRegistry().Refresh(ctx->GetMappingFile())) {
        return;
    }

    // Send notification that any asset resolution done with an
    // ArResolverContext holding an equivalent context object to
    // ctx has been invalidated.
    ArNotice::ResolverChanged(*ctx).Send();
}

ArTimestamp
UsdResolverExampleResolver::_GetModificationTimestamp(
    const std::string& assetPath,
    const ArResolvedPath& resolvedPath) const
{
    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg(
        "UsdResolverExampleResolver::GetModificationTimestamp('%s', '%s')\n",
        assetPath.c_str(), resolvedPath.GetPathString().c_str());

    std::string filesystemPath = _GetFilesystemPath(resolvedPath);

    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg(
        "  - Getting timestamp for %s\n", filesystemPath.c_str());
    return ArFilesystemAsset::GetModificationTimestamp(
        ArResolvedPath(std::move(filesystemPath)));
}

static std::string
_GetVersionFromResolvedPath(
    const std::string& assetPath,
    const ArResolvedPath& resolvedPath)
{
    const size_t versionStart = assetPath.find(_tokens->version);
    if (versionStart == std::string::npos) {
        return std::string();
    }

    std::string resolvedPathString = resolvedPath.GetPathString();
    const size_t versionEnd = resolvedPathString.find(
        assetPath.substr(versionStart + 10), versionStart);
    
    return std::string(
        resolvedPathString.begin() + versionStart,
        resolvedPathString.begin() + versionEnd);
}

ArAssetInfo
UsdResolverExampleResolver::_GetAssetInfo(
    const std::string& assetPath,
    const ArResolvedPath& resolvedPath) const
{
    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg(
        "UsdResolverExampleResolver::GetAssetInfo('%s', '%s')\n",
        assetPath.c_str(), resolvedPath.GetPathString().c_str());

    ArAssetInfo assetInfo;

    const _URI resolvedURI(resolvedPath);
    assetInfo.assetName = resolvedURI.GetAssetName();
    assetInfo.version = _GetVersionFromResolvedPath(assetPath, resolvedPath);
    assetInfo.resolverInfo = VtDictionary{
        {"filesystemPath", VtValue(_GetFilesystemPath(resolvedURI))}
    };

    return assetInfo;
}

std::shared_ptr<ArAsset>
UsdResolverExampleResolver::_OpenAsset(
    const ArResolvedPath& resolvedPath) const
{
    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg(
        "UsdResolverExampleResolver::OpenAsset('%s')\n",
        resolvedPath.GetPathString().c_str());

    std::string filesystemPath = _GetFilesystemPath(resolvedPath);

    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg(
        "  - Opening file at %s\n", filesystemPath.c_str());
    return ArFilesystemAsset::Open(
        ArResolvedPath(std::move(filesystemPath)));
}

std::shared_ptr<ArWritableAsset>
UsdResolverExampleResolver::_OpenAssetForWrite(
    const ArResolvedPath& resolvedPath,
    WriteMode writeMode) const
{
    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg(
        "UsdResolverExampleResolver::_OpenAssetForWrite('%s', %d)\n",
        resolvedPath.GetPathString().c_str(),
        static_cast<int>(writeMode));

    std::string filesystemPath = _GetFilesystemPath(resolvedPath);

    TF_DEBUG(USD_RESOLVER_EXAMPLE).Msg(
        "  - Opening file for write at %s\n", filesystemPath.c_str());
    return ArFilesystemWritableAsset::Create(
        ArResolvedPath(std::move(filesystemPath)), writeMode);
}
