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
#ifndef USD_CLIP_H
#define USD_CLIP_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/node.h"

#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/propertySpec.h"

#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/staticTokens.h"

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include <iosfwd>
#include <map>
#include <mutex>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class PcpPrimIndex;

/// Returns true if the given scene description metadata \p fieldName is
/// associated with value clip functionality.
bool
UsdIsClipRelatedField(const TfToken& fieldName);

/// \class Usd_ResolvedClipInfo
///
/// Object containing resolved clip metadata for a prim in a LayerStack.
///
struct Usd_ResolvedClipInfo
{
    Usd_ResolvedClipInfo() : indexOfLayerWhereAssetPathsFound(0) { }

    bool operator==(const Usd_ResolvedClipInfo& rhs) const
    {
        return (clipAssetPaths == rhs.clipAssetPaths
            && clipManifestAssetPath == rhs.clipManifestAssetPath
            && clipPrimPath == rhs.clipPrimPath
            && clipActive == rhs.clipActive
            && clipTimes == rhs.clipTimes
            && indexOfLayerWhereAssetPathsFound 
                    == rhs.indexOfLayerWhereAssetPathsFound);
    }

    bool operator!=(const Usd_ResolvedClipInfo& rhs) const
    {
        return !(*this == rhs);
    }

    size_t GetHash() const
    {
        size_t hash = indexOfLayerWhereAssetPathsFound;
        if (clipAssetPaths) {
            for (const auto& assetPath : *clipAssetPaths) {
                boost::hash_combine(hash, assetPath.GetHash());
            }
        }
        if (clipManifestAssetPath) {
            boost::hash_combine(hash, clipManifestAssetPath->GetHash());
        }
        if (clipPrimPath) {
            boost::hash_combine(hash, *clipPrimPath);
        }               
        if (clipActive) {
            for (const auto& active : *clipActive) {
                boost::hash_combine(hash, active[0]);
                boost::hash_combine(hash, active[1]);
            }
        }
        if (clipTimes) {
            for (const auto& time : *clipTimes) {
                boost::hash_combine(hash, time[0]);
                boost::hash_combine(hash, time[1]);
            }
        }

        return hash;
    }

    boost::optional<VtArray<SdfAssetPath> > clipAssetPaths;
    boost::optional<SdfAssetPath> clipManifestAssetPath;
    boost::optional<std::string> clipPrimPath;
    boost::optional<VtVec2dArray> clipActive;
    boost::optional<VtVec2dArray> clipTimes;
    size_t indexOfLayerWhereAssetPathsFound;
};

/// Resolves clip metadata values for the prim index \p primIndex.
/// Returns the source node where either clipAssetPaths or 
/// clipTemplateAssetPath was found.
PcpNodeRef
Usd_ResolveClipInfo(
    const PcpPrimIndex& primIndex,
    Usd_ResolvedClipInfo* clipInfo);

/// Sentinel values authored on the edges of a clipTimes range.
constexpr double Usd_ClipTimesEarliest = -std::numeric_limits<double>::max();
constexpr double Usd_ClipTimesLatest = std::numeric_limits<double>::max();

/// \class Usd_Clip
///
/// Represents a clip from which time samples may be read during
/// value resolution.
///
struct Usd_Clip : public boost::noncopyable
{
public:
    /// A clip has two time domains: an external and an internal domain.
    /// The internal time domain is what is authored in the clip layer.
    /// The external time domain is what is used by clients of Usd_Clip.
    /// 
    /// The TimeMapping object specifies a mapping from external time to
    /// internal time. For example, mapping [ 0:10 ] means that a request
    /// for time samples at time 0 should retrieve the sample authored at
    /// time 10 in the clip layer. Consumers of Usd_Clip will always deal
    /// with external times. Usd_Clip will convert between time domains as
    /// needed.
    /// 
    /// The mappings that apply to a clip are given in a TimeMappings object.
    /// Times are linearly interpolated between entries in this object.
    /// For instance, given a mapping [ 0:10, 10:20 ], external time 0 maps
    /// to internal time 10, time 5 maps to time 15, and time 10 to time 20.
    /// The simplest way to visualize this is to imagine that TimeMappings
    /// specifies a piecewise-linear function, with each pair of TimeMapping
    /// entries specifying a single segment.
    /// 
    /// Time mappings are authored in the 'clipTimes' metadata. This allows
    /// users to control the timing of animation from clips, potentially
    /// offsetting or scaling the animation. 
    typedef double ExternalTime;
    typedef double InternalTime;
    struct TimeMapping {
        ExternalTime externalTime;
        InternalTime internalTime;

        TimeMapping() {}
        TimeMapping(const ExternalTime e, const InternalTime i) 
            : externalTime(e),
              internalTime(i)
        {}
    };

    typedef std::vector<TimeMapping> TimeMappings;

