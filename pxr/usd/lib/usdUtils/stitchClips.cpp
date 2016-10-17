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
#include "pxr/usd/usdUtils/stitchClips.h"
#include "pxr/usd/usdUtils/stitch.h"

#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/spec.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/reference.h"

#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/warning.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/nullPtr.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/work/loops.h"

#include <tbb/parallel_reduce.h>

#include <boost/foreach.hpp>

#include <set>
#include <string>
#include <limits>
#include <tuple>

namespace {
    // typedefs
    // ------------------------------------------------------------------------
    using _SdfAssetArray = VtArray<SdfAssetPath>;
    using _ClipFileVector = std::vector<std::string>;

    // constants
    // ------------------------------------------------------------------------
    constexpr double TIME_MAX = std::numeric_limits<double>::max();

    // We insert the topology layer as the strongest
    constexpr size_t TOPOLOGY_SUBLAYER_STRENGTH = 0;

    // keys for indexing into the clip info
    // XXX: This code is shared in usd/clip.h
    // ------------------------------------------------------------------------
    const TfToken clipActiveKey = TfToken("clipActive");
    const TfToken clipPrimPathKey = TfToken("clipPrimPath");
    const TfToken clipTimeKey = TfToken("clipTimes");
    const TfToken clipAssetPathsKey = TfToken("clipAssetPaths");
    const TfToken clipManifestAssetPath = TfToken("clipManifestAssetPath");

    // Convenience function for wrapping up nice error message
    // when checking os permissions of a layers backing file.
    bool
    _LayerIsWritable(const SdfLayerHandle& layer)
    {
        if (layer 
            and TfIsFile(layer->GetIdentifier())
            and not TfIsWritable(layer->GetIdentifier())) {
            TF_RUNTIME_ERROR("Error: Layer %s is unwritable.", 
                             layer->GetIdentifier().c_str());
            return false;
        }

        return true;
    }

    // Private function for simplifying queries
    // Looks up a value at a prim and converts its type
    template <typename T>
    T
    _GetUnboxedValue(const SdfLayerRefPtr& resultLayer, 
                     const SdfPath& primPath,
                     const TfToken& key) 
    {
        SdfPrimSpecHandle prim = resultLayer->GetPrimAtPath(primPath);

        // if we have a valid prim in the layer
        if (prim) {
            VtValue boxedValue = prim->GetInfo(key);
            // if we are actually holding a T
            if (boxedValue.IsHolding<T>()) {
                return boxedValue.Get<T>();
            }
        }

        // return a default constructed T in error.
        return T();
    }

    // Appends the collection at \p path in the rhs layer
    // to the lhs at the same prim path. This is useful for 
    // joining collections outside which can't subscribe to our
    // notion of precedence in stitching.
    template <typename C>
    void
    _AppendCollection(const SdfLayerRefPtr& lhs, const SdfLayerRefPtr& rhs, 
                      const SdfPath& path, const TfToken& key)
    {
        const C lhsValues = _GetUnboxedValue<C>(lhs, path, key);
        const C rhsValues = _GetUnboxedValue<C>(rhs, path, key);

        C result;
        result.reserve(lhsValues.size() + rhsValues.size());

        for (const auto& value : lhsValues) {
            result.push_back(value);
        }

        for (const auto& value : rhsValues) {
            result.push_back(value);
        }

        lhs->GetPrimAtPath(path)->SetInfo(key, VtValue(result));
    }

    // Retime a set of clipActives that have been joined together with 
    // _MergeRootLayerMetadata.
    void 
    _RetimeClipActive(const SdfLayerHandle& layer, 
                      const SdfPath& path) 
    {
        size_t timer = 0; 
        auto result = _GetUnboxedValue<VtVec2dArray>(layer, path, clipActiveKey);

        for (auto& clipActive : result) {
            clipActive[1] = timer;
            timer++;
        }

        layer->GetPrimAtPath(path)->SetInfo(clipActiveKey, VtValue(result));
    }

