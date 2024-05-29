//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "TestUsdResolverChangedResolver.h"

#include "pxr/usd/ar/defineResolver.h"
#include "pxr/usd/ar/defineResolverContext.h"
#include "pxr/usd/ar/filesystemAsset.h"
#include "pxr/usd/ar/notice.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/timestamp.h"

#include "pxr/base/plug/interfaceFactory.h"
#include "pxr/base/plug/staticInterface.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/vt/value.h"

#include <string>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

/// \class _TestResolver
///
class _TestResolver
    : public ArResolver
{
public:
    _TestResolver()
    {
        _configNameToAssetPathMap.swap(_pendingConfigNameToAssetPathMap);
        _configNameToVersionMap.swap(_pendingConfigNameToVersionMap);
    }

    // Map from asset name to file path for layer.
    using AssetNameToPathMap = std::unordered_map<std::string, std::string>;

    static void SetAssetPathsForConfig(
        const std::string& configName,
        const AssetNameToPathMap& assetNamesToPaths)
    {
        _pendingConfigNameToAssetPathMap[configName] = assetNamesToPaths;
    }

    static void SetVersionForConfig(
        const std::string& configName,
        const std::string& version)
    {
        _pendingConfigNameToVersionMap[configName] = version;
    }
    
    static const AssetNameToPathMap& GetAssetPathsForConfig(
        const std::string& configName)
    {
        static const AssetNameToPathMap empty;
        const AssetNameToPathMap* result = 
            TfMapLookupPtr(_configNameToAssetPathMap, configName);
        return result ? *result : empty;
    }

    static const std::string& GetVersionForConfig(
        const std::string& configName)
    {
        static const std::string empty;
        const std::string* result =
            TfMapLookupPtr(_configNameToVersionMap, configName);
        return result ? *result : empty;
    }

protected:
    std::string _CreateIdentifier(
        const std::string& assetPathIn,
        const ArResolvedPath& anchorAssetPath) const final
    {
        std::string assetPath;
        if (const _TestResolverContext* ctx =
            _GetCurrentContextObject<_TestResolverContext>()) {

            // If this asset path exists in the asset path map, just
            // return it as-is; we'll return the associated path in _Resolve.
            const AssetNameToPathMap& assetPathMap =
                GetAssetPathsForConfig(ctx->configName);
            if (assetPathMap.find(assetPathIn) != assetPathMap.end()) {
                return assetPathIn;
            }
            
            // Otherwise replace the {version} string and fall through.
            assetPath = TfStringReplace(
                assetPathIn, "{version}", GetVersionForConfig(ctx->configName));
        }
        else {
            assetPath = assetPathIn;
        }

        if (anchorAssetPath) {
            assetPath = TfStringCatPaths(
                TfGetPathName(anchorAssetPath), assetPath);
        }

        return TfAbsPath(assetPath);
    }

    ArResolvedPath _Resolve(const std::string& assetPath) const final
    {
        // If this assetPath already indicates a file, just return it.
        const std::string absAssetPath = TfAbsPath(assetPath);
        if (TfIsFile(absAssetPath)) {
            return ArResolvedPath(absAssetPath);
        }

        // See if this assetPath has an entry in the path map for the
        // configuration specified by the current context.
        if (const _TestResolverContext* ctx =
            _GetCurrentContextObject<_TestResolverContext>()) {

            const AssetNameToPathMap& assetPathMap = 
                GetAssetPathsForConfig(ctx->configName);

            if (const std::string* filePath = 
                TfMapLookupPtr(assetPathMap, assetPath)) {
                return ArResolvedPath(TfAbsPath(*filePath));
            }
        }

        return ArResolvedPath();
    }

    void _RefreshContext(const ArResolverContext& context) final
    {
        const _TestResolverContext* ctx = context.Get<_TestResolverContext>();
        if (!ctx) {
            return;
        }

        // See if there are any pending changes to the resolver configuration
        // for the config name specified in the context; if so, apply them
        // and send a ResolverChanged notice to inform listeners.

        bool didChange = false;

        auto pendingMapIter = 
            _pendingConfigNameToAssetPathMap.find(ctx->configName);
        if (pendingMapIter != _pendingConfigNameToAssetPathMap.end()) {
            _configNameToAssetPathMap[ctx->configName] = 
                std::move(pendingMapIter->second);
            _pendingConfigNameToAssetPathMap.erase(pendingMapIter);
            didChange = true;
        }

        auto pendingVersionIter =
            _pendingConfigNameToVersionMap.find(ctx->configName);
        if (pendingVersionIter != _pendingConfigNameToVersionMap.end()) {
            _configNameToVersionMap[ctx->configName] =
                std::move(pendingVersionIter->second);
            _pendingConfigNameToVersionMap.erase(pendingVersionIter);
            didChange = true;
        }

        if (didChange) {
            ArNotice::ResolverChanged(*ctx).Send();
        }
    }

    bool _IsContextDependentPath(const std::string& assetPath) const final
    {
        // This resolver deals with two types of paths that rely on the
        // config name specified in the associated resolver context:
        // - Paths with "{version}" in them
        // - Paths that are just model names, like "Buzz" or "Woody".
        return TfStringContains(assetPath, "{version}") ||
            TfGetExtension(assetPath).empty();
    }

    std::string _CreateIdentifierForNewAsset(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) const final
    {
        return assetPath;
    }

    ArResolvedPath _ResolveForNewAsset(const std::string& assetPath) const final
    {
        return ArResolvedPath();
    }

    ArTimestamp _GetModificationTimestamp(
        const std::string& assetPath,
        const ArResolvedPath& resolvedPath) const final
    {
        return ArFilesystemAsset::GetModificationTimestamp(resolvedPath);
    }

    std::shared_ptr<ArAsset> _OpenAsset(
        const ArResolvedPath& resolvedPath) const final
    {
        return ArFilesystemAsset::Open(resolvedPath);
    }

    std::shared_ptr<ArWritableAsset> _OpenAssetForWrite(
        const ArResolvedPath& resolvedPath,
        WriteMode writeMode) const final
    {
        return nullptr;
    }

private:
    using _ConfigNameToAssetPathMap = 
        std::unordered_map<std::string, AssetNameToPathMap>;
    static _ConfigNameToAssetPathMap _configNameToAssetPathMap;
    static _ConfigNameToAssetPathMap _pendingConfigNameToAssetPathMap;

    using _ConfigNameToVersionMap =
        std::unordered_map<std::string, std::string>;
    static _ConfigNameToVersionMap _configNameToVersionMap;
    static _ConfigNameToVersionMap _pendingConfigNameToVersionMap;

};

AR_DEFINE_RESOLVER(_TestResolver, ArResolver);

_TestResolver::_ConfigNameToAssetPathMap
_TestResolver::_configNameToAssetPathMap;

_TestResolver::_ConfigNameToAssetPathMap
_TestResolver::_pendingConfigNameToAssetPathMap;

_TestResolver::_ConfigNameToVersionMap
_TestResolver::_configNameToVersionMap;

_TestResolver::_ConfigNameToVersionMap
_TestResolver::_pendingConfigNameToVersionMap;

// ------------------------------------------------------------

class _TestResolverPluginImpl
    : public _TestResolverPluginInterface
{
public:
    void SetAssetPathsForConfig(
        const std::string& configName,
        const std::unordered_map<std::string, std::string>& assetPathMap) final
    {
        _TestResolver::SetAssetPathsForConfig(configName, assetPathMap);
    }

    void SetVersionForConfig(
        const std::string& configName,
        const std::string& version) final
    {
        _TestResolver::SetVersionForConfig(configName, version);
    }
};

PLUG_REGISTER_INTERFACE_SINGLETON_TYPE(
    _TestResolverPluginInterface, _TestResolverPluginImpl)

PXR_NAMESPACE_CLOSE_SCOPE
