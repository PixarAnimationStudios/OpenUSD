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
