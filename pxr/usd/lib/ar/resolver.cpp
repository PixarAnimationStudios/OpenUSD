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

#include "pxr/pxr.h"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/assetInfo.h"
#include "pxr/usd/ar/debugCodes.h"
#include "pxr/usd/ar/defaultResolver.h"
#include "pxr/usd/ar/definePackageResolver.h"
#include "pxr/usd/ar/defineResolver.h"
#include "pxr/usd/ar/packageResolver.h"
#include "pxr/usd/ar/packageUtils.h"
#include "pxr/usd/ar/resolver.h"

#include "pxr/base/vt/value.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/js/utils.h"
#include "pxr/base/js/value.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/scoped.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"

#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<ArResolver>();
}

TF_DEFINE_PRIVATE_TOKENS(_tokens,
    (extensions)
);

TF_DEFINE_ENV_SETTING(
    PXR_AR_DISABLE_PLUGIN_RESOLVER, false,
    "Disables plugin resolver implementation, falling back to default "
    "supplied by Ar.");

static TfStaticData<std::string> _preferredResolver;

void
ArSetPreferredResolver(const std::string& resolverTypeName)
{
    *_preferredResolver = resolverTypeName;
}

// ------------------------------------------------------------

ArResolver::ArResolver()
{
}

ArResolver::~ArResolver()
{
}

// ------------------------------------------------------------

