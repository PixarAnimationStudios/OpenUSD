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
#ifndef SDF_LAYER_UTILS_H
#define SDF_LAYER_UTILS_H

/// \file sdf/layerUtils.h

#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/api.h"
#include <string>

TF_DECLARE_REF_PTRS(SdfLayer);
SDF_DECLARE_HANDLES(SdfLayer);

/// Returns the path to the asset specified by \p assetPath, using the
/// \p anchor layer to anchor the path if it is relative.
/// If that path cannot be resolved and \p layerPath is a search path,
/// \p layerPath will be returned.  If \p layerPath is not relative,
/// \p layerPath will be returned.  Otherwise, the anchored path will
/// be returned.
SDF_API std::string
SdfComputeAssetPathRelativeToLayer(
    const SdfLayerHandle& anchor,
    const std::string& assetPath);

/// Returns a layer with the given \p layerPath relative to the \p anchor 
/// layer.  This function uses \ref SdfComputeAssetPathRelativeToLayer with
/// \p anchor and \p layerPath to compute the layer path to find or open.
/// See documentation on that function for more details.
///
/// If the \p anchor layer is invalid, the \p layerPath pointer is invalid, or
/// \p layerPath contains an empty string, a coding error is raised and a null
/// layer is returned.
SDF_API SdfLayerRefPtr 
SdfFindOrOpenRelativeToLayer(
    const SdfLayerHandle& anchor,
    std::string* layerPath,
    const SdfLayer::FileFormatArguments& args 
        = SdfLayer::FileFormatArguments());

#endif // SDF_LAYER_UTILS_H
