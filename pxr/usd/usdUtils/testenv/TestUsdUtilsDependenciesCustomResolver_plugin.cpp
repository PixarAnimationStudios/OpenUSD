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

#include "pxr/usd/ar/defaultResolver.h"
#include "pxr/usd/ar/defineResolver.h"
#include "pxr/usd/ar/filesystemAsset.h"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

/// This bare bones resolver is specifically setup to use different URI schemes
/// for identifier creation and asset resolution.
/// Identifiers are in form of test:path
/// Resolved paths are in form testresolved:path
class CustomResolver
    : public ArResolver
{
public:
    CustomResolver()
    {
    }

protected:
    std::string _CreateIdentifier(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) const final
    {
        return assetPath;
    }

    std::string _CreateIdentifierForNewAsset(
        const std::string& assetPath,
        const ArResolvedPath& anchorAssetPath) const final
    {
        return assetPath;
    }

    ArResolvedPath _Resolve(
        const std::string& assetPath) const final
    {
        const std::string resolved = "testresolved:" + 
            assetPath.substr(assetPath.find_first_of(":") + 1);

        return ArResolvedPath(resolved);
    }

    ArResolvedPath _ResolveForNewAsset(
        const std::string& assetPath) const final
    {
        return _Resolve(assetPath);
    }

    std::shared_ptr<ArAsset> _OpenAsset(
        const ArResolvedPath& resolvedPath) const final
    {
        const std::string pathStr = resolvedPath.GetPathString();
        const std::string filesystemPath = 
            pathStr.substr(pathStr.find_first_of(":") + 1);

        return ArFilesystemAsset::Open(ArResolvedPath(filesystemPath));
    }

    std::shared_ptr<ArWritableAsset>
    _OpenAssetForWrite(
        const ArResolvedPath& resolvedPath,
        WriteMode writeMode) const final
    {
        return nullptr;
    }

};

AR_DEFINE_RESOLVER(CustomResolver, ArResolver);
