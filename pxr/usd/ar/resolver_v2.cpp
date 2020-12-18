//
// Copyright 2020 Pixar
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
#include "pxr/usd/ar/resolvedPath.h"
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
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"

#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<ArResolver>();
}

TF_DEFINE_PRIVATE_TOKENS(_tokens,
    // Plugin metadata key for package resolver extensions.
    (extensions)
    
    // Plugin metadata key for resolver URI schemes.
    (uriSchemes)
);

TF_DEFINE_ENV_SETTING(
    PXR_AR_DISABLE_PLUGIN_RESOLVER, false,
    "Disables plugin resolver implementation, falling back to default "
    "supplied by Ar.");

TF_DEFINE_ENV_SETTING(
    PXR_AR_DISABLE_PLUGIN_URI_RESOLVERS, false,
    "Disables plugin URI resolver implementations.");

static TfStaticData<std::string> _preferredResolver;

void
ArSetPreferredResolver(const std::string& resolverTypeName)
{
    *_preferredResolver = resolverTypeName;
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

class _ResolverInfo
{
public:
    _ResolverInfo()
        : canBePrimaryResolver(false)
    { }

    // Plugin for the resolver implementation.
    PlugPluginPtr plugin;

    // TfType for the resolver implementation.
    TfType type;

    // URI schemes associated with the resolver implementation.
    std::vector<std::string> uriSchemes;

    // Whether this resolver can be used as a primary resolver.
    bool canBePrimaryResolver;
};

std::vector<_ResolverInfo> 
_GetAvailableResolvers()
{
    std::vector<TfType> sortedResolverTypes;
    {
        std::set<TfType> resolverTypes;
        PlugRegistry::GetAllDerivedTypes(
            TfType::Find<ArResolver>(), &resolverTypes);

        // Ensure this list is in a consistent order to ensure stable behavior.
        // TfType's operator< is not stable across runs, so we sort based on
        // typename instead.
        sortedResolverTypes.assign(resolverTypes.begin(), resolverTypes.end());
        std::sort(
            sortedResolverTypes.begin(), sortedResolverTypes.end(),
            [](const TfType& x, const TfType& y) {
                return x.GetTypeName() < y.GetTypeName();
            });
    }

    std::vector<_ResolverInfo> resolvers;
    resolvers.reserve(sortedResolverTypes.size());

    // Fill in the URI schemes associated with each available resolver.
    const PlugRegistry& plugRegistry = PlugRegistry::GetInstance();
    for (const TfType& resolverType : sortedResolverTypes) {
        PlugPluginPtr plugin = plugRegistry.GetPluginForType(resolverType);
        if (!plugin) {
            TF_CODING_ERROR(
                "Failed to find plugin for %s", 
                resolverType.GetTypeName().c_str());
            continue;
        }

        std::vector<std::string> uriSchemes;
        if (const JsOptionalValue uriSchemesVal = JsFindValue(
                plugin->GetMetadataForType(resolverType),
                _tokens->uriSchemes.GetString())) {

            if (uriSchemesVal->IsArrayOf<std::string>()) {
                uriSchemes = uriSchemesVal->GetArrayOf<std::string>();
            }
            else {
                TF_CODING_ERROR(
                    "'%s' metadata for %s must be a list of strings.",
                    _tokens->uriSchemes.GetText(),
                    resolverType.GetTypeName().c_str());
                continue;
            }
        }

        _ResolverInfo info;
        info.plugin = plugin;
        info.type = resolverType;
        info.uriSchemes = std::move(uriSchemes);
        info.canBePrimaryResolver = info.uriSchemes.empty();

        resolvers.push_back(std::move(info));
    }

    return resolvers;
}

std::vector<TfType>
_GetAvailablePrimaryResolvers(
    const std::vector<_ResolverInfo>& availableResolvers)
{
    const TfType defaultResolverType = TfType::Find<ArDefaultResolver>();

    std::vector<TfType> availablePrimaryResolvers;

    const std::vector<_ResolverInfo> emptyResolverList;
    const std::vector<_ResolverInfo>* allAvailableResolvers = 
        TfGetEnvSetting(PXR_AR_DISABLE_PLUGIN_RESOLVER) ? 
        &emptyResolverList : &availableResolvers;

    for (const _ResolverInfo& resolver : *allAvailableResolvers) {
        // Skip resolvers that are not marked as a potential primary resolver.
        if (!resolver.canBePrimaryResolver) {
            continue;
        }

        // Skip the default resolver so that we only process plugin types.
        // We'll add the default resolver back later.
        if (resolver.type == defaultResolverType) {
            continue;
        }

        // Skip all resolvers that are currently under construction.
        if (std::find(
                _resolverStack->begin(), _resolverStack->end(), resolver.type)
            != _resolverStack->end()) {
            continue;
        }

        availablePrimaryResolvers.push_back(resolver.type);
    }

    // The default resolver is always the last resolver to be considered.
    availablePrimaryResolvers.push_back(defaultResolverType);

    return availablePrimaryResolvers;
}

// Helper class to manage plugin resolvers that are loaded on-demand.
template <class ResolverType, class ResolverTypeFactory>
class _PluginResolver
{
public:
    _PluginResolver(
        const PlugPluginPtr& plugin,
        const TfType& resolverType,
        const std::shared_ptr<ResolverType>& resolver = nullptr)
        : _plugin(plugin)
        , _resolverType(resolverType)
        , _hasResolver(static_cast<bool>(resolver))
        , _resolver(resolver)
    {
    }

    const TfType& GetType() const { return _resolverType; }

    std::unique_ptr<ResolverType> Create() 
    {
        std::unique_ptr<ResolverType> resolver;

        if (!_plugin->Load()) {
            TF_CODING_ERROR("Failed to load plugin %s for %s",
                _plugin->GetName().c_str(),
                _resolverType.GetTypeName().c_str());
            return nullptr;
        }

        ResolverTypeFactory* factory =
            _resolverType.GetFactory<ResolverTypeFactory>();
        if (factory) {
            resolver.reset(factory->New());
        }

        if (!resolver) {
            TF_CODING_ERROR(
                "Failed to manufacture asset resolver %s from plugin %s", 
                _resolverType.GetTypeName().c_str(), 
                _plugin->GetName().c_str());
        }

        return resolver;
    }
    
    ResolverType* Get()
    {
        if (!_hasResolver) {
            std::unique_ptr<ResolverType> newResolver = Create();
            
            std::lock_guard<std::mutex> g(_mutex);
            if (!_hasResolver) {
                _resolver.reset(newResolver.release());
                _hasResolver = true;
            }
        }
        return _resolver.get();
    };

private:
    PlugPluginPtr _plugin;
    TfType _resolverType;

    std::atomic<bool> _hasResolver;
    std::mutex _mutex;
    std::shared_ptr<ResolverType> _resolver;
};

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

        tmpResolver = _PluginResolver<ArResolver, Ar_ResolverFactoryBase>(
            plugin, resolverType).Create();

        if (tmpResolver && debugMsg) {
            *debugMsg = TfStringPrintf(
                "Using asset resolver %s from plugin %s",
                resolverType.GetTypeName().c_str(),
                plugin->GetPath().c_str());
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
        : _maxURISchemeLength(0)
    {
        const std::vector<_ResolverInfo> availableResolvers =
            _GetAvailableResolvers();

        _InitializePrimaryResolver(availableResolvers);
        _InitializeURIResolvers(availableResolvers);
        _InitializePackageResolvers();
    }

    ArResolver& GetPrimaryResolver()
    {
        return *_resolver;
    }

    ArResolverContext CreateContextFromString(
        const std::string& uriScheme, const std::string& contextStr)
    {
        ArResolver* resolver =
            uriScheme.empty() ?
            _resolver.get()  : _GetURIResolverForScheme(uriScheme);
        return resolver ? 
            resolver->CreateContextFromString(contextStr) : ArResolverContext();
    }

    ArResolverContext CreateContextFromStrings(
        const std::vector<std::pair<std::string, std::string>>& strs)
    {
        std::vector<ArResolverContext> contexts;
        contexts.reserve(strs.size());

        for (const auto& entry : strs) {
            ArResolverContext ctx =
                CreateContextFromString(entry.first, entry.second);
            if (!ctx.IsEmpty()) {
                contexts.push_back(std::move(ctx));
            }
        };

        return ArResolverContext(contexts);
    }

    virtual void ConfigureResolverForAsset(const std::string& path) override
    { 
        ArResolver& resolver = _GetResolver(path);
        if (ArIsPackageRelativePath(path)) {
            resolver.ConfigureResolverForAsset(
                ArSplitPackageRelativePathOuter(path).first);
            return;
        }
        resolver.ConfigureResolverForAsset(path); 
    }

    virtual std::string AnchorRelativePath(
        const std::string& anchorPath, 
        const std::string& path) override
    {
        // If path has a recognized URI scheme, we assume it's an
        // absolute URI per RFC 3986 sec 4.3 and just return it as-is.
        // Otherwise, it's unclear whether the resolver for anchorPath
        // or path should be responsible for handling this query.
        //
        // If path does not have a recognized URI scheme, we delegate
        // to the resolver for anchorPath to do the anchoring. Although
        // we could implement URI anchoring per RFC 3986 sec 5 here,
        // we want to give implementations the chance to do additional
        // manipulations during the anchoring process.
        ArResolver* resolver = _GetURIResolver(path);
        if (resolver) {
            return path;
        }
        else {
            resolver = &_GetResolver(anchorPath);
        }

        if (ArIsPackageRelativePath(path)) {
            std::pair<std::string, std::string> packagePath = 
                ArSplitPackageRelativePathOuter(path);

            packagePath.first = resolver->AnchorRelativePath(
                ArSplitPackageRelativePathOuter(anchorPath).first,
                packagePath.first);

            return ArJoinPackageRelativePath(packagePath);
        }
        return resolver->AnchorRelativePath(anchorPath, path);
    }

    virtual std::string _CreateIdentifier(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) override
    {
        return _CreateIdentifierHelper(
            assetPath, anchorAssetPath,
            [](ArResolver& resolver, const std::string& assetPath,
               const ArResolvedPath& anchorAssetPath) {
                return resolver.CreateIdentifier(assetPath, anchorAssetPath);
            });
    }

    virtual std::string _CreateIdentifierForNewAsset(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) override
    {
        return _CreateIdentifierHelper(
            assetPath, anchorAssetPath,
            [](ArResolver& resolver, const std::string& assetPath,
               const ArResolvedPath& anchorAssetPath) {
                return resolver.CreateIdentifierForNewAsset(
                    assetPath, anchorAssetPath);
            });
    }

    template <class CreateIdentifierFn>
    std::string _CreateIdentifierHelper(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath,
        const CreateIdentifierFn& createIdentifierFn)
    {
        // If assetPath has a recognized URI scheme, we assume it's an absolute
        // URI per RFC 3986 sec 4.3 and delegate to the associated URI resolver
        // to handle this query.
        //
        // If path does not have a recognized URI scheme, we delegate to the
        // resolver for the anchorAssetPath. Although we could implement URI
        // anchoring per RFC 3986 sec 5 here, we want to give implementations
        // the chance to do additional manipulations.
        ArResolver* resolver = _GetURIResolver(assetPath);
        if (!resolver) {
            resolver = &_GetResolver(anchorAssetPath);
        }

        // XXX: 
        // If the anchorAssetPath is a package-relative path like
        // /foo/bar.package[baz.file], we curently just use the outer package
        // path as the anchoring asset. It might be more consistent if we
        // used the inner *packaged* path as the anchor instead. Since the
        // packaged path syntax is fully under Ar's control, we might not
        // dispatch to any other resolver in this case and just anchor
        // the packaged path and given assetPath ourselves.
        const ArResolvedPath anchorResolvedPath(
            ArSplitPackageRelativePathOuter(anchorAssetPath).first);

        if (ArIsPackageRelativePath(assetPath)) {
            std::pair<std::string, std::string> packageAssetPath =
                ArSplitPackageRelativePathOuter(assetPath);
            packageAssetPath.first = createIdentifierFn(
                *resolver, packageAssetPath.first, anchorResolvedPath);

            return ArJoinPackageRelativePath(packageAssetPath);
        }

        return createIdentifierFn(*resolver, assetPath, anchorResolvedPath);
    }

    virtual bool IsRelativePath(const std::string& path) override
    {
        // See AnchorRelativePath.
        ArResolver* resolver = _GetURIResolver(path);
        if (resolver) {
            return false;
        }
        else {
            resolver = &_GetResolver(path);
        }

        if (ArIsPackageRelativePath(path)) {
            return resolver->IsRelativePath(
                ArSplitPackageRelativePathOuter(path).first);
        }
        return resolver->IsRelativePath(path);
    }

    virtual bool _IsContextDependentPath(
        const std::string& assetPath)
    {
        ArResolver& resolver = _GetResolver(assetPath);
        if (ArIsPackageRelativePath(assetPath)) {
            return resolver.IsContextDependentPath(
                ArSplitPackageRelativePathOuter(assetPath).first);
        }
        return resolver.IsContextDependentPath(assetPath);
    }

    virtual bool _IsRepositoryPath(const std::string& path) override
    {
        ArResolver& resolver = _GetResolver(path);
        if (ArIsPackageRelativePath(path)) {
            return resolver.IsRepositoryPath(
                ArSplitPackageRelativePathOuter(path).first);
        }
        return resolver.IsRepositoryPath(path);
    }

    virtual bool IsSearchPath(const std::string& path) override
    {
        ArResolver& resolver = _GetResolver(path);
        if (ArIsPackageRelativePath(path)) {
            return resolver.IsSearchPath(
                ArSplitPackageRelativePathOuter(path).first);
        }
        return resolver.IsSearchPath(path);
    }

    virtual std::string _GetExtension(const std::string& path) override
    {
        ArResolver& resolver = _GetResolver(path);
        if (ArIsPackageRelativePath(path)) {
            // We expect clients of this API will primarily care about the
            // *packaged* asset, so we return the extension of the inner-most
            // packaged path. Clients that care about the outer package's
            // extension can split the package-relative path and call this
            // function on the package path.
            //
            // XXX: This doesn't seem right. If Ar is defining the packaged
            // path syntax, then Ar should be responsible for getting the
            // extension for packaged paths instead of delegating.
            const std::pair<std::string, std::string> packagePath =
                ArSplitPackageRelativePathInner(path);
            return resolver.GetExtension(packagePath.second);
        }
        return resolver.GetExtension(path);
    }

    // The primary resolver and the URI resolvers all participate
    // in context binding and may have context-related data to store
    // away. To accommodate this, _Resolve stores away a vector of
    // VtValues where each element corresponds to the primary resolver
    // or a URI resolver.
    using _ResolverContextData = std::vector<VtValue>;

    virtual void _BindContext(
        const ArResolverContext& context,
        VtValue* bindingData) override
    {
        _ResolverContextData contextData(1 + _uriResolvers.size());

        size_t dataIndex = 0;

        _resolver->BindContext(context, &contextData[dataIndex]);
        ++dataIndex;

        for (const auto& entry : _uriResolvers) {
            if (ArResolver* uriResolver = entry.second->Get()) {
                uriResolver->BindContext(context, &contextData[dataIndex]);
            }
            ++dataIndex;
        }

        bindingData->Swap(contextData);
    }

    virtual void _UnbindContext(
        const ArResolverContext& context,
        VtValue* bindingData) override
    {
        if (!TF_VERIFY(bindingData->IsHolding<_ResolverContextData>())) {
            return;
        }

        _ResolverContextData contextData;
        bindingData->UncheckedSwap(contextData);

        size_t dataIndex = 0;

        _resolver->UnbindContext(context, &contextData[dataIndex]);
        ++dataIndex;

        for (const auto& entry : _uriResolvers) {
            if (ArResolver* uriResolver = entry.second->Get()) {
                uriResolver->UnbindContext(context, &contextData[dataIndex]);
            }
            ++dataIndex;
        }

        bindingData->Swap(contextData);
    }

    virtual ArResolverContext _CreateDefaultContext() override
    {
        std::vector<ArResolverContext> contexts;

        contexts.push_back(_resolver->CreateDefaultContext());

        for (const auto& entry : _uriResolvers) {
            if (ArResolver* uriResolver = entry.second->Get()) {
                contexts.push_back(uriResolver->CreateDefaultContext());
            }
        }

        return ArResolverContext(contexts);
    }

    virtual ArResolverContext _CreateContextFromString(
        const std::string& str) override
    {
        return _resolver->CreateContextFromString(str);
    }

    virtual ArResolverContext _CreateDefaultContextForAsset(
        const std::string& filePath) override
    {
        ArResolver& resolver = _GetResolver(filePath);
        if (ArIsPackageRelativePath(filePath)) {
            return resolver.CreateDefaultContextForAsset(
                ArSplitPackageRelativePathOuter(filePath).first);
        }
        return resolver.CreateDefaultContextForAsset(filePath);
    }

    virtual void _RefreshContext(const ArResolverContext& context) override
    {
        _resolver->RefreshContext(context);
        for (const auto& entry : _uriResolvers) {
            if (ArResolver* uriResolver = entry.second->Get()) {
                uriResolver->RefreshContext(context);
            }
        }
    }

    virtual ArResolverContext _GetCurrentContext() override
    {
        // XXX:
        // This assumes that when binding a context, each resolver
        // will only return an ArResolverContext containing just the
        // context objects that are relevant. This could be cleaned
        // up if _Resolver itself were responsible for managing the
        // "current context" instead of making each resolver subclass
        // responsible for that.
        std::vector<ArResolverContext> contexts;
        contexts.push_back(_resolver->GetCurrentContext());
        for (const auto& entry : _uriResolvers) {
            if (ArResolver* uriResolver = entry.second->Get()) {
                contexts.push_back(uriResolver->GetCurrentContext());
            }
        }
        return ArResolverContext(contexts);
    }

    virtual ArResolvedPath _Resolve(
        const std::string& assetPath) override
    {
        return _ResolveHelper(
            assetPath, 
            [this](const std::string& path) {
                return _GetResolver(path).Resolve(path);
            });
    }

    virtual ArResolvedPath _ResolveForNewAsset(
        const std::string& assetPath) override
    {
        ArResolver& resolver = _GetResolver(assetPath);
        if (ArIsPackageRelativePath(assetPath)) {
            std::pair<std::string, std::string> packagePath =
                ArSplitPackageRelativePathOuter(assetPath);
            packagePath.first = resolver.ResolveForNewAsset(packagePath.first);
            return ArResolvedPath(ArJoinPackageRelativePath(packagePath));
        }
        return resolver.ResolveForNewAsset(assetPath);
    };

    virtual ArAssetInfo _GetAssetInfo(
        const std::string& assetPath,
        const ArResolvedPath& resolvedPath)
    {
        ArAssetInfo assetInfo;

        ArResolver& resolver = _GetResolver(assetPath);
        if (ArIsPackageRelativePath(assetPath)) {
            std::pair<std::string, std::string> packageAssetPath =
                ArSplitPackageRelativePathOuter(assetPath);
            std::pair<std::string, std::string> packageResolvedPath =
                ArSplitPackageRelativePathOuter(resolvedPath);                

            assetInfo = resolver.GetAssetInfo(
                packageAssetPath.first,
                ArResolvedPath(packageResolvedPath.first));

            // If resolvedPath was a package-relative path, make sure the
            // repoPath field is also a package-relative path, since the primary
            // resolver would only have been given the outer package path.
            if (!assetInfo.repoPath.empty()) {
                assetInfo.repoPath = ArJoinPackageRelativePath(
                    assetInfo.repoPath, packageResolvedPath.second);
            }

            return assetInfo;
        }
        return resolver.GetAssetInfo(assetPath, resolvedPath);
    }

    virtual VtValue _GetModificationTimestamp(
        const std::string& path,
        const ArResolvedPath& resolvedPath) override
    {
        ArResolver& resolver = _GetResolver(path);
        if (ArIsPackageRelativePath(path)) {
            return resolver.GetModificationTimestamp(
                ArSplitPackageRelativePathOuter(path).first,
                ArResolvedPath(
                    ArSplitPackageRelativePathOuter(resolvedPath).first));
        }
        return resolver.GetModificationTimestamp(path, resolvedPath);
    }

    virtual bool FetchToLocalResolvedPath(
        const std::string& path,
        const std::string& resolvedPath) override
    {
        ArResolver& resolver = _GetResolver(path);
        if (ArIsPackageRelativePath(path)) {
            return resolver.FetchToLocalResolvedPath(
                ArSplitPackageRelativePathOuter(path).first,
                ArSplitPackageRelativePathOuter(resolvedPath).first);
        }
        return resolver.FetchToLocalResolvedPath(path, resolvedPath);
    }

    virtual std::shared_ptr<ArAsset> _OpenAsset(
        const ArResolvedPath& resolvedPath) override
    { 
        ArResolver& resolver = _GetResolver(resolvedPath);
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
        return resolver.OpenAsset(resolvedPath);
    }

    virtual std::shared_ptr<ArWritableAsset> _OpenAssetForWrite(
        const ArResolvedPath& resolvedPath,
        WriteMode mode) override
    {
        ArResolver& resolver = _GetResolver(resolvedPath);
        if (ArIsPackageRelativePath(resolvedPath)) {
            TF_CODING_ERROR("Cannot open package-relative paths for write");
            return nullptr;
        };
        return resolver.OpenAssetForWrite(resolvedPath, mode);
    }

    virtual bool CreatePathForLayer(
        const std::string& path) override
    {
        ArResolver& resolver = _GetResolver(path);
        if (ArIsPackageRelativePath(path)) {
            return resolver.CreatePathForLayer(
                ArSplitPackageRelativePathOuter(path).first);
        }
        return resolver.CreatePathForLayer(path);
    }

    virtual bool CanWriteLayerToPath(
        const std::string& path,
        std::string* whyNot) override
    {
        ArResolver& resolver = _GetResolver(path);
        if (ArIsPackageRelativePath(path)) {
            return resolver.CanWriteLayerToPath(
                ArSplitPackageRelativePathOuter(path).first, whyNot);
        }
        return resolver.CanWriteLayerToPath(path, whyNot);
    }

    virtual bool CanCreateNewLayerWithIdentifier(
        const std::string& identifier, 
        std::string* whyNot) override
    {
        ArResolver& resolver = _GetResolver(identifier);
        if (ArIsPackageRelativePath(identifier)) {
            return resolver.CanCreateNewLayerWithIdentifier(
                ArSplitPackageRelativePathOuter(identifier).first, whyNot);
        }
        return resolver.CanCreateNewLayerWithIdentifier(
            identifier, whyNot);
    }

    // The primary resolver and the package resolvers all participate in
    // scoped caching and may have caching-related data to store away. To
    // accommodate this, _Resolver stores away a vector of VtValues where
    // each element corresponds to either the primary resolver or a
    // package resolver.
    using _ResolverCacheData = std::vector<VtValue>;

    virtual void _BeginCacheScope(
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
            cacheData.resize(
                1 + _packageResolvers.size() + _uriResolvers.size());
        }

        TF_VERIFY(cacheData.size() == 
            1 + _packageResolvers.size() + _uriResolvers.size());

        size_t cacheDataIndex = 0;

        _resolver->BeginCacheScope(&cacheData[cacheDataIndex]);
        ++cacheDataIndex;

        for (const auto& entry : _uriResolvers) {
            if (ArResolver* uriResolver = entry.second->Get()) {
                uriResolver->BeginCacheScope(&cacheData[cacheDataIndex]);
            }
            ++cacheDataIndex;
        }

        for (size_t i = 0, e = _packageResolvers.size(); i != e;
             ++i, ++cacheDataIndex) {
            ArPackageResolver* packageResolver = _packageResolvers[i]->Get();
            if (packageResolver) {
                packageResolver->BeginCacheScope(&cacheData[cacheDataIndex]);
            }
        }

        cacheScopeData->Swap(cacheData);
    }

    virtual void _EndCacheScope(
        VtValue* cacheScopeData) override
    {
        if (!TF_VERIFY(cacheScopeData->IsHolding<_ResolverCacheData>())) {
            return;
        }

        _ResolverCacheData cacheData;
        cacheScopeData->UncheckedSwap(cacheData);

        size_t cacheDataIndex = 0;

        _resolver->EndCacheScope(&cacheData[cacheDataIndex]);
        ++cacheDataIndex;

        for (const auto& entry : _uriResolvers) {
            if (ArResolver* uriResolver = entry.second->Get()) {
                uriResolver->EndCacheScope(&cacheData[cacheDataIndex]);
            }
            ++cacheDataIndex;
        }

        for (size_t i = 0, e = _packageResolvers.size(); i != e;
             ++i, ++cacheDataIndex) {
            ArPackageResolver* packageResolver = _packageResolvers[i]->Get();
            if (packageResolver) {
                packageResolver->EndCacheScope(&cacheData[cacheDataIndex]);
            }
        }

        cacheScopeData->Swap(cacheData);
    }

private:
    void _InitializePrimaryResolver(
        const std::vector<_ResolverInfo>& availableResolvers)
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
            resolverTypes = _GetAvailablePrimaryResolvers(availableResolvers);

            TF_DEBUG(AR_RESOLVER_INIT).Msg(
                "ArGetResolver(): Found primary asset resolver types: [%s]\n",
                _GetTypeNames(resolverTypes).c_str());
        }

        std::string debugMsg;

        // resolverTypes should never be empty -- _GetAvailablePrimaryResolvers
        // should always return at least the default resolver. Because of this,
        // if there's more than 2 elements in resolverTypes, there must have 
        // been more than one resolver from an external plugin.
        if (TF_VERIFY(!resolverTypes.empty())) {
            const TfType& resolverType = resolverTypes.front();
            if (resolverTypes.size() > 2) {
                TF_DEBUG(AR_RESOLVER_INIT).Msg(
                    "ArGetResolver(): Found multiple primary asset "
                    "resolvers, using %s\n", 
                    resolverType.GetTypeName().c_str());
            }

            _resolver = _CreateResolver(resolverType, &debugMsg);
            _resolverType = resolverType;
        }

        if (!_resolver) {
            _resolver = _CreateResolver(defaultResolverType, &debugMsg);
            _resolverType = defaultResolverType;
        }

        TF_DEBUG(AR_RESOLVER_INIT).Msg(
            "ArGetResolver(): %s for primary resolver\n",
            debugMsg.c_str());
    }

    void _InitializeURIResolvers(
        const std::vector<_ResolverInfo>& availableResolvers)
    {
        if (TfGetEnvSetting(PXR_AR_DISABLE_PLUGIN_URI_RESOLVERS)) {
            TF_DEBUG(AR_RESOLVER_INIT).Msg(
                "ArGetResolver(): Plugin URI asset resolvers disabled via "
                "PXR_AR_DISABLE_PLUGIN_URI_RESOLVERS.\n");
            return;
        }

        size_t maxSchemeLength = 0;
        std::unordered_map<std::string, _URIResolverSharedPtr> uriResolvers;

        for (const _ResolverInfo& resolverInfo : availableResolvers) {
            TF_DEBUG(AR_RESOLVER_INIT).Msg(
                "ArGetResolver(): Found URI resolver %s\n",
                resolverInfo.type.GetTypeName().c_str());

            std::vector<std::string> uriSchemes;
            uriSchemes.reserve(resolverInfo.uriSchemes.size());

            for (std::string uriScheme : resolverInfo.uriSchemes) {
                // Per RFC 3986 sec 3.1 schemes are case-insensitive.
                // Force all schemes to lower-case to support this.
                uriScheme = TfStringToLower(uriScheme);

                if (const _URIResolverSharedPtr* existingResolver =
                    TfMapLookupPtr(uriResolvers, uriScheme)) {
                    TF_WARN(
                        "ArGetResolver(): %s registered to handle scheme '%s' "
                        "which is already handled by %s. Ignoring.\n",
                        resolverInfo.type.GetTypeName().c_str(),
                        uriScheme.c_str(), 
                        (*existingResolver)->GetType().GetTypeName().c_str());
                }
                else {
                    uriSchemes.push_back(uriScheme);
                }
            }

            if (uriSchemes.empty()) {
                continue;
            }

            // Create resolver. We only want one instance of each resolver
            // type, so make sure we reuse the primary resolver if it has
            // also been registered as handling additional URI schemes.
            _URIResolverSharedPtr uriResolver = std::make_shared<_URIResolver>(
                resolverInfo.plugin, resolverInfo.type, 
                resolverInfo.type == _resolverType ? _resolver : nullptr);
            
            for (std::string& uriScheme : uriSchemes) {
                maxSchemeLength = std::max(uriScheme.length(), maxSchemeLength);
                uriResolvers.emplace(std::move(uriScheme), uriResolver);
            };

            TF_DEBUG(AR_RESOLVER_INIT).Msg(
                "ArGetResolver(): Using %s for URI scheme(s) [\"%s\"]\n",
                resolverInfo.type.GetTypeName().c_str(),
                TfStringJoin(uriSchemes, "\", \"").c_str());
        }

        _uriResolvers = std::move(uriResolvers);
        _maxURISchemeLength = maxSchemeLength;
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
                TF_CODING_ERROR(
                    "Could not find plugin for package resolver %s", 
                    packageResolverType.GetTypeName().c_str());
                continue;
            }

            const JsOptionalValue extensionsVal = JsFindValue(
                plugin->GetMetadataForType(packageResolverType),
                _tokens->extensions.GetString());
            if (!extensionsVal) {
                TF_CODING_ERROR(
                    "No package formats specified in '%s' metadata for '%s'",
                    _tokens->extensions.GetText(), 
                    packageResolverType.GetTypeName().c_str());
                continue;
            }

            std::vector<std::string> extensions;
            if (extensionsVal->IsArrayOf<std::string>()) {
                extensions = extensionsVal->GetArrayOf<std::string>();
            }
            else {
                TF_CODING_ERROR(
                    "'%s' metadata for %s must be a list of strings.",
                    _tokens->extensions.GetText(), 
                    packageResolverType.GetTypeName().c_str());
                continue;
            }

            for (const std::string& extension : extensions) {
                if (extension.empty()) {
                    continue;
                }

                _packageResolvers.push_back(std::make_shared<_PackageResolver>(
                    extension, plugin, packageResolverType));

                TF_DEBUG(AR_RESOLVER_INIT).Msg(
                    "ArGetResolver(): Using package resolver %s for %s "
                    "from plugin %s\n", 
                    packageResolverType.GetTypeName().c_str(),
                    extension.c_str(), plugin->GetName().c_str());
            }
        }
    }

    ArResolver&
    _GetResolver(const std::string& assetPath)
    {
        ArResolver* uriResolver = _GetURIResolver(assetPath);
        return uriResolver ? *uriResolver : *_resolver;
    }

    ArResolver*
    _GetURIResolver(const std::string& assetPath)
    {
        if (_uriResolvers.empty()) {
            return nullptr;
        }

        // Search for the first ":" character delimiting a URI scheme in
        // the given asset path. As an optimization, we only search the
        // first _maxURISchemeLength + 1 (to accommodate the ":") characters.
        if (assetPath.length() < _maxURISchemeLength + 1) {
            return nullptr;
        }

        auto endIt = assetPath.begin() + (_maxURISchemeLength + 1);
        auto delimIt = std::find(assetPath.begin(), endIt, ':');
        if (delimIt == endIt) {
            return nullptr;
        }

        return _GetURIResolverForScheme(
            std::string(assetPath.begin(), delimIt));
    }

    ArResolver*
    _GetURIResolverForScheme(const std::string& scheme)
    {
        // Per RFC 3986 sec 3.1 schemes are case-insensitive. The schemes
        // stored in _uriResolvers are always stored in lower-case, so
        // convert our candidate scheme to lower case as well.
        const _URIResolverSharedPtr* uriResolver = 
            TfMapLookupPtr(_uriResolvers, TfStringToLower(scheme));
        return uriResolver ? (*uriResolver)->Get() : nullptr;
    }

    ArPackageResolver*
    _GetPackageResolver(const std::string& packageRelativePath)
    {
        const std::string innermostPackage = 
            ArSplitPackageRelativePathInner(packageRelativePath).first;
        const std::string format = GetExtension(innermostPackage);

        for (size_t i = 0, e = _packageResolvers.size(); i != e; ++i) {
            if (_packageResolvers[i]->HandlesFormat(format)) {
                return _packageResolvers[i]->Get();
            }
        }

        return nullptr;
    }

    template <class ResolveFn>
    ArResolvedPath
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
                return ArResolvedPath();
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
                    return ArResolvedPath();
                }

                packagePath = packageResolver->Resolve(
                    resolvedPackagePath, packagePath);
                if (packagePath.empty()) {
                    return ArResolvedPath();
                }

                resolvedPackagePath = ArJoinPackageRelativePath(
                    resolvedPackagePath, packagePath);
            }

            return ArResolvedPath(std::move(resolvedPackagePath));
        }

        return resolveFn(path);
    }

    // Primary Resolver --------------------

    TfType _resolverType;
    std::shared_ptr<ArResolver> _resolver;

    // URI Resolvers --------------------

    class _URIResolver
        : public _PluginResolver<ArResolver, Ar_ResolverFactoryBase>
    {
        using Base = _PluginResolver<ArResolver, Ar_ResolverFactoryBase>;

    public:
        _URIResolver(
            const PlugPluginPtr& plugin,
            const TfType& resolverType,
            const std::shared_ptr<ArResolver>& resolver = nullptr)
            : Base(plugin, resolverType, resolver)
        {
        }
    };

    using _URIResolverSharedPtr = std::shared_ptr<_URIResolver>;
    std::unordered_map<std::string, _URIResolverSharedPtr> _uriResolvers;
    size_t _maxURISchemeLength;

    // Package Resolvers --------------------

    class _PackageResolver
        : public _PluginResolver<
            ArPackageResolver, Ar_PackageResolverFactoryBase>
    {
        using Base = _PluginResolver<
            ArPackageResolver, Ar_PackageResolverFactoryBase>;

    public:
        _PackageResolver(
            const std::string& packageFormat,
            const PlugPluginPtr& plugin,
            const TfType& resolverType)
            : Base(plugin, resolverType)
            , _packageFormat(packageFormat)
        { }

        bool HandlesFormat(const std::string& extension) const
        {
            return _packageFormat == extension;
        }

    private:
        std::string _packageFormat;
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


// ------------------------------------------------------------

ArResolver::ArResolver()
{
}

ArResolver::~ArResolver()
{
}

std::string
ArResolver::CreateIdentifier(
    const std::string& assetPath,
    const ArResolvedPath& anchorAssetPath)
{
    return _CreateIdentifier(assetPath, anchorAssetPath);
}

std::string
ArResolver::CreateIdentifierForNewAsset(
    const std::string& assetPath,
    const ArResolvedPath& anchorAssetPath)
{
    return _CreateIdentifierForNewAsset(assetPath, anchorAssetPath);
}

ArResolvedPath
ArResolver::Resolve(
    const std::string& assetPath)
{
    return _Resolve(assetPath);
}

ArResolvedPath
ArResolver::ResolveForNewAsset(
    const std::string& assetPath)
{
    return _ResolveForNewAsset(assetPath);
}

void
ArResolver::BindContext(
    const ArResolverContext& context,
    VtValue* bindingData)
{
    _BindContext(context, bindingData);
}

void
ArResolver::UnbindContext(
    const ArResolverContext& context,
    VtValue* bindingData)
{
    _UnbindContext(context, bindingData);
}

ArResolverContext
ArResolver::CreateDefaultContext()
{
    return _CreateDefaultContext();
}

ArResolverContext
ArResolver::CreateDefaultContextForAsset(
    const std::string& assetPath)
{
    return _CreateDefaultContextForAsset(assetPath);
}

ArResolverContext
ArResolver::CreateContextFromString(
    const std::string& contextStr)
{
    return _CreateContextFromString(contextStr);
}

ArResolverContext
ArResolver::CreateContextFromString(
    const std::string& uriScheme, const std::string& contextStr)
{
    return _GetResolver().CreateContextFromString(uriScheme, contextStr);
}

ArResolverContext
ArResolver::CreateContextFromStrings(
    const std::vector<std::pair<std::string, std::string>>& contextStrs)
{
    return _GetResolver().CreateContextFromStrings(contextStrs);
}

void
ArResolver::RefreshContext(
    const ArResolverContext& context)
{
    _RefreshContext(context);
}

ArResolverContext
ArResolver::GetCurrentContext()
{
    return _GetCurrentContext();
}

std::string
ArResolver::GetExtension(
    const std::string& assetPath)
{
    return _GetExtension(assetPath);
}

ArAssetInfo
ArResolver::GetAssetInfo(
    const std::string& assetPath,
    const ArResolvedPath& resolvedPath)
{
    return _GetAssetInfo(assetPath, resolvedPath);
}

VtValue
ArResolver::GetModificationTimestamp(
    const std::string& assetPath,
    const ArResolvedPath& resolvedPath)
{
    return _GetModificationTimestamp(assetPath, resolvedPath);
}

std::shared_ptr<ArAsset>
ArResolver::OpenAsset(
    const ArResolvedPath& resolvedPath)
{
    return _OpenAsset(resolvedPath);
}

std::shared_ptr<ArWritableAsset>
ArResolver::OpenAssetForWrite(
    const ArResolvedPath& resolvedPath,
    WriteMode mode)
{
    return _OpenAssetForWrite(resolvedPath, mode);
}

bool
ArResolver::IsContextDependentPath(
    const std::string& assetPath)
{
    return _IsContextDependentPath(assetPath);
}

void
ArResolver::BeginCacheScope(
    VtValue* cacheScopeData)
{
    _BeginCacheScope(cacheScopeData);
}

void
ArResolver::EndCacheScope(
    VtValue* cacheScopeData)
{
    _EndCacheScope(cacheScopeData);
}

void
ArResolver::ConfigureResolverForAsset(
    const std::string& path)
{
}

bool
ArResolver::IsSearchPath(
    const std::string& path)
{
    return false;
}

bool
ArResolver::IsRepositoryPath(
    const std::string& path)
{
    return _IsRepositoryPath(path);
}

void
ArResolver::_BindContext(
    const ArResolverContext& context,
    VtValue* bindingData)
{
}

bool
ArResolver::FetchToLocalResolvedPath(
    const std::string& path,
    const std::string& resolvedPath)
{
    return true;
}

bool
ArResolver::CanWriteLayerToPath(
    const std::string& path,
    std::string* whyNot)
{
    return true;
}

bool
ArResolver::CanCreateNewLayerWithIdentifier(
    const std::string& identifier, 
    std::string* whyNot)
{
    return true;
}

void
ArResolver::_UnbindContext(
    const ArResolverContext& context,
    VtValue* bindingData)
{
}

ArResolverContext
ArResolver::_CreateDefaultContext()
{
    return ArResolverContext();
}

ArResolverContext
ArResolver::_CreateDefaultContextForAsset(
    const std::string& assetPath)
{
    return ArResolverContext();
}

ArResolverContext
ArResolver::_CreateContextFromString(
    const std::string& contextStr)
{
    return ArResolverContext();
}

void
ArResolver::_RefreshContext(
    const ArResolverContext& context)
{
}

ArResolverContext
ArResolver::_GetCurrentContext()
{
    return ArResolverContext();
}

ArAssetInfo
ArResolver::_GetAssetInfo(
    const std::string& assetPath,
    const ArResolvedPath& resolvedPath)
{
    return ArAssetInfo();
}

bool
ArResolver::_IsContextDependentPath(
    const std::string& assetPath)
{
    return false;
}

bool
ArResolver::_IsRepositoryPath(
    const std::string& path)
{
    return false;
}

// ------------------------------------------------------------

ArResolver& 
ArGetResolver()
{
    return _GetResolver();
}

ArResolver&
ArGetUnderlyingResolver()
{
    return _GetResolver().GetPrimaryResolver();
}

std::vector<TfType>
ArGetAvailableResolvers()
{
    return _GetAvailablePrimaryResolvers(_GetAvailableResolvers());
}

std::unique_ptr<ArResolver>
ArCreateResolver(const TfType& resolverType)
{
    return _CreateResolver(resolverType);
}

PXR_NAMESPACE_CLOSE_SCOPE
