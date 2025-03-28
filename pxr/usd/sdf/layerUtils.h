//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_LAYER_UTILS_H
#define PXR_USD_SDF_LAYER_UTILS_H

/// \file sdf/layerUtils.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/layer.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);

/// Returns the path to the asset specified by \p assetPath, using the
/// \p anchor layer to anchor the path if it is relative.  If the result of
/// anchoring \p assetPath to \p anchor's path cannot be resolved and
/// \p assetPath is a search path, \p assetPath will be returned.  If
/// \p assetPath is not relative, \p assetPath will be returned.  Otherwise,
/// the anchored path will be returned.
///
/// Note that if \p anchor is an anonymous layer, we will always return
/// the untouched \p assetPath.
SDF_API std::string
SdfComputeAssetPathRelativeToLayer(
    const SdfLayerHandle& anchor,
    const std::string& assetPath);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_LAYER_UTILS_H
