//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_SPLINE_KEY_FRAMES_H
#define PXR_BASE_TS_SPLINE_KEY_FRAMES_H

#include "pxr/pxr.h"
#include "pxr/base/ts/keyFrame.h"
#include "pxr/base/ts/keyFrameMap.h"
#include "pxr/base/ts/loopParams.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/vt/value.h"

#include <vector>
#include <map>
#include <typeinfo>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TsSpline_KeyFrames 
/// \brief Maintains the keyframes for a spline
///
/// The TsSpline_KeyFrames is a private class that holds onto and provides 
/// API for interacting with the spline's keyframes.  Its principle duty
/// is to manage the looping/non-looping representations of the spline.  This
/// class should only be held by TsSpline.
///
class TsSpline_KeyFrames
{
public:
    TsSpline_KeyFrames();
    ~TsSpline_KeyFrames();

    /// Generalized copy constructor.
    ///
    /// If \a keyFrames is not NULL, this constructor has the same behavior
    /// as first copying other, then calling SetKeyFrames with keyFrames.
    TsSpline_KeyFrames(TsSpline_KeyFrames const &other,
                         TsKeyFrameMap const *keyFrames = NULL);

    /// Gets the looped or unlooped keys, according to whether the spline is 
    /// looping.
    const TsKeyFrameMap & GetKeyFrames() const;

    /// If looping, just writes to the non unrolled intervals.
    void SetKeyFrames(const TsKeyFrameMap&);

    /// Replaces the key frames of this spline with keyFrames, and replaces
    /// the contents of keyFrames with the key frames in this spline.  If
    /// the spline is looping, the data put into keyFrames will be the key
    /// frames from the looped view of the spline, and hidden keys will be
    /// preserved when keyFrames is swapped into this spline.
    void SwapKeyFrames(std::vector<TsKeyFrame>* keyFrames);

    /// If looping, just writes to the non unrolled intervals.
    void SetKeyFrame( TsKeyFrame kf, GfInterval 
            *intervalAffected=NULL );

    /// If looping, just affects the non unrolled intervals.
    void RemoveKeyFrame( TsTime t, GfInterval 
            *intervalAffected=NULL );

    /// Clears both maps.
    void Clear();
    
    /// Gets the underlying normal keys.
    const TsKeyFrameMap & GetNormalKeyFrames() const;

    /// Get the loop parameters.
    const TsLoopParams &GetLoopParams() const;
    
    /// Sets the loop parameters.
    void SetLoopParams(const TsLoopParams &loopParams);

    /// Get the left and right extrapolation.
    const TsExtrapolationPair &GetExtrapolation() const;

    /// Sets the left and right extrapolation.
    void SetExtrapolation(const TsExtrapolationPair &extrapolation);

    bool operator==(const TsSpline_KeyFrames &rhs) const;

    // Bakes looped key frames out and turns looping off.
    void BakeSplineLoops();

private:
    typedef std::pair<TsKeyFrameMap::iterator,
                      TsKeyFrameMap::iterator> _KeyFrameRange;

    // Get a pointer to the keyframes that lets us change them
    TsKeyFrameMap *_GetKeyFramesMutable();

    // Returns the time interval affected by an edit to a keyframe at
    // the given time.
    GfInterval _GetTimeInterval( TsTime time );

    // Copy the normal to the looped and then unroll the master keys
    void _SetLoopedFromNormal();

    // Copy the master, prepeat and repeated intervals from the looped keys to
    // the normal keys
    void _SetNormalFromLooped();

    // Unroll the master interval of the looped keys to itself; clears the
    // entire unrolled region first
    void _UnrollMaster();

    void _UnrollKeyFrames(TsKeyFrameMap *keyFrames,
                            const TsLoopParams &params);

    // Unroll the given range of _loopedKeyFrames.  If times is given, return
    // the times that were written.  Does not clear the unrolled region before
    // writing.
    void _UnrollKeyFrameRange(TsKeyFrameMap *keyFrames,
                                const TsKeyFrameMap::iterator &k0,
                                const TsKeyFrameMap::iterator &k1,
                                const TsLoopParams &params,
                                std::vector<TsTime> *times = NULL);

    // Returns the range of keyframes including time as non-const iterators.
    // If there is a keyframe at \p time then this is the keyframe before the
    // keyframe at \p time to the keyframe after that one.  If there isn't a
    // keyframe at \p time then it's the closest keyframes before and after \p
    // time.
    _KeyFrameRange _GetKeyFrameRange( TsTime time );

    // Returns the range of keyframes including the time interval as non-const
    // iterators.  These are the key frames from the key frame before (not at)
    // \p leftTime to the key frame after (not at) \p rightTime.
    _KeyFrameRange _GetKeyFrameRange( TsTime leftTime, TsTime rightTime );

    // Returns the time interval that will be changed by removing a key frame
    // at the given \p time.
    GfInterval _FindRemoveKeyFrameChangedInterval(TsTime time);

    // Returns the time interval that will be changed by setting the given
    // \p keyFrame on the spline.
    GfInterval _FindSetKeyFrameChangedInterval(const TsKeyFrame &keyFrame);

    // Determine the effective extrapolation for \p keyframe on \p side
    TsExtrapolationType _GetEffectiveExtrapolationType(
        const TsKeyFrame &keyFrame,
        const TsSide &side) const;

    /// The Spline calls these when the loop params have changed.
    void _LoopParamsChanged(bool loopingChanged, bool valueOffsetChanged,
            bool domainChanged);

private:
    friend class TsKeyFrameEvalUtil;
    friend class TsSpline;

    TsExtrapolationPair _extrapolation;
    TsLoopParams _loopParams;
    TsKeyFrameMap _normalKeyFrames;
    TsKeyFrameMap _loopedKeyFrames;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
