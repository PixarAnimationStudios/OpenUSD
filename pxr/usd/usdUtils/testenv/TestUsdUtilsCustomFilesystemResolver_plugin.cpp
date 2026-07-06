//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/base/tf/pathUtils.h"
#include "pxr/usd/ar/defaultResolver.h"
#include "pxr/usd/ar/defineResolver.h"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

// This test resolver is meant for testing purposes only.  Identifiers that
// begin with '///' have the prefix stripped and are made absolute. In all other
// cases this resolver defers to ArDefaultResolver.
class CustomFilesystemResolver
    : public ArDefaultResolver
{
    protected:
        ArResolvedPath 
        _Resolve(
            const std::string& assetPath) const override
        {
            return ArDefaultResolver::_Resolve(_RemovePrefix(assetPath));
        }

        ArResolvedPath
        _ResolveForNewAsset(
            const std::string& assetPath) const override
        {
            return ArDefaultResolver::_ResolveForNewAsset(
                _RemovePrefix(assetPath));
        }

        std::string _CreateIdentifier(
            const std::string& assetPath,
            const ArResolvedPath& anchorAssetPath) const override
        {
            return _IsCustomPath(assetPath) ? assetPath : 
                ArDefaultResolver::_CreateIdentifier(
                    assetPath, anchorAssetPath);
        }

        bool
        _IsContextDependentPath(
            const std::string& assetPath) const override
        {
            return _IsCustomPath(assetPath) ? 
                true : 
                ArDefaultResolver::_IsContextDependentPath(assetPath);
        }

private:
    bool 
    _IsCustomPath(
        const std::string& path) const
    {
        return path.find("///") == 0;
    }
    
    std::string
    _RemovePrefix(
        const std::string& path) const
    {
        return _IsCustomPath(path) ? path.substr(3) : path;
    }
};

PXR_NAMESPACE_OPEN_SCOPE
AR_DEFINE_RESOLVER(CustomFilesystemResolver, ArDefaultResolver);
PXR_NAMESPACE_CLOSE_SCOPE