    // Try to determine if we should use a relative path for this
    // clip asset path. If the clip's identifier is itself has no
    // directory component, assume it's relative to the result layer.
    // Otherwise, look at the real paths to see if the clip path
    // can be made relative to the result layer.
    std::string
    _GetRelativePathIfPossible(const std::string& referencedIdentifier,
                               const std::string& referencedRealPath,
                               const std::string& resultRealPath)
    {
        std::string resultingIdentifier;
        if (TfGetPathName(referencedIdentifier).empty()) {
            resultingIdentifier = "./" + referencedIdentifier;
        } else {
            std::string resultDir = TfGetPathName(resultRealPath);
            if (TfStringStartsWith(referencedRealPath, resultDir)) {
                resultingIdentifier = std::string(referencedRealPath).replace(0, 
                    resultDir.length(), "./");
            }
        }

        return resultingIdentifier.empty() ? 
                referencedIdentifier 
                : resultingIdentifier;
    }

    // During parallel generation, we will generate non-releative paths
    // for clipAssetPaths so we need to make a post-processing pass.
    // We want to respect paths which have already been normalized,
    // meaning paths which already existed in the root layer.
    // So we take the difference of the number of clips being stitched
    // in from the current assetPaths in the root layer(note that this
    // is to be called after parallel stitching). This difference shows
    // us how many asset paths need to be normalized.
    void 
    _NormalizeClipAssetPaths(const SdfLayerHandle& resultLayer, 
                             const SdfLayerRefPtrVector& clipLayers,
                             const SdfPath& clipPath) 
    {
        const auto currentAssetPaths 
            = _GetUnboxedValue<_SdfAssetArray>(resultLayer, clipPath, 
                                               clipAssetPathsKey);
        const auto diff = currentAssetPaths.size() - clipLayers.size();

        _SdfAssetArray result;
        result.reserve(currentAssetPaths.size());

        // keep existing paths which don't need to be normalized
        for (size_t i = 0; i < diff; ++i) {
            result.push_back(currentAssetPaths[i]);
        }

        const auto resultPath = resultLayer->GetRealPath();

        // update newly added clip paths which need normalizing
        for (size_t i = 0; i < currentAssetPaths.size() - diff; ++i) {
            const auto path 
                = _GetRelativePathIfPossible(clipLayers[i]->GetIdentifier(),
                                             clipLayers[i]->GetRealPath(),
                                             resultPath); 
            result.push_back(SdfAssetPath(path));
        }

        resultLayer->GetPrimAtPath(clipPath)->SetInfo(clipAssetPathsKey, 
                                                      VtValue(result));
    }

    // Merge to root layers metadata by joining the collections.
    // This works differently from traditional stitching in the following way:
    //
    // Given layers lhs and rhs, and some property x which holds a VtVec2dArray
    // In traditional stitching, if lhs has a valid x, we keep it. Else if
    // rhs has a valid x, we'll take that. In this approach, we combine
    // lhs's x and rhs's x. This is useful when we have multiple root layers
    // created during parallel stitching.
    void 
    _MergeRootLayerMetadata(const SdfLayerRefPtr& lhs,
                            const SdfLayerRefPtr& rhs,
                            const SdfPath& clipPath) 
    {
        _AppendCollection<_SdfAssetArray>(lhs, rhs, clipPath, clipAssetPathsKey);
        _AppendCollection<VtVec2dArray>(lhs, rhs, clipPath, clipTimeKey);
        _AppendCollection<VtVec2dArray>(lhs, rhs, clipPath, clipActiveKey);
    }

    // Add the clipPrimPath metadata at the specified \p stitchPath
    // within the \p resultLayer. The \p startTimeCode is used to determine
    void
    _StitchClipPrimPath(const SdfLayerRefPtr& resultLayer,
                        const SdfPath& stitchPath)
    {
        SdfPrimSpecHandle clipPrim 
            = resultLayer->GetPrimAtPath(stitchPath);       

        if (clipPrim) {
            clipPrim->SetInfo(clipPrimPathKey, 
                              VtValue(stitchPath.GetString()));
        }
    }

    
    ////////////////////////////////////////////////////////////////////////////
    // XXX(Frame->Time): backwards compatibility
    // Temporary helper functions to support backwards compatibility.
    // -------------------------------------------------------------------------

    static
    bool
    _HasStartFrame(const SdfLayerConstHandle &layer)
    {
        return layer->GetPseudoRoot()->HasInfo(SdfFieldKeys->StartFrame);
    }

    static
    bool
    _HasEndFrame(const SdfLayerConstHandle &layer)
    {
        return layer->GetPseudoRoot()->HasInfo(SdfFieldKeys->EndFrame);
    }