    Usd_Clip();
    Usd_Clip(
        const PcpNodeRef& clipSourceNode,
        size_t clipSourceLayerIndex,
        const SdfAssetPath& clipAssetPath,
        const SdfPath& clipPrimPath,
        ExternalTime clipStartTime,
        ExternalTime clipEndTime,
        const TimeMappings& timeMapping);

    bool HasField(const SdfAbstractDataSpecId& id, const TfToken& field) const;

    SdfPropertySpecHandle GetPropertyAtPath(
            const SdfAbstractDataSpecId& id) const;

    template <class T>
    bool HasField(
        const SdfAbstractDataSpecId& id, const TfToken& field,
        T* value) const
    {
        return _GetLayerForClip()->HasField(
            _TranslateIdToClip(id).id, field, value);
    }

    size_t GetNumTimeSamplesForPath(const SdfAbstractDataSpecId& id) const;

    // Internal function used during value resolution. When determining
    // resolve info sources, value resolution needs to determine when clipTimes
    // are mapping into an empty clip with no samples, so it can continue
    // searching for value sources. 
    size_t _GetNumTimeSamplesForPathInLayerForClip(
            const SdfAbstractDataSpecId& id) const {
        return _GetLayerForClip()->GetNumTimeSamplesForPath(
                                        _TranslateIdToClip(id).id);
    }

    std::set<ExternalTime>
    ListTimeSamplesForPath(const SdfAbstractDataSpecId& id) const;

    bool GetBracketingTimeSamplesForPath(
        const SdfAbstractDataSpecId& id, ExternalTime time, 
        ExternalTime* tLower, ExternalTime* tUpper) const;

    template <class T>
    bool QueryTimeSample(
        const SdfAbstractDataSpecId& id, ExternalTime time, 
        T* value) const
    {
        const _TranslatedSpecId clipId = _TranslateIdToClip(id);
        const InternalTime clipTime = _TranslateTimeToInternal(time);
        const SdfLayerRefPtr& clip = _GetLayerForClip();

        if (clip->QueryTimeSample(clipId.id, clipTime, value)) {
            return true;
        }

        // See comment in Usd_Clip::GetBracketingTimeSamples.
        double lowerInClip, upperInClip;
        if (clip->GetBracketingTimeSamplesForPath(
                clipId.id, clipTime, &lowerInClip, &upperInClip)) {
            return clip->QueryTimeSample(clipId.id, lowerInClip, value);
        }

        return false;
    }

    /// Return the layer associated with this clip iff it has already been
    /// opened successfully.
    ///
    /// USD tries to be as lazy as possible about opening clip layers to
    /// avoid unnecessary latency and memory bloat; however, once a layer is
    /// open, it will generally be kept open for the life of the stage.
    SdfLayerHandle GetLayerIfOpen() const;

    /// Node in the composed prim index and index of layer in its LayerStack
    /// where this clip was introduced.
    PcpNodeRef sourceNode;
    size_t sourceLayerIndex;

    /// Asset path for the clip and the path to the prim in the clip
    /// that provides data.
    SdfAssetPath assetPath;
    SdfPath primPath;

    /// A clip is active in the time range [startTime, endTime).
    ExternalTime startTime;
    ExternalTime endTime;

    /// Mapping of external to internal times.
    TimeMappings times;

private:
    friend class UsdStage;

    struct _TranslatedSpecId {
    public:
        _TranslatedSpecId(const SdfPath& path, const TfToken& propName)
            : id(&_path, &propName)
            , _path(path)
            { }

        SdfAbstractDataSpecId id;

    private:
        SdfPath _path;
    };

    std::set<InternalTime>
    _GetMergedTimeSamplesForPath(const SdfAbstractDataSpecId& id) const;

    bool 
    _GetBracketingTimeSamplesForPathInternal(const SdfAbstractDataSpecId& id, 
                                             ExternalTime time, 
                                             ExternalTime* tLower, 
                                             ExternalTime* tUpper) const;

    _TranslatedSpecId _TranslateIdToClip(const SdfAbstractDataSpecId& id) const;

    // Helpers to translate between internal and external time domains.
    InternalTime _TranslateTimeToInternal(
        ExternalTime extTime) const;
    ExternalTime _TranslateTimeToExternal(
        InternalTime clipTime, TimeMapping m1, TimeMapping m2) const;

    SdfLayerRefPtr _GetLayerForClip() const;

private:
    mutable bool _hasLayer;
    mutable std::mutex _layerMutex;
    mutable SdfLayerRefPtr _layer;
};

typedef boost::shared_ptr<Usd_Clip> Usd_ClipRefPtr;
typedef std::vector<Usd_ClipRefPtr> Usd_ClipRefPtrVector;

std::ostream&
operator<<(std::ostream& out, const Usd_ClipRefPtr& clip);


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_CLIP_H
