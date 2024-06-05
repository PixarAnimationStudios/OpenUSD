//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_FLATTEN_UTILS_H
#define PXR_USD_USD_FLATTEN_UTILS_H

/// \file usd/flattenUtils.h 
///
/// Utilities for flattening layer stacks into a single layer.

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);

/// Flatten \p layerStack into a single layer with the given optional \p tag.
///
/// A composed UsdStage created from this flattened layer will be the same
/// as a composed UsdStage whose root layer stack is the original layer stack.
///
/// Unlike UsdStage::Flatten(), this function does not flatten
/// composition arcs, such as references, payloads, inherits,
/// specializes, or variants.
///
/// Sublayer time offsets on the sublayers will be applied to remap
/// any time-keyed scene description, such as timeSamples and clips.
///
/// Asset paths will be resolved to absolute form, to ensure that
/// they continue to identify the same asset from the output layer.
///
/// Asset paths containing stage variable expressions will be evaluated
/// using the variables from the root and session layer of \p layerStack
/// before being resolved. \sa UsdFlattenLayerStackResolveAssetPath
///
/// A few historical scene description features cannot be flattened
/// into a single opinion because they unfortunately encode
/// operations that are not closed under composition.  Specifically,
/// the SdfListOp operations "add" and "reorder" cannot be flattened.
/// Instead, "add" will be converted to "append", and "reorder"
/// will be discarded.
///
USD_API
SdfLayerRefPtr
UsdFlattenLayerStack(const PcpLayerStackRefPtr &layerStack,
                     const std::string& tag = std::string());

/// Callback function for overloaded version of \c UsdFlattenLayerStack.
///
/// The callback is given the \c sourceLayer and the \c assetPath authored in
/// that layer.  It should return the \c std::string that should be authored in
/// the flattened layer.
///
/// \sa UsdFlattenLayerStackResolveAssetPath
using UsdFlattenResolveAssetPathFn = std::function<std::string(
    const SdfLayerHandle& sourceLayer, 
    const std::string& assetPath)>;

/// Flatten the \p layerStack into a single layer with the given optional \p tag
/// and using the \p resolveAssetPathFn to resolve asset paths that are
/// encountered.
///
/// This is an advanced version of the above function.  
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
///
/// Asset paths containing stage variable expressions will be evaluated using
/// the variables from the root and session layer of \p layerStack. The
/// evaluated asset path will be passed to the \p resolveAssetPathFn callback
/// instead of the original asset path.
/// \sa UsdFlattenLayerStackResolveAssetPath.
USD_API
SdfLayerRefPtr
UsdFlattenLayerStack(const PcpLayerStackRefPtr &layerStack,
                     const UsdFlattenResolveAssetPathFn& resolveAssetPathFn,
                     const std::string& tag = std::string());

/// Implements the default asset path flattening behavior for
/// \c UsdFlattenLayerStack. \p assetPath will be anchored to \p sourceLayer
/// by calling SdfComputeAssetPathRelativeToLayer. This function assumes
/// that \p assetPath does not contain a stage variable expression.
USD_API
std::string
UsdFlattenLayerStackResolveAssetPath(
    const SdfLayerHandle& sourceLayer,
    const std::string& assetPath);

/// \class UsdFlattenResolveAssetPathContext
/// Context object containing information used when resolving asset paths
/// during layer stack flattening.
class UsdFlattenResolveAssetPathContext
{
public:
    /// Layer where the asset path is authored
    SdfLayerHandle sourceLayer;

    /// Authored asset path
    std::string assetPath;

    /// Expression variables from the layer stack
    VtDictionary expressionVariables;
};

/// Callback function for \c UsdFlattenLayerStack.
///
/// The callback is given a \c UsdFlattenResolveAssetPathContext containing
/// information needed to resolve a given asset path.  It should return the
/// \c std::string that should be authored in the flattened layer.
///
/// \sa UsdFlattenLayerStack
using UsdFlattenResolveAssetPathAdvancedFn = std::function<
    std::string(const UsdFlattenResolveAssetPathContext&)>;

/// Flatten the \p layerStack into a single layer with the given optional \p tag
/// and using the \p resolveAssetPathFn to resolve asset paths that are
/// encountered.
///
/// This is an advanced version of \c UsdFlattenLayerStack that provides full
/// control over how asset paths are resolved during flattening via the
/// \p resolveAssetPathFn callback. For example, the callback might maintain
/// relative asset paths instead of resolving them to absolute form. As
/// another example, the callback might maintain stage variable expressions
/// in their unevaluated form.
USD_API
SdfLayerRefPtr
UsdFlattenLayerStack(
    const PcpLayerStackRefPtr &layerStack,
    const UsdFlattenResolveAssetPathAdvancedFn& resolveAssetPathFn,
    const std::string& tag = std::string());

/// Implements the default asset path flattening behavior for
/// \c UsdFlattenLayerStack. The asset path in \p context will be anchored to
/// the source layer by calling SdfComputeAssetPathRelativeToLayer. If the
/// asset path contains a stage variable expression, it will be evaluated using
/// the expression variables in \p context before being anchored.
USD_API
std::string
UsdFlattenLayerStackResolveAssetPathAdvanced(
    const UsdFlattenResolveAssetPathContext& context);

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* PXR_USD_USD_FLATTEN_UTILS_H */