    static
    double
    _GetStartFrame(const SdfLayerConstHandle &layer)
    {
        VtValue startFrame = layer->GetPseudoRoot()->GetInfo(SdfFieldKeys->StartFrame);
        if (startFrame.IsHolding<double>())
            return startFrame.UncheckedGet<double>();
        return 0.0;
    }

    static
    double
    _GetEndFrame(const SdfLayerConstHandle &layer)
    {
        VtValue endFrame = layer->GetPseudoRoot()->GetInfo(SdfFieldKeys->EndFrame);
        if (endFrame.IsHolding<double>())
            return endFrame.UncheckedGet<double>();
        return 0.0;
    }

    // Backwards compatible helper function for getting the startTimeCode of a 
    // layer. 
    static 
    double
    _GetStartTimeCode(const SdfLayerConstHandle &layer) 
    {
        return layer->HasStartTimeCode() ? 
            layer->GetStartTimeCode() : 
            (_HasStartFrame(layer) ? _GetStartFrame(layer) : 0.0);
    }

    // Backwards compatible helper function for getting the endTimeCode of a 
    // layer. 
    static 
    double
    _GetEndTimeCode(const SdfLayerConstHandle &layer) 
    {
        return layer->HasEndTimeCode() ? 
            layer->GetEndTimeCode() : 
            (_HasEndFrame(layer) ? _GetEndFrame(layer) : 0.0);
    }

    ////////////////////////////////////////////////////////////////////////////

    void
    _StitchClipActive(const SdfLayerRefPtr& resultLayer,
                      const SdfLayerRefPtr& clipLayer,
                      const SdfPath& stitchPath)
    {
        VtVec2dArray currentClipActive 
            = _GetUnboxedValue<VtVec2dArray>(resultLayer,
                                             stitchPath,
                                             clipActiveKey);

        // grab the number of elements in clipAssetPaths
        // note that this code is contingent on _StitchClipAssetPath()
        // being called first in _StitchClipMetadata()
        const double clipIndex = static_cast<double> (
                                    resultLayer
                                        ->GetPrimAtPath(stitchPath)
                                        ->GetInfo(clipAssetPathsKey)
                                        .Get<VtArray<SdfAssetPath> >()
                                        .size()) - 1;

        if (resultLayer->GetPrimAtPath(stitchPath)) {
            const double startTimeCode = _GetStartTimeCode(clipLayer);
            const double endTimeCode = _GetEndTimeCode(clipLayer);
            const double timeSpent = endTimeCode - startTimeCode;

            // if it is our first clip
            if (currentClipActive.empty()) {
                currentClipActive.push_back(GfVec2d(startTimeCode, 
                                                    clipIndex));
            } else {
                currentClipActive.push_back(GfVec2d(startTimeCode+timeSpent,
                                                    clipIndex));
            }
            resultLayer
                ->GetPrimAtPath(stitchPath)
                ->SetInfo(clipActiveKey, VtValue(currentClipActive));
        }
    }

    // Add the clipTime metadata at the specified \p stitchPath
    // within the \p resultLayer. The \p startTimeCode is used to determine
    // the current stage frame which is incremented as we add clipTimes
    void 
    _StitchClipTime(const SdfLayerRefPtr& resultLayer,
                    const SdfLayerRefPtr& clipLayer,
                    const SdfPath& stitchPath)
    {
        VtVec2dArray currentClipTimes 
            = _GetUnboxedValue<VtVec2dArray>(resultLayer,
                                             stitchPath,
                                             clipTimeKey);

        if (resultLayer->GetPrimAtPath(stitchPath)) {
            const double startTimeCode = _GetStartTimeCode(clipLayer);
            const double endTimeCode = _GetEndTimeCode(clipLayer);
            const double timeSpent = endTimeCode - startTimeCode;

            // insert the sample pair into the cliptimes
            currentClipTimes.push_back(GfVec2d(startTimeCode, startTimeCode));

            // We need not author duplicate pairs
            if (timeSpent != 0) {
                currentClipTimes.push_back(GfVec2d(startTimeCode + timeSpent,
                                           endTimeCode));
            }

            resultLayer
                ->GetPrimAtPath(stitchPath)
                ->SetInfo(clipTimeKey, VtValue(currentClipTimes));
        }
    }

