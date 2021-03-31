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
//
#include "pxr/pxr.h"

#include "TestArURIResolver_plugin.h"

#include "pxr/usd/ar/defaultResolver.h"
#include "pxr/usd/ar/defineResolver.h"
#include "pxr/usd/ar/defineResolverContext.h"

#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include <tbb/enumerable_thread_specific.h>

#include <map>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

// Base class for test URI resolvers
class _TestURIResolverBase
    : public ArResolver
{
public:
    virtual void ConfigureResolverForAsset(
        const std::string& path) final
    {
    }

    virtual std::string AnchorRelativePath(
        const std::string& anchorPath, 
        const std::string& path) final
    {
        TF_AXIOM(TfStringStartsWith(TfStringToLower(anchorPath), _uriScheme));
        TF_AXIOM(!TfStringStartsWith(TfStringToLower(path), _uriScheme));
        return TfStringCatPaths(anchorPath, path);
    }

    virtual bool IsRelativePath(
        const std::string& path) final
    {
        TF_AXIOM(!TfStringStartsWith(TfStringToLower(path), _uriScheme));
        return true;
    }

    virtual std::string _GetExtension(
        const std::string& path) final
    {
        TF_AXIOM(TfStringStartsWith(TfStringToLower(path), _uriScheme));
        return TfGetExtension(path);
    }

    virtual std::string _CreateIdentifier(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) final
    {
        TF_AXIOM(
            TfStringStartsWith(TfStringToLower(assetPath), _uriScheme) ||
            TfStringStartsWith(TfStringToLower(anchorAssetPath), _uriScheme));
        return assetPath;
    }

    virtual std::string _CreateIdentifierForNewAsset(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) final
    {
        TF_AXIOM(
            TfStringStartsWith(TfStringToLower(assetPath), _uriScheme) ||
            TfStringStartsWith(TfStringToLower(anchorAssetPath), _uriScheme));
        return assetPath;
    }

    virtual ArResolvedPath _Resolve(
        const std::string& assetPath) final
    {
        TF_AXIOM(TfStringStartsWith(TfStringToLower(assetPath), _uriScheme));

        const _TestURIResolverContext* uriContext = _GetCurrentContextPtr();
        if (uriContext && !uriContext->data.empty()) {
            return ArResolvedPath(assetPath + "?" + uriContext->data);
        }

        return ArResolvedPath(assetPath);
    }

    virtual ArResolvedPath _ResolveForNewAsset(
        const std::string& assetPath) final
    {
        return _Resolve(assetPath);
    }

    virtual void _BindContext(
        const ArResolverContext& context,
        VtValue* bindingData) final
    {
        _ContextStack& contextStack = _threadContextStack.local();
        contextStack.push_back(context.Get<_TestURIResolverContext>());
    }

    virtual void _UnbindContext(
        const ArResolverContext& context,
        VtValue* bindingData) final
    {
        _ContextStack& contextStack = _threadContextStack.local();
        TF_AXIOM(!contextStack.empty());
        contextStack.pop_back();
    }

    virtual ArResolverContext _CreateDefaultContext() final
    {
        return ArResolverContext(_TestURIResolverContext());
    }

    virtual ArResolverContext _CreateDefaultContextForAsset(
        const std::string& filePath) final
    {
        TF_AXIOM(TfStringStartsWith(TfStringToLower(filePath), _uriScheme));
        return ArResolverContext(_TestURIResolverContext());
    }

    virtual ArResolverContext _GetCurrentContext() final
    {
        const _TestURIResolverContext* uriContext = _GetCurrentContextPtr();
        if (uriContext) {
            return ArResolverContext(*uriContext);
        }
        return ArResolverContext();
    }

    virtual VtValue _GetModificationTimestamp(
        const std::string& path,
        const ArResolvedPath& resolvedPath) final
    {
        TF_AXIOM(TfStringStartsWith(TfStringToLower(path), _uriScheme));
        return VtValue(42);
    }

    virtual std::shared_ptr<ArAsset> _OpenAsset(
        const ArResolvedPath& resolvedPath)
    {
        TF_AXIOM(TfStringStartsWith(TfStringToLower(resolvedPath), _uriScheme));
        return nullptr;
    }

    virtual bool CreatePathForLayer(
        const std::string& path) final
    {
        TF_AXIOM(TfStringStartsWith(TfStringToLower(path), _uriScheme));
        return false;
    }

    virtual void _BeginCacheScope(
        VtValue* cacheScopeData) final
    {
    }

    virtual void _EndCacheScope(
        VtValue* cacheScopeData) final
    {
    }

protected:
    _TestURIResolverBase(const std::string& uriScheme)
        : _uriScheme(uriScheme + ":")
    {
    }

    virtual ArResolverContext _CreateContextFromString(
        const std::string& contextStr) override
    {
        return ArResolverContext(_TestURIResolverContext(contextStr));
    };

    virtual std::shared_ptr<ArWritableAsset>
    _OpenAssetForWrite(
        const ArResolvedPath& resolvedPath,
        WriteMode writeMode) override
    {
        TF_AXIOM(TfStringStartsWith(TfStringToLower(resolvedPath), _uriScheme));
        return nullptr;
    }

private:
    const _TestURIResolverContext* _GetCurrentContextPtr()
    {
        const _ContextStack& contextStack = _threadContextStack.local();
        return contextStack.empty() ? nullptr : contextStack.back();
    }

    std::string _uriScheme;

    using _ContextStack = std::vector<const _TestURIResolverContext*>;
    using _PerThreadContextStack = 
        tbb::enumerable_thread_specific<_ContextStack>;
    _PerThreadContextStack _threadContextStack;
};

// Test resolver that handles asset paths of the form "test://...."
class _TestURIResolver
    : public _TestURIResolverBase
{
public:
    _TestURIResolver()
        : _TestURIResolverBase("test")
    {
    }
};

// Test resolver that handles asset paths of the form "test_other://...."
class _TestOtherURIResolver
    : public _TestURIResolverBase
{
public:
    _TestOtherURIResolver()
        : _TestURIResolverBase("test_other")
    {
    }
};

// XXX: Should have a AR_DEFINE_ABSTRACT_RESOLVER macro like
// AR_DEFINE_ABSTRACT_RESOLVER(_TestURIResolverBase, ArResolver)
// to take care of this registration.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<_TestURIResolverBase, TfType::Bases<ArResolver>>();
}

AR_DEFINE_RESOLVER(_TestURIResolver, _TestURIResolverBase);
AR_DEFINE_RESOLVER(_TestOtherURIResolver, _TestURIResolverBase);
