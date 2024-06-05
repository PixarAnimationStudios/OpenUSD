//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/ar/defineResolver.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverContext.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_USING_DIRECTIVE

class _TestResolver
    : public ArResolver
{
protected:
    void _PrintFunctionName(const std::string& func) const
    {
        printf("%s\n", func.c_str());
    }

    void _BindContext(
        const ArResolverContext& context,
        VtValue* bindingData) final
    {
        _PrintFunctionName("_BindContext");
    }
    
    void _UnbindContext(
        const ArResolverContext& context,
        VtValue* bindingData) final
    {
        _PrintFunctionName("_UnbindContext");
    }

    ArResolverContext _CreateDefaultContext() const final
    {
        _PrintFunctionName("_CreateDefaultContext");
        return ArResolverContext();
    }

    ArResolverContext _CreateDefaultContextForAsset(
        const std::string& assetPath) const final
    {
        _PrintFunctionName("_CreateDefaultContextForAsset");
        return ArResolverContext();
    };

    ArResolverContext _CreateContextFromString(
        const std::string& contextStr) const final
    {
        _PrintFunctionName("_CreateContextFromString");
        return ArResolverContext();
    };

    void _RefreshContext(
        const ArResolverContext& context) final
    {
        _PrintFunctionName("_RefreshContext");
    };

    ArResolverContext _GetCurrentContext() const final
    {
        _PrintFunctionName("_GetCurrentContext");
        return ArResolverContext();
    };

    bool _IsContextDependentPath(
        const std::string& assetPath) const final
    {
        _PrintFunctionName("_IsContextDependentPath");
        return false;
    };

    void _BeginCacheScope(
        VtValue* cacheScopeData) final 
    { 
        _PrintFunctionName("_BeginCacheScope");
    }

    void _EndCacheScope(
        VtValue* cacheScopeData) final 
    { 
        _PrintFunctionName("_EndCacheScope");
    }

    // Dummy implementations of pure virtuals that aren't called in this
    // test.
    std::string _CreateIdentifier(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) const final { return std::string(); }
    std::string _CreateIdentifierForNewAsset(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) const final { return std::string(); }
    ArResolvedPath _Resolve(
        const std::string& assetPath) const final { return ArResolvedPath(); }
    ArResolvedPath _ResolveForNewAsset(
        const std::string& assetPath) const final { return ArResolvedPath(); }
    std::shared_ptr<ArAsset> _OpenAsset(
        const ArResolvedPath& resolvedPath) const final { return nullptr; }
    std::shared_ptr<ArWritableAsset> _OpenAssetForWrite(
        const ArResolvedPath& resolvedPath,
        WriteMode writeMode) const final { return nullptr; }
};

AR_DEFINE_RESOLVER(_TestResolver, ArResolver);

class _TestResolverWithContextMethods
    : public _TestResolver
{
};

AR_DEFINE_RESOLVER(_TestResolverWithContextMethods, _TestResolver);

class _TestDerivedResolverWithContextMethods
    : public _TestResolverWithContextMethods
{
};

AR_DEFINE_RESOLVER(
    _TestDerivedResolverWithContextMethods,
    _TestResolverWithContextMethods);

class _TestResolverWithCacheMethods
    : public _TestResolver
{
};

AR_DEFINE_RESOLVER(_TestResolverWithCacheMethods, _TestResolver);

class _TestDerivedResolverWithCacheMethods
    : public _TestResolverWithCacheMethods
{
};

AR_DEFINE_RESOLVER(
    _TestDerivedResolverWithCacheMethods,
    _TestResolverWithCacheMethods);
