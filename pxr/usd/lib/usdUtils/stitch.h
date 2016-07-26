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
#ifndef _USDUTILS_STITCH_H_
#define _USDUTILS_STITCH_H_

#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/spec.h"
SDF_DECLARE_HANDLES(SdfLayer);

/// \file stitch.h
///
/// Collection of module-scoped utilities for combining layers.
/// These utilize the convention of a strong and a weak layer.
/// The strong layer will be the first parameter to the function
/// and will always have precedence in conflicts during the merge.

/// The function will recurse down the root prims of each layer,
/// either making clean copies if no path match is found or recursing to
/// any subelements such as properties and metadata.
///
/// When stitching occurs, the prims are at the same level of a hierarchy
/// For example, if the trees look like this
///
/// (pseudoroot)          (pseudoroot)
/// |                     |
/// |                     |
/// |___(def "foo")       |___(def "foo")
///     |                     |
///     |_(timeSamples)       |_(timeSamples)
///        |_ {101: (.....)}    |_ {102: (.....)}
///
/// We would see the def "foo" in our \p weakLayer
/// already exists in our \p strongLayer ,
/// pictured on the left, so we would recurse into the "foo" prims
/// and see if there were any subelements we could copy over, this 
/// would involve examining their timeSample maps(just as one example,
/// all items with an infoKey are examined). A map-join is done on 
/// the timeSample maps with the strong keys taking precedence, so we get
/// this
///
/// (pseudoroot)
/// |
/// |
/// |___(def "foo")
///     |
///     |_(timeSamples)
///       |_ {101: (....), 102: (....)
///
/// Note that for non map types, if the key is already populated in the
/// corresponding strong prim, we do nothing, and if it isn't we copy over
/// the corresponding value in the weak prim.
///
/// Stitching also involves examining layer-level properties, such as
/// frames-per-second. This is done in the same way
/// as it is with prims, with the strong layer taking precedence
/// and the weak layers element being copied over if none exists in the strong.
//
/// The exception is start frame and end frame. These are calculated
/// by taking the minimum frame seen across the layers as the start frame
/// and the maximum frame across the layers as the end frame.
///
/// For list edited data, like references, inherits and relationships,
/// the stronger layer will win in conflict, no merging is done.
///
/// Also note that for time samples, the values are directly examined with
/// no fuzzying of the numbers, so, if strongLayer contains a timeSample
/// 101.000001 and weakLayer contains one at 101.000002, both will be in
/// strongLayer after the operation.
///
/// Verification is done post stitching to warn the user if time samples 
/// outside of the range were provided or if a begin frame is missing to 
/// corresponding end frame or vise-versa.
void UsdUtilsStitchLayers(const SdfLayerHandle& strongLayer, 
                          const SdfLayerHandle& weakLayer,
                          bool ignoreTimeSamples = false);

/// This function will stitch all data collectable with ListInfoKeys()
/// from the SdfLayer API. In the case of dictionaries, it will do 
/// a dictionary style composition. In the case of flat data, 
/// we will follow our traditional rule: If \p strongObj has the key 
/// already, nothing changes, if it does not and \p weakObj does, 
/// we will copy \p weakObj's info over.
void UsdUtilsStitchInfo(const SdfSpecHandle& strongObj, 
                        const SdfSpecHandle& weakObj,
                        bool ignoreTimeSamples = false);

#endif // _USDUTILS_STITCH_H_
