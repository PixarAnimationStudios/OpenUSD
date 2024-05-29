//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/ar/defaultResolver.h"
#include "pxr/usd/ar/defineResolver.h"
#include "pxr/usd/sdf/layer.h"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

// Test resolver used by some Sdf unit tests to verify some conditions:
//
// - The resolver should never be passed an asset path with file format
//   arguments attached. File format arguments are a Sdf-level concern
//   that resolver plugins should not have to reason about.
//

static bool
_AssetPathHasArguments(const std::string& assetPath)
{
    std::string layerPath;
    SdfLayer::FileFormatArguments args;
    return SdfLayer::SplitIdentifier(assetPath, &layerPath, &args)
        && !args.empty();
}

class Sdf_TestResolver
    : public ArDefaultResolver
{
public:

protected:
    using _Parent = ArDefaultResolver;

    std::string _CreateIdentifier(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) const override
    {
        TF_AXIOM(!_AssetPathHasArguments(assetPath));
        return _Parent::_CreateIdentifier(assetPath, anchorAssetPath);
    }

    std::string _CreateIdentifierForNewAsset(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) const override
    {
        TF_AXIOM(!_AssetPathHasArguments(assetPath));
        return _Parent::_CreateIdentifierForNewAsset(
            assetPath, anchorAssetPath);
    }

    ArResolvedPath _Resolve(
        const std::string& assetPath) const override
    {
        TF_AXIOM(!_AssetPathHasArguments(assetPath));
        return _Parent::_Resolve(assetPath);
    }

    ArResolvedPath _ResolveForNewAsset(
        const std::string& assetPath) const override
    {
        TF_AXIOM(!_AssetPathHasArguments(assetPath));
        return _Parent::_ResolveForNewAsset(assetPath);
    }

    bool _IsContextDependentPath(
        const std::string& assetPath) const override
    {
        TF_AXIOM(!_AssetPathHasArguments(assetPath));
        return _Parent::_IsContextDependentPath(assetPath);
    }

    std::string _GetExtension(
        const std::string& assetPath) const override
    {
        TF_AXIOM(!_AssetPathHasArguments(assetPath));
        return _Parent::_GetExtension(assetPath);
    }

    ArAssetInfo _GetAssetInfo(
        const std::string& assetPath,
        const ArResolvedPath& resolvedPath) const override
    {
        TF_AXIOM(!_AssetPathHasArguments(assetPath));
        return _Parent::_GetAssetInfo(assetPath, resolvedPath);
    }

    ArTimestamp _GetModificationTimestamp(
        const std::string& assetPath,
        const ArResolvedPath& resolvedPath) const override
    {
        TF_AXIOM(!_AssetPathHasArguments(assetPath));
        return _Parent::_GetModificationTimestamp(assetPath, resolvedPath);
    }

};

PXR_NAMESPACE_OPEN_SCOPE
AR_DEFINE_RESOLVER(Sdf_TestResolver, ArDefaultResolver);
PXR_NAMESPACE_CLOSE_SCOPE
