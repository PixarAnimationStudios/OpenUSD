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
#ifndef _USDUTILS_STITCH_CLIPS_H_
#define _USDUTILS_STITCH_CLIPS_H_

/// \file usdUtils/stitchClips.h
///
/// Collection of utilities for sequencing multiple layers each holding
/// sequential time-varying data into
/// \ref Usd_ClipsOverview "USD Value Clips".

#include "pxr/usd/usdUtils/api.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"
SDF_DECLARE_HANDLES(SdfLayer);

#include <limits>

/// A function that creates layers that use
/// \ref Usd_ClipsOverview "USD Value Clips"
/// to effectively merge the time samples in the given \p clipLayers under \p
/// clipPath without copying the samples into a separate layer.
///
/// \p resultLayer            The layer to which clip meta data and frame data 
///                           will be written
///
/// \p clipLayerFiles         The files containing the time varying data.
///
/// \p clipPath               The path at which we will put the clip meta data.
///
/// \p reuseExistingTopology  Whether or not we will attempt to reuse an 
///                           existing topology file.
///
/// \p startTimeCode          The first time coordinate for the rootLayer 
///                           to point to. If none is provided, it will be 
///                           the lowest startTimeCode available from 
///                           the \p clipLayers
///
/// Details on how this is accomplished can be found below:
///
/// This will begin by generating a topology layer, if necessary.
/// If the user has marked \p reuseExistingTopology as true, and a layer
/// exists, it will be reused. Otherwise, a fresh one will be generated. 
/// In either case, topology layers will be named/looked up 
/// via the following scheme: 
///
///     topologyLayerName = <resultIdWithoutExt>.topology.<resultExt>
///
/// For example: if the resultLayerFile's name is foo.usd the expected topology
/// layer will be foo.topology.usd. 
/// 
/// This layer contains the aggregated topology of the set of \p clipLayers.
/// This process will merge prims and properties, save for time 
/// varying properties, those will be accessed from the original
/// clip files.
///
/// The aggregation of topology works by merging a clipLayer at a time with
/// the topologyLayer. If a prim already exists in the topologyLayer, its 
/// attributes will be merged.
///
/// For example, if we have a layer, clipA with attributes /World/fx/foo.bar
/// and a second layer with /World/fx/foo.baz. Our aggregate topology layer
/// will contain both /World/fx/foo.bar, /World/fx/foo.baz.
///
/// The \p resultLayer will contain clip meta data: clipTimes, clipPrimPath 
/// clipManifestAssetPath, clipActive etc. at the specified \p clipPath.
/// The resultLayer will also have timeCode range data, such as start and end 
/// timeCodes written to it, with the starting position being provided by 
/// \p startTimeCode.
///
/// Note: an invalid clip path(because the prim doesn't exist in
/// the aggregate topologyLayer) will result in a TF_CODING_ERROR.
/// 
/// Note: if this function fails, the root layer will be not be created.
/// If the topology is not being reused, it will not be generated either.
USDUTILS_API bool 
UsdUtilsStitchClips(const SdfLayerHandle& resultLayer, 
                    const std::vector<std::string>& clipLayerFiles,
                    const SdfPath& clipPath, 
                    const bool reuseExistingTopology
                        = true,
                    const double startTimeCode 
                        = std::numeric_limits<double>::max());

#endif // _USDUTILS_STITCH_CLIPS_H_
