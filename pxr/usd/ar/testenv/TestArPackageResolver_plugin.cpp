//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/ar/definePackageResolver.h"
#include "pxr/usd/ar/packageResolver.h"

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_USING_DIRECTIVE

// Test package resolver that handles packages of the form
// "foo.package[...]"
class _TestPackageResolver
    : public ArPackageResolver
{
public:
    virtual std::string Resolve(
        const std::string& resolvedPackagePath,
        const std::string& packagedPath) override
    {
        TF_AXIOM(TfStringEndsWith(resolvedPackagePath, ".package"));
        return packagedPath;
    }

    virtual std::shared_ptr<ArAsset> OpenAsset(
        const std::string& resolvedPackagePath,
        const std::string& resolvedPackagedPath) override
    {
        return nullptr;
    }

    virtual void BeginCacheScope(
        VtValue* cacheScopeData) override
    {
    }

    virtual void EndCacheScope(
        VtValue* cacheScopeData) override
    {
    }

};

AR_DEFINE_PACKAGE_RESOLVER(_TestPackageResolver, ArPackageResolver);