    void
    _StitchClipsTopologySubLayerPath(const SdfLayerRefPtr& resultLayer,
                                     const std::string& topIdentifier)
    {
        auto sublayers = resultLayer->GetSubLayerPaths();

        // We only want to add the topology layer if it hasn't been
        // previously sublayered into this result layer.
        if (std::find(sublayers.begin(), sublayers.end(), topIdentifier) 
            == sublayers.end()) {
            resultLayer->InsertSubLayerPath(topIdentifier, 
                                            TOPOLOGY_SUBLAYER_STRENGTH);
        }
    }

    // Add the clipAssetPath metadata at the specified \p stitchPath
    // within the \p resultLayer.
    void
    _StitchClipAssetPath(const SdfLayerRefPtr& resultLayer,
                         const SdfLayerRefPtr& clipLayer,
                         const SdfPath& stitchPath)
    {
       _SdfAssetArray currentAssets 
            = _GetUnboxedValue<_SdfAssetArray>(resultLayer,
                                               stitchPath,
                                               clipAssetPathsKey);
       
       if (resultLayer->GetPrimAtPath(stitchPath)) {
            std::string clipId = 
                _GetRelativePathIfPossible(clipLayer->GetIdentifier(),
                                           clipLayer->GetRealPath(),
                                           resultLayer->GetRealPath());

            currentAssets.push_back(SdfAssetPath(clipId));
            resultLayer
                ->GetPrimAtPath(stitchPath)
                ->SetInfo(clipAssetPathsKey, VtValue(currentAssets));
        }
    }

    // Add the clipManifestAssetPath metadata at the specified \p stitchPath
    // within the \p resultLayer.
    void
    _StitchClipManifest(const SdfLayerRefPtr& resultLayer,
                        const SdfLayerRefPtr& topologyLayer,
                        const SdfPath& stitchPath)
    {
        const std::string manifestAssetPath = 
            _GetRelativePathIfPossible(topologyLayer->GetIdentifier(),
                                       topologyLayer->GetRealPath(),
                                       resultLayer->GetRealPath());

        resultLayer
           ->GetPrimAtPath(stitchPath) 
           ->SetInfo(clipManifestAssetPath, 
                     VtValue(SdfAssetPath(manifestAssetPath)));
    }

    // Stitching can also be done on per frame data using the notion of
    // model clips. 
    // 
    // Model clip stitching works by creating a set of "overs" given the
    // specified topology file and stitchPath.
    // 
    // After creating the the new structure, the clip data is aggregated, this
    // includes clipManifestAssetPath, clipActive, 
    //          clipTimes, clipAssetPaths, clipPrimPath.
    //
    // For each layer, we add its layer identifier as an asset
    // to clipAssetPaths, set its clipTimes to its frame number,
    // retain the clipPrimPath and set clipActive to its position in the asset
    // array(the end of the array during this operation, since the asset was just
    // pushed on). If the layer is located at or under the same directory as
    // the output layer, its entry in clipAssetPaths will be a relative path.
    //
    // \p resultLayer the layer being merged into
    // \p clipLayer the layer we are merging clip data from
    // \p topologyLayer the layer with a reference topology
    // \p stitchPath the prim path in the reference topolgy we need to emulate
    // \p startTimeCode a frame number to start at for stage frames, 
    //    if no startTimeCode is supplied, the number will be taken from the most
    //    recently added clip data. If there is no other clip data, its taken 
    //    from the startTimeCode in clipLayer.
    // 
    //
    // Note: The clipLayer's start and end frame values reflect the
    // time sample values that the function will use
    void
    _StitchClipMetadata(const SdfLayerRefPtr& resultLayer,
                        const SdfLayerRefPtr& clipLayer,
                        const SdfPath& stitchPath,
                        const double startTimeCode)
    {
        // Create overs to match structure
        SdfCreatePrimInLayer(resultLayer, stitchPath);

        // Set the search path for this prim and its accompanying clip data
        // note that the ordering of these operations is important, as 
        // _StitchClipActive() and _StitchClipTime() rely on _ClipAssetPath()
        // having been called.
        _StitchClipPrimPath(resultLayer, stitchPath);
        _StitchClipAssetPath(resultLayer, clipLayer, stitchPath);
        _StitchClipActive(resultLayer, clipLayer, stitchPath);
        _StitchClipTime(resultLayer, clipLayer, stitchPath);
    }

