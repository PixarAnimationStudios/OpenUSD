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
#include "pxr/usd/sdf/layer.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/base/trace/trace.h"

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

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
