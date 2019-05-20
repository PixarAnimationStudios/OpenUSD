//
// Copyright 2016 Pixar
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
///
/// \file Sdf/LayerUtils.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/layerUtils.h"
#include "pxr/usd/sdf/assetPathResolver.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/layer.h"

#include "pxr/usd/ar/packageUtils.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/trace/trace.h"

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

// Anchor the given relativePath to the same path as the layer 
// specified by anchorLayerPath.
string
_AnchorRelativePath(const string& anchorLayerPath, const string& relativePath)
{
    const string anchorPath = TfGetPathName(anchorLayerPath);
    return anchorPath.empty() ? 
        relativePath : TfStringCatPaths(anchorPath, relativePath);
}

// Expand a (package path, packaged path) pair until the packaged path is
// a non-package layer that is the root layer of the package layer specified
// by the package path.
std::pair<string, string>
_ExpandPackagePath(const std::pair<string, string>& packageRelativePath)
{
    std::pair<string, string> result = packageRelativePath;
    while (1) {
        if (result.second.empty()) {
            break;
        }

        SdfFileFormatConstPtr packagedFormat = 
            SdfFileFormat::FindByExtension(result.second);
        if (!packagedFormat || !packagedFormat->IsPackage()) {
            break;
        }

        result.first = ArJoinPackageRelativePath(result.first, result.second);
        result.second = packagedFormat->GetPackageRootLayerPath(result.first);
    }
    return result;
}

} // end anonymous namespace

string
SdfComputeAssetPathRelativeToLayer(
    const SdfLayerHandle& anchor,
    const string& assetPath)
{
    if (!anchor) {
        TF_CODING_ERROR("Invalid anchor layer");
        return string();
    }

    if (assetPath.empty()) {
        TF_CODING_ERROR("Layer path is empty");
        return string();
    }

    TRACE_FUNCTION();

    ArResolver& resolver = ArGetResolver();

    // Relative asset paths have special behavior when anchoring to a
    // package or packaged layer: 
    // 
    // - Anchored relative paths (e.g., "./foo/bar.sdf") are always anchored
    //   to the packaged layer in which they are authored. For example, if the
    //   above were authored in the following layers:
    //       "test.package[inner.sdf]" ->  "test.package[foo/bar.sdf]"
    //       "test.package[sub/inner.sdf]" -> "test.package[sub/foo/bar.sdf]"
    //       "test.package" -> "/tmp/test.package[foo/bar.sdf]"
    //
    //   The last case depends on the path of the root layer in the package.
    //   If the package root layer were "inner.sdf", anchoring would give the
    //   same result as the first case; if it were "sub/inner.sdf", it would
    //   give the same result as the second case.
    //
    // - Search relative paths (e.g., "foo/bar.sdf") are first anchored to the
    //   packaged layer in which they are authored. If that does not resolve
    //   to a valid file, the path is then anchored to the package's root
    //   layer. If that does not resolve the path is not anchored and is 
    //   resolved as-is.
    // 
    if (Sdf_IsPackageOrPackagedLayer(anchor) && TfIsRelativePath(assetPath)) {
        // XXX: The use of repository path or real path is the same as in
        // SdfLayer::ComputeAbsolutePath. This logic might want to move
        // somewhere common.
        const string anchorPackagePath = anchor->GetRepositoryPath().empty() ? 
            anchor->GetRealPath() : anchor->GetRepositoryPath();

        // Split the anchoring layer's identifier, since we anchor the asset
        // path against the innermost packaged path. If the anchor layer
        // is a package, anchor against its root layer, which may be
        // nested in another package layer.
        std::pair<string, string> packagePath;
        if (anchor->GetFileFormat()->IsPackage()) {
            packagePath.first = anchorPackagePath;
            packagePath.second = anchor->GetFileFormat()->
                GetPackageRootLayerPath(anchor->GetRealPath());

            packagePath = _ExpandPackagePath(packagePath);
        }
        else {
            packagePath = ArSplitPackageRelativePathInner(anchorPackagePath);
        }

        const string normAssetPath = TfNormPath(assetPath);
        packagePath.second = _AnchorRelativePath(
            packagePath.second, normAssetPath);

        string finalLayerPath = ArJoinPackageRelativePath(packagePath);

        // If assetPath is not a search-relative path, we're done. Otherwise,
        // we need to search in the locations described above.
        const bool isSearchRelativePath = assetPath.front() != '.';
        if (!isSearchRelativePath) {
            return finalLayerPath;
        }

        // If anchoring the asset path to the anchor layer resolves to a
        // valid layer, we're done.
        if (!resolver.Resolve(finalLayerPath).empty()) {
            return finalLayerPath;
        }

        // Try anchoring the layer to the owning package's root layer
        // (which may be nested in another package layer). If this resolves
        // to a valid layer, we're done.
        SdfFileFormatConstPtr packageFormat = 
            SdfFileFormat::FindByExtension(packagePath.first);
        if (packageFormat && packageFormat->IsPackage()) {
            packagePath.second = 
                packageFormat->GetPackageRootLayerPath(packagePath.first);
            packagePath = _ExpandPackagePath(packagePath);

            packagePath.second = _AnchorRelativePath(
                packagePath.second, normAssetPath);
        }
        else {
            packagePath.second = normAssetPath;
        }

        finalLayerPath = ArJoinPackageRelativePath(packagePath);
        if (!resolver.Resolve(finalLayerPath).empty()) {
            return finalLayerPath;
        }

        // If we were unable to resolve this search-relative path within
        // the package, fall through to normal path resolution.
    }

    // Relative paths are resolved using the look-here-first scheme, in
    // which we first look relative to the layer, then fall back to search
    // path resolution.
    string finalLayerPath = anchor->ComputeAbsolutePath(assetPath);
    if (!SdfLayer::IsAnonymousLayerIdentifier(finalLayerPath)) {
        if (resolver.IsSearchPath(assetPath) &&
            resolver.Resolve(finalLayerPath).empty())
            return assetPath;
    }
    
    return finalLayerPath;
}

SdfLayerRefPtr
SdfFindOrOpenRelativeToLayer(
    const SdfLayerHandle& anchor,
    string* layerPath,
    const SdfLayer::FileFormatArguments& args)
{
    if (!anchor) {
        TF_CODING_ERROR("Invalid anchor layer");
        return TfNullPtr;
    }

    if (!layerPath) {
        TF_CODING_ERROR("Invalid layer path pointer");
        return TfNullPtr;
    }

    if (layerPath->empty()) {
        TF_CODING_ERROR("Layer path is empty");
        return TfNullPtr;
    }

    TRACE_FUNCTION();

    *layerPath = SdfComputeAssetPathRelativeToLayer(anchor, *layerPath);
    return SdfLayer::FindOrOpen(*layerPath, args);
}

PXR_NAMESPACE_CLOSE_SCOPE
