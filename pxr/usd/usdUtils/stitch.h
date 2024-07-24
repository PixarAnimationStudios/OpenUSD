//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_UTILS_STITCH_H
#define PXR_USD_USD_UTILS_STITCH_H

/// \file usdUtils/stitch.h
///
/// Collection of module-scoped utilities for combining layers.
/// These utilize the convention of a strong and a weak layer. The strong
/// layer will be the first parameter to the function and will always have
/// precedence in conflicts during the merge.

#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/api.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/spec.h"

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);

/// Merge all scene description in \p weakLayer into \p strongLayer.
///
/// Prims and properties in \p weakLayer that do not exist in \p strongLayer
/// will be copied into \p strongLayer. Prims and properties that do
/// exist in \p strongLayer will be merged with the existing scene
/// description.
///
/// Merging prims and properties is done on a field-by-field basis.
/// In general, if a field has a value in \p strongLayer, the value from
/// \p weakLayer will be ignored. However, certain fields have special
/// rules for merging values together:
///
/// - For map and dictionary-valued fields (including time samples),
///   a dictionary merge is performed; values in the weaker dictionary
///   are copied into the stronger dictionary only if the key does not
///   already exist.
///
/// - For listOp-valued fields, the listOps will be combined into a 
///   single listOp. The historical "add" and "reorder" list op operations
///   cannot be combined in this way; "add" will be converted to "append", 
///   and "reorder" will be discarded.
///
/// - The minimum startTimeCode value and maximum endTimeCode value will
///   be used.
USDUTILS_API
void UsdUtilsStitchLayers(
    const SdfLayerHandle& strongLayer, 
    const SdfLayerHandle& weakLayer);

/// Merge the scene description for \p weakObj into \p strongObj.
///
/// See documentation on UsdUtilsStitchLayers for a description of
/// the merging behavior.
USDUTILS_API
void UsdUtilsStitchInfo(
    const SdfSpecHandle& strongObj, 
    const SdfSpecHandle& weakObj);

/// \name Advanced Stitching API
/// @{

/// Status enum returned by UsdUtilsStitchValueFn describing the
/// desired value stitching behavior.
enum class UsdUtilsStitchValueStatus
{
    NoStitchedValue, ///< Don't stitch values for this field.
    UseDefaultValue, ///< Use the default stitching behavior for this field.
    UseSuppliedValue ///< Use the value supplied in stitchedValue.
};

/// Callback for customizing how values are stitched together. 
/// 
/// This callback will be invoked for each field being stitched from the 
/// source spec at \p path in \p weakLayer to the destination spec at 
/// \p path in \p strongLayer. \p fieldInStrongLayer and \p fieldInWeakLayer 
/// indicates whether the field has values in either layer.
///
/// The callback should return a UsdUtilsStitchValueStatus to indicate the
/// desired behavior. Note that if the callback returns UseSuppliedValue and
/// supplies an empty VtValue in \p stitchedValue, the field will be removed
/// from the destination spec.
using UsdUtilsStitchValueFn = std::function<
    UsdUtilsStitchValueStatus(
        const TfToken& field, const SdfPath& path,
        const SdfLayerHandle& strongLayer, bool fieldInStrongLayer,
        const SdfLayerHandle& weakLayer, bool fieldInWeakLayer,
        VtValue* stitchedValue)>;

/// Advanced version of UsdUtilsStitchLayers that accepts a \p stitchValueFn
/// callback to customize how fields in \p strongLayer and \p weakLayer are
/// stitched together. See documentation on UsdUtilsStitchValueFn for more
/// details.
USDUTILS_API
void UsdUtilsStitchLayers(
    const SdfLayerHandle& strongLayer, 
    const SdfLayerHandle& weakLayer,
    const UsdUtilsStitchValueFn& stitchValueFn);

/// Advanced version of UsdUtilsStitchInfo that accepts a \p stitchValueFn
/// callback to customize how fields in \p strongObj and \p weakObj are
/// stitched together. See documentation on UsdUtilsStitchValueFn for more
/// details.
USDUTILS_API
void UsdUtilsStitchInfo(
    const SdfSpecHandle& strongObj, 
    const SdfSpecHandle& weakObj,
    const UsdUtilsStitchValueFn& stitchValueFn);

/// @}

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* PXR_USD_USD_UTILS_STITCH_H */