    // This allows one to set the start and end frame data in 
    // a \p resultLayer, based on model clip data contained at 
    // \p stitchPath. This function will take the minimum available 
    // startTimeCode(unless one is supplied) from inside of the clipTimes at the 
    // \p stitchPath and the maximum available endTimeCode.
    //
    // Note: if the prim at \p stitchPath has no clip data, neither the start
    // nor end frame will be set by this operations
    void
    _SetTimeCodeRange(const SdfLayerHandle& resultLayer,
                      const SdfPath& clipDataPath,
                      double startTimeCode,
                      double endTimeCode) 
    {
        // it is a coding error to look up clip data in a non-existent path
        if (not resultLayer->GetPrimAtPath(clipDataPath)) {
            TF_CODING_ERROR("Invalid prim in path: @%s@<%s>",
                            resultLayer->GetIdentifier().c_str(),
                            clipDataPath.GetString().c_str());
            return;
        }

        // obtain the current set of clip times
        VtVec2dArray currentClipTimes 
            = _GetUnboxedValue<VtVec2dArray>(resultLayer,
                                             clipDataPath,
                                             clipTimeKey);

        // sort based on stage frame number
        std::sort(currentClipTimes.begin(), currentClipTimes.end(),
                  [](const GfVec2d& v1, const GfVec2d& v2) {
                    return v1[0] < v2[0]; 
                  });

        // exit if we have no data to set the startframes with
        if (currentClipTimes.empty()) {
            return;
        }

        // grab the min at the front and max at the back
        if (endTimeCode == TIME_MAX) {
            endTimeCode = (*currentClipTimes.rbegin())[0];
        }
        resultLayer->SetEndTimeCode(endTimeCode);

        if (startTimeCode == TIME_MAX) {
            startTimeCode = (*currentClipTimes.begin())[0];
        }
        resultLayer->SetStartTimeCode(startTimeCode);
    }

    // Generates a toplogy file name based on an input file name
    // 
    // For example, if given 'foo.usd', it generates 'foo.topology.usd'
    // 
    // Note: this will not strip preceding paths off of a file name
    // so /bar/baze/foo.usd will produce /bar/baze/foo.topology.usd
    std::string
    _CreateTopologyName(const std::string& baseFileName)
    {
        // XXX: perhaps we have do/could have a file delimiter defined in Tf
        //  as well as our topology convention.
        const std::string delimiter = ".";
        const std::size_t delimiterPos = baseFileName.rfind(".");
        const std::string topologyFileBaseName = "topology";
        if (delimiterPos == std::string::npos) {
            return std::string();
        }

        return std::string(baseFileName).insert(delimiterPos,
            delimiter+topologyFileBaseName);
    }

    std::tuple<SdfLayerRefPtr, bool>
    _CreateTopologyLayer(const SdfLayerHandle& resultLayer,
                         const bool reuseExistingTopology)
    {
        const auto topologyName 
            = _CreateTopologyName(resultLayer->GetIdentifier());

        bool topologyWasGenerated = false;

        SdfLayerRefPtr topologyLayer = SdfLayer::FindOrOpen(topologyName);

        if (not _LayerIsWritable(topologyLayer)) {
            return std::make_tuple(TfNullPtr, topologyWasGenerated); 
        }

        if (not reuseExistingTopology) {
            topologyWasGenerated = true;
            if (topologyLayer) {
                topologyLayer->Clear();
            } else {
                topologyLayer = SdfLayer::CreateNew(topologyName);
            }

        } else if (not topologyLayer) {
            topologyWasGenerated = true;
            topologyLayer = SdfLayer::CreateNew(topologyName); 
        }

        return std::make_tuple(topologyLayer, topologyWasGenerated);
    }

    struct _StitchLayersResult {
        SdfPath clipPath;
        SdfLayerRefPtr topology; 
        SdfLayerRefPtr root; 
            
        _StitchLayersResult(const SdfPath& _clipPath) 
            : clipPath(_clipPath),
              topology(SdfLayer::CreateAnonymous()), 
              root(SdfLayer::CreateAnonymous())
        {
        }

