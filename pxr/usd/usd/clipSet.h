//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_CLIP_SET_H
#define PXR_USD_USD_CLIP_SET_H

#include "pxr/pxr.h"

#include "pxr/usd/usd/clip.h"
#include "pxr/usd/usd/valueUtils.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/declarePtrs.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_PTRS(PcpLayerStack);

class GfInterval;
class Usd_ClipSet;
class Usd_ClipSetDefinition;

using Usd_ClipSetRefPtr = std::shared_ptr<Usd_ClipSet>;

/// \class Usd_ClipSet
///
/// Represents a clip set for value resolution. A clip set primarily
/// consists of a list of Usd_Clip objects from which attribute values
/// are retrieved during value resolution.
///
class Usd_ClipSet
{
public:
    /// Create a new clip set based on the given definition. If clip 
    /// set creation fails, returns a null pointer and populates
    /// \p status with an error message. Otherwise \p status may be
    /// populated with other information or debugging output.
    static Usd_ClipSetRefPtr New(
        const std::string& name,
        const Usd_ClipSetDefinition& definition,
        std::string* status);

    Usd_ClipSet(const Usd_ClipSet&) = delete;
    Usd_ClipSet& operator=(const Usd_ClipSet&) = delete;

    /// Return the active clip at the given \p time. This should
    /// always be a valid Usd_ClipRefPtr.
    ///
    /// This overload tries to find if \p time has any jump discontinuity, and
    /// if so, and if querying a pre-time, it will return the previous clip.
    ///
    /// \sa GetActiveClip(UsdTimeCode time, bool timeHasJumpDiscontinuity)
    const Usd_ClipRefPtr& GetActiveClip(UsdTimeCode time) const
    {
        if (!time.IsPreTime()) {
            // When querying an ordinary time, we do not need to check if there 
            // is a jump discontinuity at time, the active clip will be decided 
            // based on the later time mapping.
            return GetActiveClip(time, false /*timeHasJumpDiscontinuity*/);
        }

        return GetActiveClip(
            time, _HasJumpDiscontinuityAtTime(time.GetValue()));
    }

    /// Return the active clip at the given \p time. This should
    /// always be a valid Usd_ClipRefPtr.
    ///
    /// If \p timeHasJumpDiscontinuity is true, and \p time is a pre-time 
    /// then our active clip should be previous clip.
    const Usd_ClipRefPtr& GetActiveClip(
        UsdTimeCode time, bool timeHasJumpDiscontinuity) const
    {
        size_t clipIndex = _FindClipIndexForTime(time.GetValue());
        return (timeHasJumpDiscontinuity && time.IsPreTime() && clipIndex > 0) ?
            valueClips[clipIndex - 1] : valueClips[clipIndex];
    }

    /// Returns the previous clip given a \p clip. 
    ///
    /// If there is no previous clip, this \p clip is returned as the previous 
    /// clip.
    const Usd_ClipRefPtr& GetPreviousClip(
        const Usd_ClipRefPtr& clip) const
    {
        auto it = std::find(valueClips.begin(), valueClips.end(), clip);
        if (it == valueClips.end()) {
            TF_CODING_ERROR("Clip must be in clip set");
            return clip;
        }
        if (it != valueClips.begin()) {
            return *(--it);
        }
        // No previous clip, return the same clip.
        return clip; 
    }

    /// Return bracketing time samples for the attribute at \p path
    /// at \p time.
    bool GetBracketingTimeSamplesForPath(
        const SdfPath& path, double time,
        double* lower, double* upper) const;

    /// Returns the previous time sample authored just before the querying \p 
    /// time.
    ///
    /// If there is no time sample authored just before \p time, this function
    /// returns false. Otherwise, it returns true and sets \p tPrevious to the
    /// time of the previous sample.
    bool GetPreviousTimeSampleForPath(
        const SdfPath& path, double time, double* tPrevious) const;

    /// Return set of time samples for attribute at \p path.
    std::set<double> ListTimeSamplesForPath(const SdfPath& path) const;

    /// Return list of time samples for attribute at \p path
    /// in the given \p interval.
    std::vector<double> GetTimeSamplesInInterval(
        const SdfPath& path, const GfInterval& interval) const;

    /// Query time sample for the attribute at \p path at \p time.
    /// If no time sample exists in the active clip at \p time,
    /// \p interpolator will be used to try to interpolate the
    /// value from the surrounding time samples in the active clip.
    /// If the active clip has no time samples, use the default
    /// value for the attribute declared in the manifest. If no
    /// default value is declared, use the fallback value for
    /// the attribute's value type.
    template <class T>
    bool QueryTimeSample(
        const SdfPath& path, UsdTimeCode time, 
        Usd_InterpolatorBase* interpolator, T* value) const;

