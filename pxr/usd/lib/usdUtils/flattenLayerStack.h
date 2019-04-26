//
// Copyright 2017 Pixar
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
#ifndef USDUTILS_FLATTEN_LAYER_STACK_H_
#define USDUTILS_FLATTEN_LAYER_STACK_H_

/// \file usdUtils/flattenLayerStack.h 
///
/// Utilities for flattening layer stacks into a single layer.

#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdUtils/api.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);

/// Flatten the root layer stack of the given \p stage into a single layer
/// with the given optional \p tag.
///
/// The result layer can be substituted for the original layer stack
/// while producing the same composed UsdStage.
///
/// Unlike UsdStage::Export(), this function does not flatten
/// composition arcs, such as references, payloads, inherits,
/// specializes, or variants.
///
/// Sublayer time offsets on the sublayers will be applied to remap
/// any time-keyed scene description, such as timeSamples and clips.
///
/// Asset paths will be resolved to absolute form, to ensure that
/// they continue to identify the same asset from the output layer.
/// \sa UsdUtilsFlattenLayerStackResolveAssetPath
///
/// A few historical scene description features cannot be flattened
/// into a single opinion because they unfortunately encode
/// operations that are not closed under composition.  Specifically,
/// the SdfListOp operations "add" and "reorder" cannot be flattened.
/// Instead, "add" will be converted to "append", and "reorder"
/// will be discarded.
///
USDUTILS_API
SdfLayerRefPtr
UsdUtilsFlattenLayerStack(const UsdStagePtr &stage,
                          const std::string& tag = std::string());

/// Callback function for overloaded version of \c UsdUtilsFlattenLayerStack.
///
/// The callback is given the \c sourceLayer and the \c assetPath authored in
/// that layer.  It should return the \c std::string that should be authored in
/// the flattened layer.
///
/// \sa UsdUtilsFlattenLayerStackResolveAssetPath
using UsdUtilsResolveAssetPathFn = std::function<std::string(
        const SdfLayerHandle& sourceLayer, 
        const std::string& assetPath)>;

/// Flatten the root layer stack of the given \p stage into a single layer with
/// the given optional \p tag and using the \p resolveAssetPathFn to resolve
/// asset paths that are encountered.  
///
/// This is an advanced version of the above function.  
///
///
/// One use case for this version of the function is to flatten a layer stack
/// that contains relative asset paths that we want to preserve as relative
/// paths.  For example:
///
/// \code
/// /source/root.usd # sublayers a.usd and b.usd
/// /source/a.usd    # contains reference to ./subdir/layer.usd
/// /source/b.usd
/// /source/subdir/layer.usd
/// \endcode
///
/// We may want to generate \c "/dest/root.flat.usd" knowing that we will
/// (by some other means) also be copying \c "/source/subdir" into \c
/// "/dest/subdir".  It's useful then to preserve the relative paths.
///
/// Note, only the caller knows the ultimate destination of the flattened layer.
/// So to accomplish this, we can provide a \c resolveAssetPathFn callback that
/// captures the outputDir, tests if the authored path is relative, and if so,
/// computes a new relative path (based on where it will eventually be
/// exported).
USDUTILS_API
SdfLayerRefPtr
UsdUtilsFlattenLayerStack(const UsdStagePtr &stage,
                          const UsdUtilsResolveAssetPathFn& resolveAssetPathFn,
                          const std::string& tag = std::string());

/// The default \c UsdUtilsResolvePathFn used by \c UsdUtilsFlattenLayerStack.
/// For paths that the current ArResolver identifies as searchpaths or absolute
/// paths, we return the unmodified path. However, any "Layer relative path"
/// (see SdfComputeAssetPathRelativeToLayer) will be absolutized, because we do
/// not know if the flattened layer's containing directory will be the same as
/// any given source layer's in the incoming layerStack.
USDUTILS_API
std::string
UsdUtilsFlattenLayerStackResolveAssetPath(
    const SdfLayerHandle& sourceLayer,
    const std::string& assetPath);


PXR_NAMESPACE_CLOSE_SCOPE

#endif /* USDUTILS_FLATTEN_LAYER_STACK_H_ */