        _StitchLayersResult(const _StitchLayersResult& s, tbb::split)
            : _StitchLayersResult(s.clipPath)
        {
        }

        void operator()(const tbb::blocked_range<
                          SdfLayerRefPtrVector::const_iterator>& clipLayers)
        {
            for (const auto& layer : clipLayers) {
                UsdUtilsStitchLayers(topology, layer,
                                     /*ignoreTimeSamples=*/ true);
                if (clipPath != SdfPath::AbsoluteRootPath()) {
                    _StitchClipMetadata(root, layer, clipPath,  
                                        _GetStartTimeCode(layer));
                }
            } 
        }

        void join(_StitchLayersResult& rhs) {
            UsdUtilsStitchLayers(topology, rhs.topology,
                                 /*ignoreTimeSamples=*/ true);  
            if (clipPath != SdfPath::AbsoluteRootPath()) {
                _MergeRootLayerMetadata(root, rhs.root, clipPath);             
            }
        }
    };

    _StitchLayersResult
    _AggregateDataFromClips(const SdfLayerRefPtr& topologyLayer,
                            const SdfLayerRefPtrVector& clipLayers,
                            const SdfPath& clipPath=SdfPath::AbsoluteRootPath())
    {
        // Create a result which will store the result of the 
        // successive computations done by parallel_reduce
        _StitchLayersResult result(clipPath);
        tbb::blocked_range<SdfLayerRefPtrVector::const_iterator>
            clipRange(clipLayers.begin(), clipLayers.end());
        tbb::parallel_reduce(clipRange, result);

        return result;
    }

    // Stitches a manifest file, containing the clip meta data aggregated
    // from the input \p clipLayers. These include clipPrimPath, clipTimes, 
    // clipManifestAssetPath clipActive and clipAssetPaths as well as an 
    // authored reference to the \p topologyLayer
    // Stitches a topology file in \p topologyLayer, based on the aggregate 
    // topology of \p clipLayers at the specified \p clipPath. 
    void
    _StitchLayers(const SdfLayerHandle& resultLayer,
                  const SdfLayerRefPtr& topologyLayer,
                  const SdfLayerRefPtrVector& clipLayers,
                  const SdfPath& clipPath)
    {
        auto result = _AggregateDataFromClips(
            topologyLayer, clipLayers, clipPath);
        UsdUtilsStitchLayers(topologyLayer, result.topology, true);

        // if the rootLayer has no clip-metadata authored 
        if (not resultLayer->GetPrimAtPath(clipPath)) {
            // we need to run traditional stitching to add the prim structure
            UsdUtilsStitchLayers(resultLayer, result.root, true);
        } else {
            _MergeRootLayerMetadata(resultLayer, result.root, clipPath);
        }

        // we need to retime in either case, because the clips
        // may be aggregated in parallel, and thus will have clipActives
        // which are out of sync with one another.
        _RetimeClipActive(resultLayer, clipPath);
        _NormalizeClipAssetPaths(resultLayer, clipLayers, clipPath);

        // set the topology reference and manifest path because we
        // use anonymous layers during parallel reduction
        _StitchClipManifest(resultLayer, topologyLayer, clipPath);
        
        // fetch the rootPrim from the topology layer
        if (topologyLayer->GetRootPrims().empty()) {
            TF_CODING_ERROR("Failed to generate topology.");
        } else {
            SdfPrimSpecHandle rootPrim = *topologyLayer->GetRootPrims().begin();
            SdfPath rootPath = rootPrim->GetPath();
            const std::string topologyId 
                = _GetRelativePathIfPossible(topologyLayer->GetIdentifier(),
                                             topologyLayer->GetRealPath(),
                                             resultLayer->GetRealPath());

            _StitchClipsTopologySubLayerPath(resultLayer, topologyId); 
        }
    }

    bool
    _UsdUtilsStitchClipsTopologyImpl(const SdfLayerRefPtr& topologyLayer,
                                     const SdfLayerRefPtrVector& clipLayers)
    {
        // Note that we don't specify a unique clipPath since we're only
        // interested in aggregating topology. 
        auto result  = _AggregateDataFromClips(topologyLayer, clipLayers);
        UsdUtilsStitchLayers(topologyLayer, result.topology, true);
        topologyLayer->Save();
        return true;
    }

