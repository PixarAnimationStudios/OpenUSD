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
#ifndef PXR_USD_USD_UTILS_STITCH_CLIPS_H
#define PXR_USD_USD_UTILS_STITCH_CLIPS_H

/// \file usdUtils/stitchClips.h
///
/// Collection of utilities for sequencing multiple layers each holding
/// sequential time-varying data into
/// \ref Usd_Page_ValueClips "USD Value Clips".

#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/api.h"
#include "pxr/usd/usd/clipsAPI.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"

#include <limits>

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);

/// A function that creates layers that use
/// \ref Usd_Page_ValueClips "USD Value Clips"
/// to effectively merge the time samples in the given \p clipLayers under \p
/// clipPath without copying the samples into a separate layer.
///
/// \p resultLayer            The layer to which clip metadata and frame data 
///                           will be written. The layer representing the static
///                           scene topology will be authored as a sublayer on
///                           this layer as well; it will be authored as the 
///                           first sublayer in the list(strongest).
///
/// \p clipLayerFiles         The files containing the time varying data.
///
/// \p clipPath               The path at which we will put the clip metadata.
///
/// \p startTimeCode          The first time coordinate for the rootLayer 
///                           to point to. If none is provided, it will be 
///                           the lowest startTimeCode available from 
///                           the \p clipLayers.
///
/// \p endTimeCode            The last time coordinate for the rootLayer to 
///                           point to. If none is provided, it will be the 
///                           highest endTimeCode authored from the 
///                           \p clipLayers.
///
/// \p interpolateMissingClipValues
///                           Whether values for clips without samples are
///                           interpolated from surrounding clips. See
///                           UsdClipsAPI::GetInterpolateMissingClipValues
///                           for more details.
///
/// \p clipSet                The name of the clipSet in which the
///                           aforementioned metadata will be authored.
///                           \note If this parameter is omitted, the default
///                           clipSet name will be authored.
///
/// Details on how this is accomplished can be found below:
///
/// Pre-existing opinions will be wiped away upon success. Upon failure, the 
/// original topology and manifest layers, if pre-existing, will be preserved. 
/// These layers will be named/looked up via the following scheme: 
///
///     topologyLayerName = <resultIdWithoutExt>.topology.<resultExt>
///     manifestLayerName = <resultIdWithoutExt>.manifest.<resultExt>
///
/// For example: if the resultLayerFile's name is foo.usd the expected topology
/// layer will be foo.topology.usd and the expected manifest layer will be
/// foo.manifest.usd.
/// 
/// The topology layer contains the aggregated topology of the set of 
/// \p clipLayers. This process will merge prims and properties, save for time 
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
/// The manifest layer contains declarations for all attributes that exist
/// under \p clipPath and descendants in the clip layers with authored time
/// samples. Any default values authored into the topology layer for these
/// time sampled attributes will also be authored into the manifest.
///
/// The \p resultLayer will contain clip metadata at the specified \p clipPath.
/// The resultLayer will also have timeCode range data, such as start and end 
/// timeCodes written to it, with the starting position being provided by 
/// \p startTimeCode and the ending provided by \p endTimeCode.
///
/// Note: an invalid clip path(because the prim doesn't exist in
/// the aggregate topologyLayer) will result in a TF_CODING_ERROR.
/// 
USDUTILS_API
bool 
UsdUtilsStitchClips(const SdfLayerHandle& resultLayer, 
                    const std::vector<std::string>& clipLayerFiles,
                    const SdfPath& clipPath, 
                    const double startTimeCode 
                        = std::numeric_limits<double>::max(),
                    const double endTimeCode
                        = std::numeric_limits<double>::max(),
                    const bool interpolateMissingClipValues
                        = false,
                    const TfToken& clipSet
                        = UsdClipsAPISetNames->default_);

/// A function which aggregates the topology of a set of \p clipLayerFiles
/// for use in USD's Value Clips system. This aggregated scene topology
/// will only include non-time-varying data, as it is for use in conjunction
/// with the value clip metadata in a manifest layer.
///
/// \p topologyLayer          The layer in which topology of the 
///                           \p clipLayerFiles will be aggregated and inserted.
///
/// \p clipLayerFiles         The files containing the time varying data.
/// 
USDUTILS_API
bool 
UsdUtilsStitchClipsTopology(const SdfLayerHandle& topologyLayer, 
                            const std::vector<std::string>& clipLayerFiles);

