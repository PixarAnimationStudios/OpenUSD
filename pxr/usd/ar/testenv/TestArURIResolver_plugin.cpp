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

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/value.h"

#include <map>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

// Base class for test URI resolvers
class _TestURIResolverBase
    : public ArResolver
{
protected:
    _TestURIResolverBase(const std::string& uriScheme)
        : _uriScheme(uriScheme + ":")
    {
    }

    std::string _CreateIdentifier(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) const final
    {
        TF_AXIOM(
            TfStringStartsWith(TfStringToLower(assetPath), _uriScheme) ||
            TfStringStartsWith(TfStringToLower(anchorAssetPath), _uriScheme));
        return assetPath;
    }

    std::string _CreateIdentifierForNewAsset(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) const final
    {
        TF_AXIOM(
            TfStringStartsWith(TfStringToLower(assetPath), _uriScheme) ||
            TfStringStartsWith(TfStringToLower(anchorAssetPath), _uriScheme));
        return assetPath;
    }

    ArResolvedPath _Resolve(
        const std::string& assetPath) const final
    {
        TF_AXIOM(TfStringStartsWith(TfStringToLower(assetPath), _uriScheme));

        const _TestURIResolverContext* uriContext = _GetCurrentContextPtr();
        if (uriContext && !uriContext->data.empty()) {
            return ArResolvedPath(assetPath + "?" + uriContext->data);
        }

        return ArResolvedPath(assetPath);
    }

    ArResolvedPath _ResolveForNewAsset(
        const std::string& assetPath) const final
    {
        return _Resolve(assetPath);
    }

    ArResolverContext _CreateDefaultContext() const final
    {
        return ArResolverContext(
            _TestURIResolverContext("CreateDefaultContext"));
    }

    ArResolverContext _CreateDefaultContextForAsset(
        const std::string& assetPath) const final
    {
        return ArResolverContext(
            _TestURIResolverContext(TfAbsPath(assetPath)));
    }

    std::shared_ptr<ArAsset> _OpenAsset(
        const ArResolvedPath& resolvedPath) const final
    {
        TF_AXIOM(TfStringStartsWith(TfStringToLower(resolvedPath), _uriScheme));
        return nullptr;
    }

    ArResolverContext _CreateContextFromString(
        const std::string& contextStr) const final
    {
        return ArResolverContext(_TestURIResolverContext(contextStr));
    };

    std::shared_ptr<ArWritableAsset>
    _OpenAssetForWrite(
        const ArResolvedPath& resolvedPath,
        WriteMode writeMode) const final
    {
        TF_AXIOM(TfStringStartsWith(TfStringToLower(resolvedPath), _uriScheme));
        return nullptr;
    }

private:
    const _TestURIResolverContext* _GetCurrentContextPtr() const
    {
        return _GetCurrentContextObject<_TestURIResolverContext>();
    }

    std::string _uriScheme;
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

// Test resolver that handles asset paths of the form "test-other://...."
class _TestOtherURIResolver
    : public _TestURIResolverBase
{
public:
    _TestOtherURIResolver()
        : _TestURIResolverBase("test-other")
    {
    }
};

// Underbar characters should cause a failure to register under strict mode
class _TestInvalidUnderbarURIResolver
    : public _TestURIResolverBase
{
public:
    _TestInvalidUnderbarURIResolver()
        : _TestURIResolverBase("test_other")
    {
    }
};

// A colon in the scheme could cause problems when parsing an asset path.
// This should cause a failure to register under strict mode.
class _TestInvalidColonURIResolver
    : public _TestURIResolverBase
{
public:
    _TestInvalidColonURIResolver()
        : _TestURIResolverBase("other:test")
    {
    }
};

// UTF-8 characters should cause a failure to register under strict mode
class _TestInvalidNonAsciiURIResolver
    : public _TestURIResolverBase
{
public:
    _TestInvalidNonAsciiURIResolver()
        : _TestURIResolverBase("test-π-utf8")
    {
    }
};

// Schemes starting with numeric characters should cause a failure to
// register under strict mode
class _TestInvalidNumericPrefixResolver
    : public _TestURIResolverBase
{
public:
    _TestInvalidNumericPrefixResolver()
        : _TestURIResolverBase("113-test")
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
AR_DEFINE_RESOLVER(_TestInvalidUnderbarURIResolver, _TestURIResolverBase);
AR_DEFINE_RESOLVER(_TestInvalidColonURIResolver, _TestURIResolverBase);
AR_DEFINE_RESOLVER(_TestInvalidNonAsciiURIResolver, _TestURIResolverBase);
AR_DEFINE_RESOLVER(_TestInvalidNumericPrefixResolver, _TestURIResolverBase);