namespace
{
std::string 
_GetTypeNames(const std::vector<TfType>& types)
{
    std::vector<std::string> typeNames;
    typeNames.reserve(types.size());
    for (const auto& type : types) {
        typeNames.push_back(type.GetTypeName());
    }
    return TfStringJoin(typeNames, ", ");
}

// Global stack of resolvers being constructed used by
// _CreateResolver / ArCreateResolver and ArGetAvailableResolvers.
// These functions are documented to be non-thread-safe.
static TfStaticData<std::vector<TfType>> _resolverStack;

std::vector<TfType> 
_GetAvailableResolvers()
{
    const TfType defaultResolverType = TfType::Find<ArDefaultResolver>();

    std::vector<TfType> sortedResolverTypes;

    if (!TfGetEnvSetting(PXR_AR_DISABLE_PLUGIN_RESOLVER)) {
        std::set<TfType> resolverTypes;
        PlugRegistry::GetAllDerivedTypes(
            TfType::Find<ArResolver>(), &resolverTypes);

        // Remove the default resolver so that we only process plugin types.
        // We'll add the default resolver back later.
        resolverTypes.erase(defaultResolverType);

        // Remove all resolvers that are currently under construction.
        for (const TfType& usedResolver : *_resolverStack) {
            resolverTypes.erase(usedResolver);
        }

        // We typically expect to find only one plugin asset resolver,
        // but if there's more than one we want to ensure this list is in a 
        // consistent order to ensure stable behavior. TfType's operator< 
        // is not stable across runs, so we sort based on typename instead.
        sortedResolverTypes.insert(
            sortedResolverTypes.end(), 
            resolverTypes.begin(), resolverTypes.end());
        std::sort(
            sortedResolverTypes.begin(), sortedResolverTypes.end(),
            [](const TfType& x, const TfType& y) {
                return x.GetTypeName() < y.GetTypeName();
            });
    }

    // The default resolver is always the last resolver to be considered.
    sortedResolverTypes.push_back(defaultResolverType);
    return sortedResolverTypes;
}

std::unique_ptr<ArResolver>
_CreateResolver(const TfType& resolverType, std::string* debugMsg = nullptr)
{
    _resolverStack->push_back(resolverType);
    TfScoped<> popResolverStack([]() { _resolverStack->pop_back(); });

    const TfType defaultResolverType = TfType::Find<ArDefaultResolver>();
    std::unique_ptr<ArResolver> tmpResolver;
    if (!resolverType) {
        TF_CODING_ERROR("Invalid resolver type");
    }
    else if (!resolverType.IsA<ArResolver>()) {
        TF_CODING_ERROR(
            "Given type %s does not derive from ArResolver", 
            resolverType.GetTypeName().c_str());
    }
    else if (resolverType != defaultResolverType) {
        PlugPluginPtr plugin = PlugRegistry::GetInstance()
            .GetPluginForType(resolverType);
        if (!plugin) {
            TF_CODING_ERROR("Failed to find plugin for %s", 
                resolverType.GetTypeName().c_str());
        }
        else if (!plugin->Load()) {
            TF_CODING_ERROR("Failed to load plugin %s for %s",
                plugin->GetName().c_str(),
                resolverType.GetTypeName().c_str());
        }
        else {
            Ar_ResolverFactoryBase* factory =
                resolverType.GetFactory<Ar_ResolverFactoryBase>();
            if (factory) {
                tmpResolver.reset(factory->New());
            }

            if (!tmpResolver) {
                TF_CODING_ERROR(
                    "Failed to manufacture asset resolver %s from plugin %s", 
                    resolverType.GetTypeName().c_str(), 
                    plugin->GetName().c_str());
            }
            else if (debugMsg) {
                *debugMsg = TfStringPrintf(
                    "Using asset resolver %s from plugin %s",
                    resolverType.GetTypeName().c_str(),
                    plugin->GetPath().c_str());
            }
        }
    }

    if (!tmpResolver) {
        if (debugMsg) {
            *debugMsg = TfStringPrintf("Using default asset resolver %s",
                defaultResolverType.GetTypeName().c_str());
        }
        tmpResolver.reset(new ArDefaultResolver);
    }
    
    return tmpResolver;
}

// Private ArResolver implementation that owns and forwards calls to the 
// plugin asset resolver implementation. This is used to overlay additional
// behaviors on top of the plugin resolver.
class _Resolver final
    : public ArResolver
{
public:
    _Resolver()
    {
        _InitializeUnderlyingResolver();
        _InitializePackageResolvers();
    }

    ArResolver& GetUnderlyingResolver()
    {
        return *_resolver;
    }

    virtual void ConfigureResolverForAsset(const std::string& path) override
    { 
        if (ArIsPackageRelativePath(path)) {
            _resolver->ConfigureResolverForAsset(
                ArSplitPackageRelativePathOuter(path).first);
            return;
        }
        _resolver->ConfigureResolverForAsset(path); 
    }

    virtual std::string AnchorRelativePath(
        const std::string& anchorPath, 
        const std::string& path) override
    {
        if (ArIsPackageRelativePath(path)) {
            std::pair<std::string, std::string> packagePath = 
                ArSplitPackageRelativePathOuter(path);

            packagePath.first = _resolver->AnchorRelativePath(
                ArSplitPackageRelativePathOuter(anchorPath).first,
                packagePath.first);

            return ArJoinPackageRelativePath(packagePath);
        }
        return _resolver->AnchorRelativePath(anchorPath, path);
    }

    virtual bool IsRelativePath(const std::string& path) override
    {
        if (ArIsPackageRelativePath(path)) {
            return _resolver->IsRelativePath(
                ArSplitPackageRelativePathOuter(path).first);
        }
        return _resolver->IsRelativePath(path);
    }

    virtual bool IsRepositoryPath(const std::string& path) override
    {
        if (ArIsPackageRelativePath(path)) {
            return _resolver->IsRepositoryPath(
                ArSplitPackageRelativePathOuter(path).first);
        }
        return _resolver->IsRepositoryPath(path);
    }

    virtual bool IsSearchPath(const std::string& path) override
    {
        if (ArIsPackageRelativePath(path)) {
            return _resolver->IsSearchPath(
                ArSplitPackageRelativePathOuter(path).first);
        }
        return _resolver->IsSearchPath(path);
    }

    virtual std::string GetExtension(const std::string& path) override
    {
        if (ArIsPackageRelativePath(path)) {
            // We expect clients of this API will primarily care about the
            // *packaged* asset, so we return the extension of the inner-most
            // packaged path. Clients that care about the outer package's
            // extension can split the package-relative path and call this
            // function on the package path.
            const std::pair<std::string, std::string> packagePath =
                ArSplitPackageRelativePathInner(path);
            return _resolver->GetExtension(packagePath.second);
        }
        return _resolver->GetExtension(path);
    }

    virtual std::string ComputeNormalizedPath(const std::string& path) override
    {
        if (ArIsPackageRelativePath(path)) {
            std::pair<std::string, std::string> packagePath =
                ArSplitPackageRelativePathOuter(path);
            packagePath.first = 
                _resolver->ComputeNormalizedPath(packagePath.first);
            return ArJoinPackageRelativePath(packagePath);
        }
        return _resolver->ComputeNormalizedPath(path);
    }

    virtual std::string ComputeRepositoryPath(const std::string& path) override
    {
        if (ArIsPackageRelativePath(path)) {
            std::pair<std::string, std::string> packagePath =
                ArSplitPackageRelativePathOuter(path);
            packagePath.first = 
                _resolver->ComputeRepositoryPath(packagePath.first);
            return ArJoinPackageRelativePath(packagePath);
        }
        return _resolver->ComputeRepositoryPath(path);
    }

    virtual std::string ComputeLocalPath(const std::string& path) override
    {
        if (ArIsPackageRelativePath(path)) {
            std::pair<std::string, std::string> packagePath =
                ArSplitPackageRelativePathOuter(path);
            packagePath.first = _resolver->ComputeLocalPath(packagePath.first);
            return ArJoinPackageRelativePath(packagePath);
        }
        return _resolver->ComputeLocalPath(path);
    }

    virtual std::string Resolve(const std::string& path) override
    {
        return _ResolveHelper(
            path, 
            [this](const std::string& path) {
                return _resolver->Resolve(path);
            });
    }

    virtual void BindContext(
        const ArResolverContext& context,
        VtValue* bindingData) override
    {
        _resolver->BindContext(context, bindingData);
    }

    virtual void UnbindContext(
        const ArResolverContext& context,
        VtValue* bindingData) override
    {
        _resolver->UnbindContext(context, bindingData);
    }

    virtual ArResolverContext CreateDefaultContext() override
    {
        return _resolver->CreateDefaultContext();
    }

    virtual ArResolverContext CreateDefaultContextForAsset(
        const std::string& filePath) override
    {
        if (ArIsPackageRelativePath(filePath)) {
            return _resolver->CreateDefaultContextForAsset(
                ArSplitPackageRelativePathOuter(filePath).first);
        }
        return _resolver->CreateDefaultContextForAsset(filePath);
    }

    virtual void RefreshContext(const ArResolverContext& context) override
    {
        _resolver->RefreshContext(context);
    }

    virtual ArResolverContext GetCurrentContext() override
    {
        return _resolver->GetCurrentContext();
    }

    virtual std::string ResolveWithAssetInfo(
        const std::string& path, 
        ArAssetInfo* assetInfo) override
    {
        std::string resolvedPath = _ResolveHelper(
            path, 
            [assetInfo, this](const std::string& path) {
                return _resolver->ResolveWithAssetInfo(path, assetInfo);
            });

        // If path was a package-relative path, make sure the repoPath field
        // is also a package-relative path, since the primary resolver would
        // only have been given the outer package path.
        if (assetInfo && !assetInfo->repoPath.empty() && 
            ArIsPackageRelativePath(resolvedPath)) {
            assetInfo->repoPath = ArJoinPackageRelativePath(
                assetInfo->repoPath,
                ArSplitPackageRelativePathOuter(resolvedPath).second);
        }

        return resolvedPath;
    }

    virtual void UpdateAssetInfo(
        const std::string& identifier,
        const std::string& filePath,
        const std::string& fileVersion,
        ArAssetInfo* assetInfo) override
    {
        if (ArIsPackageRelativePath(identifier)) {
            // The primary resolver is not expecting package-relative paths,
            // so we replace the repoPath field with its outermost package
            // path before passing it along. After the primary resolver
            // has updated the assetInfo object, recreate the package-relative
            // path. This matches the behavior in ResolveWithAssetInfo.
            if (!assetInfo->repoPath.empty()) {
                assetInfo->repoPath = ArSplitPackageRelativePathOuter(
                    assetInfo->repoPath).first;
            }

            std::pair<std::string, std::string> resolvedPath =
                ArSplitPackageRelativePathOuter(filePath);

            _resolver->UpdateAssetInfo(
                ArSplitPackageRelativePathOuter(identifier).first,
                resolvedPath.first,
                fileVersion, assetInfo);

            if (!assetInfo->repoPath.empty()) {
                assetInfo->repoPath = ArJoinPackageRelativePath(
                    assetInfo->repoPath, resolvedPath.second);
            }
            return;
        }

        _resolver->UpdateAssetInfo(
            identifier, filePath, fileVersion, assetInfo);
    }

    virtual VtValue GetModificationTimestamp(
        const std::string& path,
        const std::string& resolvedPath) override
    {
        if (ArIsPackageRelativePath(path)) {
            return _resolver->GetModificationTimestamp(
                ArSplitPackageRelativePathOuter(path).first,
                ArSplitPackageRelativePathOuter(resolvedPath).first);
        }
        return _resolver->GetModificationTimestamp(path, resolvedPath);
    }

    virtual bool FetchToLocalResolvedPath(
        const std::string& path,
        const std::string& resolvedPath) override
    {
        if (ArIsPackageRelativePath(path)) {
            return _resolver->FetchToLocalResolvedPath(
                ArSplitPackageRelativePathOuter(path).first,
                ArSplitPackageRelativePathOuter(resolvedPath).first);
        }
        return _resolver->FetchToLocalResolvedPath(path, resolvedPath);
    }

    virtual std::shared_ptr<ArAsset> OpenAsset(
        const std::string& resolvedPath) override
    { 
        if (ArIsPackageRelativePath(resolvedPath)) {
            const std::pair<std::string, std::string> resolvedPackagePath =
                ArSplitPackageRelativePathInner(resolvedPath);

            ArPackageResolver* packageResolver = 
                _GetPackageResolver(resolvedPackagePath.first);
            if (packageResolver) {
                return packageResolver->OpenAsset(
                    resolvedPackagePath.first, resolvedPackagePath.second);
            }
            return nullptr;
        }
        return _resolver->OpenAsset(resolvedPath);
    }

    virtual bool CanWriteLayerToPath(
        const std::string& path,
        std::string* whyNot) override
    {
        if (ArIsPackageRelativePath(path)) {
            return _resolver->CanWriteLayerToPath(
                ArSplitPackageRelativePathOuter(path).first, whyNot);
        }
        return _resolver->CanWriteLayerToPath(path, whyNot);
    }

    virtual bool CanCreateNewLayerWithIdentifier(
        const std::string& identifier, 
        std::string* whyNot) override
    {
        if (ArIsPackageRelativePath(identifier)) {
            return _resolver->CanCreateNewLayerWithIdentifier(
                ArSplitPackageRelativePathOuter(identifier).first, whyNot);
        }
        return _resolver->CanCreateNewLayerWithIdentifier(
            identifier, whyNot);
    }

    // The underlying resolver and the package resolvers all participate in
    // scoped caching and may have caching-related data to store away. To
    // accommodate this, _Resolver stores away a vector of VtValues where
    // each element corresponds to either the underlying resolver or a
    // package resolver.
    using _ResolverCacheData = std::vector<VtValue>;

    virtual void BeginCacheScope(
        VtValue* cacheScopeData) override
    {
        // If we've filled in cacheScopeData from a previous call to
        // BeginCacheScope, extract the _ResolverCacheData so we can pass
        // each of the VtValues to the corresponding resolver. 
        _ResolverCacheData cacheData;
        if (cacheScopeData->IsHolding<_ResolverCacheData>()) {
            cacheScopeData->UncheckedSwap(cacheData);
        }
        else {
            cacheData.resize(1 + _GetNumPackageResolvers());
        }

        TF_VERIFY(cacheData.size() == 1 + _GetNumPackageResolvers());

        _resolver->BeginCacheScope(&cacheData[0]);
        for (size_t i = 0, e = _GetNumPackageResolvers(); i != e; ++i) {
            ArPackageResolver* packageResolver = _GetPackageResolver(i);
            if (packageResolver) {
                packageResolver->BeginCacheScope(&cacheData[i+1]);
            }
        }

        cacheScopeData->Swap(cacheData);
    }

    virtual void EndCacheScope(
        VtValue* cacheScopeData) override
    {
        if (!TF_VERIFY(cacheScopeData->IsHolding<_ResolverCacheData>())) {
            return;
        }

        _ResolverCacheData cacheData;
        cacheScopeData->UncheckedSwap(cacheData);

        _resolver->EndCacheScope(&cacheData[0]);
        for (size_t i = 0, e = _GetNumPackageResolvers(); i != e; ++i) {
            ArPackageResolver* packageResolver = _GetPackageResolver(i);
            if (packageResolver) {
                packageResolver->EndCacheScope(&cacheData[i+1]);
            }
        }

        cacheScopeData->Swap(cacheData);
    }

private:
    void _InitializeUnderlyingResolver()
    {
        const TfType defaultResolverType = TfType::Find<ArDefaultResolver>();

        std::vector<TfType> resolverTypes;
        if (TfGetEnvSetting(PXR_AR_DISABLE_PLUGIN_RESOLVER)) {
            TF_DEBUG(AR_RESOLVER_INIT).Msg(
                "ArGetResolver(): Plugin asset resolver disabled via "
                "PXR_AR_DISABLE_PLUGIN_RESOLVER.\n");
        }
        else if (!_preferredResolver->empty()) {
            const TfType resolverType = 
                PlugRegistry::FindTypeByName(*_preferredResolver);
            if (!resolverType) {
                TF_WARN(
                    "ArGetResolver(): Preferred resolver %s not found. "
                    "Using default resolver.",
                    _preferredResolver->c_str());
                resolverTypes.push_back(defaultResolverType);
            }
            else if (!resolverType.IsA<ArResolver>()) {
                TF_WARN(
                    "ArGetResolver(): Preferred resolver %s does not derive "
                    "from ArResolver. Using default resolver.\n",
                    _preferredResolver->c_str());
                resolverTypes.push_back(defaultResolverType);
            }
            else {
                TF_DEBUG(AR_RESOLVER_INIT).Msg(
                    "ArGetResolver(): Using preferred resolver %s\n",
                    _preferredResolver->c_str());
                resolverTypes.push_back(resolverType);
            }
        }

        if (resolverTypes.empty()) {
            resolverTypes = _GetAvailableResolvers();

            TF_DEBUG(AR_RESOLVER_INIT).Msg(
                "ArGetResolver(): Found asset resolver types: [%s]\n",
                _GetTypeNames(resolverTypes).c_str());
        }

        std::string debugMsg;

        // resolverTypes should never be empty -- _GetAvailableResolvers
        // should always return at least the default resolver. Because of this,
        // if there's more than 2 elements in resolverTypes, there must have 
        // been more than one resolver from an external plugin.
        if (TF_VERIFY(!resolverTypes.empty())) {
            const TfType& resolverType = resolverTypes.front();
            if (resolverTypes.size() > 2) {
                TF_DEBUG(AR_RESOLVER_INIT).Msg(
                    "ArGetResolver(): Found multiple plugin asset "
                    "resolvers, using %s\n", 
                    resolverType.GetTypeName().c_str());
            }

            _resolver = _CreateResolver(resolverType, &debugMsg);
        }

        if (!_resolver) {
            _resolver = _CreateResolver(defaultResolverType, &debugMsg);
        }

        TF_DEBUG(AR_RESOLVER_INIT).Msg(
            "ArGetResolver(): %s\n", debugMsg.c_str());
    }

    void _InitializePackageResolvers()
    {
        std::set<TfType> packageResolverTypes;
        PlugRegistry::GetAllDerivedTypes<ArPackageResolver>(
            &packageResolverTypes);

        _packageResolvers.reserve(packageResolverTypes.size());

        PlugRegistry& plugReg = PlugRegistry::GetInstance();
        for (const TfType& packageResolverType : packageResolverTypes) {
            TF_DEBUG(AR_RESOLVER_INIT).Msg(
                "ArGetResolver(): Found package resolver %s\n",
                packageResolverType.GetTypeName().c_str());

            const PlugPluginPtr plugin = 
                plugReg.GetPluginForType(packageResolverType);
            if (!plugin) {
                TF_DEBUG(AR_RESOLVER_INIT).Msg(
                    "ArGetResolver(): Skipping package resolver %s because "
                    "plugin cannot be found\n",
                    packageResolverType.GetTypeName().c_str());
                continue;
            }

            const JsOptionalValue extensionsVal = JsFindValue(
                plugin->GetMetadataForType(packageResolverType),
                _tokens->extensions.GetString());
            if (!extensionsVal) {
                TF_RUNTIME_ERROR(
                    "No package formats specified in '%s' metadata for '%s'",
                    _tokens->extensions.GetText(), 
                    packageResolverType.GetTypeName().c_str());
                continue;
            }

            std::vector<std::string> extensions;
            {
                TfErrorMark m;
                extensions = extensionsVal->GetArrayOf<std::string>();
                if (m.Clear()) {
                    TF_RUNTIME_ERROR(
                        "Expected list of formats in '%s' metadata for '%s'",
                        _tokens->extensions.GetText(), 
                        packageResolverType.GetTypeName().c_str());
                    continue;
                }
            }

            for (const std::string& extension : extensions) {
                if (extension.empty()) {
                    continue;
                }

                _PackageResolverSharedPtr r(new _PackageResolver);
                r->plugin = plugin;
                r->resolverType = packageResolverType;
                r->packageFormat = extension;
                _packageResolvers.push_back(std::move(r));

                TF_DEBUG(AR_RESOLVER_INIT).Msg(
                    "ArGetResolver(): Using package resolver %s for %s "
                    "from plugin %s\n", 
                    packageResolverType.GetTypeName().c_str(),
                    extension.c_str(), plugin->GetName().c_str());
            }
        }
    }

    size_t
    _GetNumPackageResolvers() const
    {
        return _packageResolvers.size();
    }

    ArPackageResolver*
    _GetPackageResolver(const std::string& packageRelativePath)
    {
        const std::string innermostPackage = 
            ArSplitPackageRelativePathInner(packageRelativePath).first;
        const std::string format = GetExtension(innermostPackage);

        for (size_t i = 0, e = _GetNumPackageResolvers(); i != e; ++i) {
            if (_packageResolvers[i]->packageFormat == format) {
                return _GetPackageResolver(i);
            }
        }

        return nullptr;
    }

    ArPackageResolver*
    _GetPackageResolver(size_t resolverIdx)
    {
        const _PackageResolverSharedPtr& r = _packageResolvers[resolverIdx];
        if (!r->hasResolver) {
            r->plugin->Load();

            std::shared_ptr<ArPackageResolver> newResolver;

            Ar_PackageResolverFactoryBase* factory = 
                r->resolverType.GetFactory<Ar_PackageResolverFactoryBase>();
            if (factory) {
                newResolver.reset(factory->New());
            }

            std::lock_guard<std::mutex> g(r->mutex);
            if (!r->hasResolver) {
                r->resolver = newResolver;
                r->hasResolver = true;
            }
        }
        
        return r->resolver.get();
    }

    template <class ResolveFn>
    std::string
    _ResolveHelper(const std::string& path, ResolveFn resolveFn)
    {
        if (ArIsPackageRelativePath(path)) {
            // Resolve the outer-most package path first. For example, given a
            // path like "/path/to/p.package_a[sub.package_b[asset.file]]", the
            // underlying resolver needs to resolve "/path/to/p.package_a"
            // since this is a 'real' asset in the client's asset system. 
            std::string packagePath, packagedPath;
            std::tie(packagePath, packagedPath) = 
                ArSplitPackageRelativePathOuter(path);

            std::string resolvedPackagePath = resolveFn(packagePath);
            if (resolvedPackagePath.empty()) {
                return std::string();
            }

            // Loop through the remaining packaged paths and resolve each
            // of them using the appropriate package resolver. In the above
            // example, this loop would:
            //
            //   - Resolve "sub.package_b" in package "/path/to/p.package_a"
            //   - Resolve "asset.file" in "/path/to/p.package_a[sub.package_b]"
            //
            while (!packagedPath.empty()) {
                std::tie(packagePath, packagedPath) = 
                    ArSplitPackageRelativePathOuter(packagedPath);
                
                ArPackageResolver* packageResolver =
                    _GetPackageResolver(resolvedPackagePath);
                if (!packageResolver) {
                    return std::string();
                }

                packagePath = packageResolver->Resolve(
                    resolvedPackagePath, packagePath);
                if (packagePath.empty()) {
                    return std::string();
                }

                resolvedPackagePath = ArJoinPackageRelativePath(
                    resolvedPackagePath, packagePath);
            }

            return resolvedPackagePath;
        }

        return resolveFn(path);
    }

    std::unique_ptr<ArResolver> _resolver;

    struct _PackageResolver
    {
        _PackageResolver()
            : hasResolver(false) { }

        PlugPluginPtr plugin;
        TfType resolverType;
        std::string packageFormat;

        std::atomic<bool> hasResolver;
        std::mutex mutex;
        std::shared_ptr<ArPackageResolver> resolver;
    };

    using _PackageResolverSharedPtr = std::shared_ptr<_PackageResolver>;
    std::vector<_PackageResolverSharedPtr> _packageResolvers;
};

_Resolver&
_GetResolver()
{
    // If other threads enter this function while another thread is 
    // constructing the resolver, it's guaranteed that those threads
    // will wait until the resolver is constructed.
    static _Resolver resolver;
    return resolver;
}

} // end anonymous namespace



ArResolver& 
ArGetResolver()
{
    return _GetResolver();
}

ArResolver&
ArGetUnderlyingResolver()
{
    return _GetResolver().GetUnderlyingResolver();
}

std::vector<TfType>
ArGetAvailableResolvers()
{
    return _GetAvailableResolvers();
}

std::unique_ptr<ArResolver>
ArCreateResolver(const TfType& resolverType)
{
    return _CreateResolver(resolverType);
}

PXR_NAMESPACE_CLOSE_SCOPE
