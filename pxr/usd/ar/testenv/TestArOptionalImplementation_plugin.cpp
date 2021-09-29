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
