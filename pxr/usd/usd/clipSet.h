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
/// Clip sets support both time samples and spline value formats.
/// Within a clip set, an attribute cannot mix and match time samples
/// and splines. Note the following behaviors.
/// 1. Time samples are preferred. If a single clip contains both time
///    samples and splines for an attribute, the spline is ignored.
///    If an attribute has time samples and splines in different clips,
///    it will get values from time samples and the splines are ignored.
/// 2. If a manifest is authored, the value format that resolves from
///    the annotations is the source of truth. Other formats may be
///    defined in clips but ignored to prefer the manifest definition.
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
    /// If `time` is a pre-time and its value is exactly the start time
    /// of a clip, return the previous clip if there is a previous clip.
    const Usd_ClipRefPtr& GetActiveClip(UsdTimeCode time) const
    {
        size_t clipIndex = _FindClipIndexForTime(time.GetValue());
        if (clipIndex > 0 && time.IsPreTime()
            && valueClips[clipIndex]->startTime == time.GetValue())
        {
            return valueClips[clipIndex - 1];
        }

        return valueClips[clipIndex];
    }

    /// Convenience functions for determining attribute data format for a path
    /// @{
    bool ContainsValueForAttribute(const SdfPath& path) const;
    bool ContainsTimeSamplesForAttribute(const SdfPath& path) const;
    bool ContainsSplineForAttribute(const SdfPath& path) const;
    /// @}

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
        Usd_Interpolator const &interpolator, T* value) const;

    /// If there is a time sample for \p path at \p time, return its value's
    /// typeid(), otherwise return typeid(void).
    const std::type_info &QueryTimeSampleTypeid(
        const SdfPath& path, UsdTimeCode time) const;

    /// Query time samples for an attribute at \p path at pre-time \p time if
    /// samples represent a jump discontinuity.
    ///
    /// If \p time is not a pre-time or it doesn't represent a jump
    /// discontinuity, this function returns false. Otherwise, it returns
    /// true and sets the pre-time sample value to \p value.
    template <class T>
    bool QueryPreTimeSampleWithJumpDiscontinuity(
        const SdfPath& path, UsdTimeCode time, 
        Usd_Interpolator const &interpolator, T* value) const;

    template <class T>
    bool QuerySpline(const SdfPath& path, UsdTimeCode time, T* result) const;

    /// Output a spline assembled from clips for attribute at \p path ,
    /// returning true on successfully building a spline.
    ///
    /// If the manifest reports that the attribute doesn't exist or is not
    /// a spline, this returns false.
    ///
    /// This function concatenates clips' contributed splines from
    /// Usd_Clip::BuildSpline. If a clip doesn't have a spline, the
    /// following process is invoked:
    /// 1. If the manifest has a default value, a held spline with that value
    ///    is produced that coalesces adjacent clips that are missing splines.
    /// 2. If the manifest doesn't have a default value, a value block spline
    ///    is produced with the same coalescing behavior as (1).
    /// 3. If the manifest doesn't have a default value and
    ///    interpolateMissingClipValues=true, a spline with linear
    ///    interpolation between splines built from adjacent clips is produced
    ///    with the same coalescing behavior as (1).
    bool BuildSpline(const SdfPath& path, TsSpline* result) const;

    std::string name;
    PcpLayerStackPtr sourceLayerStack;
    SdfPath sourcePrimPath;
    SdfLayerHandle sourceLayer;
    SdfPath clipPrimPath;
    Usd_ClipRefPtr manifestClip;
    Usd_ClipRefPtrVector valueClips;
    bool interpolateMissingClipValues;
    SdfLayerOffset toStageOffset;

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

    /// Return true if the \p clip has an authored value of the data format
    /// indicated by the manifest for the attribute at \p path.
    ///
    /// If the manifest authors a value block at the active time of a clip,
    /// returns false without consulting the clip itself.
    bool _ClipContainsAuthoredValueForAttribute(const Usd_ClipRefPtr& clip,
                                                const SdfPath& path) const;

    // Return whether the specified clip contributes time sample values
    // to this clip set for the attribute at \p path.
    bool _ClipContributesTimeSamples(
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
    Usd_Interpolator const &interpolator, T* value) const
{
    const Usd_ClipRefPtr& clip = GetActiveClip(time);

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
    Usd_Interpolator const &interpolator, T* value) const
{
    if (!time.IsPreTime()) {
        return false;
    }

    if (!_HasJumpDiscontinuityAtTime(time.GetValue())) {
        return false;
    }

    return QueryTimeSample(path, time, interpolator, value);
}

// ------------------------------------------------------------

template <class T>
inline bool
Usd_QueryTimeSample(
    const Usd_ClipSetRefPtr& clipSet, const SdfPath& path,
    double time, Usd_Interpolator const &interpolator, T* result)
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