    bool 
    _UsdUtilsStitchClipsImpl(const SdfLayerHandle& resultLayer, 
                             const SdfLayerRefPtr& topologyLayer,
                             const SdfLayerRefPtrVector& clipLayers,
                             const SdfPath& clipPath, 
                             const double startTimeCode,
                             const double endTimeCode)
    {
        _StitchLayers(resultLayer, topologyLayer, clipLayers, clipPath);
        _SetTimeCodeRange(resultLayer, clipPath, startTimeCode, endTimeCode);

        topologyLayer->Save();
        resultLayer->Save();

        return true;
    }

    bool 
    _ClipLayersAreValid(const SdfLayerRefPtrVector& clipLayers,
                        const _ClipFileVector& clipLayerFiles,
                        const SdfPath& clipPath) 
    {
        bool somePrimContainsPath = false;
        
        for (size_t i = 0; i < clipLayerFiles.size(); i++) { 
            const auto& layer = clipLayers[i];
            if (not layer) {
                TF_CODING_ERROR("Failed to open layer %s\n",
                                clipLayerFiles[i].c_str());
                return false;
            } else if (layer->GetPrimAtPath(clipPath)) {
                somePrimContainsPath = true;
            }
        }

        // if no clipLayers contain the primPath we want
        if (not somePrimContainsPath) {
            TF_CODING_ERROR("Invalid clip path specified <%s>", 
                            clipPath.GetString().c_str());
            return false;
        } 

        return true;
    }


    bool
    _OpenClipLayers(SdfLayerRefPtrVector* clipLayers,
                    const _ClipFileVector& clipLayerFiles,
                    const SdfPath& clipPath)
    {
        // Pre-allocate our destination vector for the clip layer handles
        clipLayers->resize(clipLayerFiles.size());

        // Open the clip layer files in parallel and place them into the vector
        WorkParallelForN(clipLayerFiles.size(),
            [&clipLayers, &clipLayerFiles](size_t begin, size_t end) 
            {
                for (size_t i = begin; i != end; ++i) {
                    (*clipLayers)[i] = SdfLayer::FindOrOpen(clipLayerFiles[i]);
                }
            });

        return _ClipLayersAreValid(*clipLayers, clipLayerFiles, clipPath);
    }
}

// public facing API
// ----------------------------------------------------------------------------

bool
UsdUtilsStitchClipsTopology(const SdfLayerHandle& topologyLayer,
                            const _ClipFileVector& clipLayerFiles)
{
    // XXX: This is necessary for any C++ API which may be called though
    // python. Since this will spawn workers(in WorkParallelForN) which 
    // will need to acquire the GIL, we need to explicitly release it.
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    if (not _LayerIsWritable(topologyLayer)) {
        return false;
    }

    SdfLayerRefPtrVector clipLayers;
    const bool clipLayersAreValid = _OpenClipLayers(&clipLayers, 
        clipLayerFiles, SdfPath::AbsoluteRootPath());

    if (not clipLayersAreValid) {
        return false;
    }

    return _UsdUtilsStitchClipsTopologyImpl(topologyLayer, clipLayers);
}

bool 
UsdUtilsStitchClips(const SdfLayerHandle& resultLayer, 
                    const _ClipFileVector& clipLayerFiles,
                    const SdfPath& clipPath, 
                    const bool reuseExistingTopology,
                    const double startTimeCode,
                    const double endTimeCode)
{
    // XXX: See comment in UsdUtilsStitchClipsTopology above.
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    if (not _LayerIsWritable(resultLayer)) {
        return false;
    }

    SdfLayerRefPtr topologyLayer;
    bool topologyWasGenerated;

    std::tie(topologyLayer, topologyWasGenerated) 
        = _CreateTopologyLayer(resultLayer, reuseExistingTopology);

    SdfLayerRefPtrVector clipLayers;
    const bool clipLayersAreValid 
        = _OpenClipLayers(&clipLayers, clipLayerFiles, clipPath);

    if (not clipLayersAreValid) {
        if (topologyWasGenerated) {
            TfDeleteFile(topologyLayer->GetIdentifier());
        }

        return false;
    }

    return _UsdUtilsStitchClipsImpl(resultLayer, topologyLayer, clipLayers, 
        clipPath, startTimeCode, endTimeCode);
}