    /// Query time samples for an attribute at \p path at pre-time \p time if
    /// samples represent a jump discontinuity.
    ///
    /// If \p time is not a pre-time or it doesn't represent a jump
    /// discontinuity, this function returns false. Otherwise, it returns
    /// true and sets the pre-time sample value to \p value.
    template <class T>
    bool QueryPreTimeSampleWithJumpDiscontinuity(
        const SdfPath& path, UsdTimeCode time, 
        Usd_InterpolatorBase* interpolator, T* value) const;

    std::string name;
    PcpLayerStackPtr sourceLayerStack;
    SdfPath sourcePrimPath;
    SdfLayerHandle sourceLayer;
    SdfPath clipPrimPath;
    Usd_ClipRefPtr manifestClip;
    Usd_ClipRefPtrVector valueClips;
    bool interpolateMissingClipValues;

private:
    Usd_ClipSet(
        const std::string& name,
        const Usd_ClipSetDefinition& definition);

    // Return the index of the clip that is active at the given \p time.
    // This will always return a valid index into the valueClips list.
    size_t _FindClipIndexForTime(double time) const;

    /// Returns true if the \p time represents a jump discontinuity.
    ///
    bool _HasJumpDiscontinuityAtTime(double time) const;

    // Return whether the specified clip contributes time sample values
    // to this clip set for the attribute at \p path.
    bool _ClipContributesValue(
        const Usd_ClipRefPtr& clip, const SdfPath& path) const;

    /// Mapping of external to internal times, populated during clips
    /// population.
    std::shared_ptr<const Usd_Clip::TimeMappings> _times;
};

// ------------------------------------------------------------

template <class T>
inline bool
Usd_ClipSet::QueryTimeSample(
    const SdfPath& path, UsdTimeCode time, 
    Usd_InterpolatorBase* interpolator, T* value) const
{
    const Usd_ClipRefPtr& clip = 
        GetActiveClip(time, false /*timeHasJumpDiscontinuity*/);

    // First query the clip for time samples at the specified time.
    if (clip->QueryTimeSample(path, time, interpolator, value)) {
        return true;
    }

    // If no samples exist in the clip, get the default value from
    // the manifest. Return true if we get a non-block value, false
    // otherwise.
    return Usd_HasDefault(manifestClip, path, value) == 
        Usd_DefaultValueResult::Found;
}

template <class T>
inline bool
Usd_ClipSet::QueryPreTimeSampleWithJumpDiscontinuity(
    const SdfPath& path, UsdTimeCode time,
    Usd_InterpolatorBase* interpolator, T* value) const
{
    if (!time.IsPreTime()) {
        return false;
    }

    if (!_HasJumpDiscontinuityAtTime(time.GetValue())) {
        return false;
    }

    const Usd_ClipRefPtr& clip = 
        GetActiveClip(time, true /*timeHasJumpDiscontinuity*/);
    
    if (clip->QueryTimeSample(path, time, interpolator, value)) {
        return true;
    }

    return Usd_HasDefault(manifestClip, path, value) == 
        Usd_DefaultValueResult::Found;
}

// ------------------------------------------------------------

template <class T>
inline bool
Usd_QueryTimeSample(
    const Usd_ClipSetRefPtr& clipSet, const SdfPath& path,
    double time, Usd_InterpolatorBase* interpolator, T* result)
{
    return clipSet->QueryTimeSample(path, time, interpolator, result);
}

/// Generate a manifest layer for the given \p clips containing all
/// attributes under the given \p clipPrimPath. Note that this will
/// open the layers for all of these clips.
///
/// If \p writeBlocksForClipsWithMissingValues is \c true, the generated
/// manifest will have value blocks authored for each attribute at the
/// activation times of clips that do not contain time samples for that 
/// attribute.
///
/// The layer will contain the given \p tag in its identifier. 
SdfLayerRefPtr
Usd_GenerateClipManifest(
    const Usd_ClipRefPtrVector& clips, const SdfPath& clipPrimPath,
    const std::string& tag = std::string(),
    bool writeBlocksForClipsWithMissingValues = false);

/// Generate a manifest layer for the given \p clipLayers containing
/// all attributes under the given \p clipPrimPath. The layer will contain
/// the given tag in its identifier.
///
/// If \p clipActive is not nullptr, it must be a list of activation times
/// for the corresponding layer in \p clipLayers. This will be used to
/// author value blocks for each attribute at the activation times of clips 
/// that do not contain time samples for that attribute.
SdfLayerRefPtr
Usd_GenerateClipManifest(
    const SdfLayerHandleVector& clipLayers, const SdfPath& clipPrimPath,
    const std::string& tag = std::string(),
    const std::vector<double>* clipActive = nullptr);

/// Return true if the given layer is a manifest that has been automatically
/// generated because the user has not supplied one. These layers are anonymous
/// layers with a specific tag in their identifiers.
bool
Usd_IsAutoGeneratedClipManifest(const SdfLayerHandle& manifestLayer);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