/// A function which creates a clip manifest from the set of \p clipLayerFiles
/// for use in USD's Value Clips system. This manifest will contain
/// declarations for attributes with authored time samples in the clip layers.
/// If a time sampled attribute has a default value authored in the given 
/// \p topologyLayer, that value will also be authored as its default in the
/// manifest.
///
/// \p manifestLayer          The layer where manifest data will be inserted.
///
/// \p topologyLayer          The topology layer for \p clipLayerFiles.
///
/// \p clipLayerFiles         The files containing the time varying data.
/// 
/// \p clipPrimPath           The manifest will contain attributes from this
///                           prim and its descendants in \p clipLayerFiles.
USDUTILS_API
bool
UsdUtilsStitchClipsManifest(const SdfLayerHandle& manifestLayer,
                            const SdfLayerHandle& topologyLayer,
                            const std::vector<std::string>& clipLayerFiles,
                            const SdfPath& clipPath);

/// A function which authors clip template metadata on a particular prim in a 
/// result layer, as well as adding the topologyLayer to the list of subLayers
/// on the \p resultLayer. It will clear the \p resultLayer and create 
/// a prim at \p clipPath. Specifically, this will author clipPrimPath,
/// clipTemplateAssetPath, clipTemplateStride, clipTemplateStartTime,  
/// clipTemplateEndTime, and clipManifestAssetPath.
///
/// \p resultLayer            The layer in which we will author the metadata.
///
/// \p topologyLayer          The layer containing the aggregate topology of 
///                           the clipLayers which the metadata refers to.
///
/// \p manifestLayer          The layer containing manifest for the attributes
///                           in the clipLayers.
///
/// \p clipPath               The path at which to author the metadata in 
///                           \p resultLayer
///
/// \p templatePath           The template string to be authored at the 
///                           clipTemplateAssetPath metadata key.
///
/// \p startTime              The start time to be authored at the 
///                           clipTemplateStartTime metadata key.
///
/// \p endTime                The end time to be authored at the 
///                           clipTemplateEndTime metadata key.
///
/// \p stride                 The stride to be authored at the 
///                           clipTemplateStride metadata key.
///
/// \p activeOffset           The offset to be authored at the 
///                           clipTemplateActiveOffset metadata key. 
///                           \note If this parameter is omitted, no value 
///                           will be authored as the metadata is optional. 
///
/// \p interpolateMissingClipValues
///                           Whether values for clips without samples are
///                           interpolated from surrounding clips. See
///                           UsdClipsAPI::GetInterpolateMissingClipValues
///                           for more details.
///
/// \p clipSet                The name of the clipSet in which the
///                           aforementioned metadata will be authored.
///                           \note If this parameter is omitted, the default
///                           clipSet name("default") will be authored.
///
/// For further information on these metadatum, see \ref Usd_Page_AdvancedFeatures
///
USDUTILS_API
bool
UsdUtilsStitchClipsTemplate(const SdfLayerHandle& resultLayer,
                            const SdfLayerHandle& topologyLayer,
                            const SdfLayerHandle& manifestLayer,
                            const SdfPath& clipPath,
                            const std::string& templatePath,
                            const double startTime,
                            const double endTime,
                            const double stride,
                            const double activeOffset
                                = std::numeric_limits<double>::max(),
                            const bool interpolateMissingClipValues
                                = false,
                            const TfToken& clipSet
                                = UsdClipsAPISetNames->default_);

/// Generates a topology file name based on an input file name
/// 
/// For example, if given 'foo.usd', it generates 'foo.topology.usd'
/// 
/// Note: this will not strip preceding paths off of a file name
/// so /bar/baz/foo.usd will produce /bar/baz/foo.topology.usd
///
/// \p rootLayerName      The filepath used as a basis for generating
///                       our topology layer name.
USDUTILS_API
std::string
UsdUtilsGenerateClipTopologyName(const std::string& rootLayerName);

/// Generates a manifest file name based on an input file name
/// 
/// For example, if given 'foo.usd', it generates 'foo.manifest.usd'
/// 
/// Note: this will not strip preceding paths off of a file name
/// so /bar/baz/foo.usd will produce /bar/baz/foo.manifest.usd
///
/// \p rootLayerName      The filepath used as a basis for generating
///                       our manifest layer name.
USDUTILS_API
std::string
UsdUtilsGenerateClipManifestName(const std::string& rootLayerName);

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* PXR_USD_USD_UTILS_STITCH_CLIPS_H */
