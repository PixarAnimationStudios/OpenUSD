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

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* USDUTILS_FLATTEN_LAYER_STACK_H_ */
