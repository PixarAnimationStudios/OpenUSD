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

// Test resolver that handles asset paths of the form "test://...."
class _TestURIResolver
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
        TF_AXIOM(TfStringStartsWith(TfStringToLower(anchorPath), "test:"));
        TF_AXIOM(!TfStringStartsWith(TfStringToLower(path), "test:"));
        return TfStringCatPaths(anchorPath, path);
    }

    virtual bool IsRelativePath(
        const std::string& path) final
    {
        TF_AXIOM(!TfStringStartsWith(TfStringToLower(path), "test:"));
        return true;
    }

    virtual std::string _GetExtension(
        const std::string& path) final
    {
        TF_AXIOM(TfStringStartsWith(TfStringToLower(path), "test:"));
        return TfGetExtension(path);
    }

    virtual std::string _CreateIdentifier(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) final
    {
        TF_AXIOM(
            TfStringStartsWith(TfStringToLower(assetPath), "test:") ||
            TfStringStartsWith(TfStringToLower(anchorAssetPath), "test:"));
        return assetPath;
    }

    virtual std::string _CreateIdentifierForNewAsset(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) final
    {
        TF_AXIOM(
            TfStringStartsWith(TfStringToLower(assetPath), "test:") ||
            TfStringStartsWith(TfStringToLower(anchorAssetPath), "test:"));
        return assetPath;
    }

    virtual ArResolvedPath _Resolve(
        const std::string& assetPath) final
    {
        TF_AXIOM(TfStringStartsWith(TfStringToLower(assetPath), "test:"));

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
        TF_AXIOM(TfStringStartsWith(TfStringToLower(filePath), "test:"));
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
        TF_AXIOM(TfStringStartsWith(TfStringToLower(path), "test:"));
        return VtValue(42);
    }

    virtual std::shared_ptr<ArAsset> _OpenAsset(
        const ArResolvedPath& resolvedPath)
    {
        TF_AXIOM(TfStringStartsWith(TfStringToLower(resolvedPath), "test:"));
        return nullptr;
    }

    virtual bool CreatePathForLayer(
        const std::string& path) final
    {
        TF_AXIOM(TfStringStartsWith(TfStringToLower(path), "test:"));
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
        TF_AXIOM(TfStringStartsWith(TfStringToLower(resolvedPath), "test:"));
        return nullptr;
    }

private:
    const _TestURIResolverContext* _GetCurrentContextPtr()
    {
        const _ContextStack& contextStack = _threadContextStack.local();
        return contextStack.empty() ? nullptr : contextStack.back();
    }

    using _ContextStack = std::vector<const _TestURIResolverContext*>;
    using _PerThreadContextStack = 
        tbb::enumerable_thread_specific<_ContextStack>;
    _PerThreadContextStack _threadContextStack;
};

AR_DEFINE_RESOLVER(_TestURIResolver, ArResolver);
