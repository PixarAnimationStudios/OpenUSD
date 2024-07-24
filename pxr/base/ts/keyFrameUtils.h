//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_KEY_FRAME_UTILS_H
#define PXR_BASE_TS_KEY_FRAME_UTILS_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/ts/loopParams.h"

PXR_NAMESPACE_OPEN_SCOPE

class Ts_Data;
class TsKeyFrame;
class TsKeyFrameMap;

/// \brief Finds the keyframe in \p keyframes closest to the given
/// time.  Returns NULL if there are no keyframes.
const TsKeyFrame* Ts_GetClosestKeyFrame(
    const TsKeyFrameMap &keyframes,
    const TsTime targetTime );

/// \brief Finds the closest keyframe in \p keyframes before the given
/// time.  Returns NULL if no such keyframe exists.
const TsKeyFrame* Ts_GetClosestKeyFrameBefore(
    const TsKeyFrameMap &keyframes,
    const TsTime targetTime );

/// \brief Finds the closest keyframe in \p keyframes after the given
/// time.  Returns NULL if no such keyframe exists.
const TsKeyFrame* Ts_GetClosestKeyFrameAfter(
    const TsKeyFrameMap &keyframes,
    const TsTime targetTime );

/// \brief Equivalant to calling Ts_GetClosestKeyFrameBefore and
/// Ts_GetClosestKeyFrameAfter, but twice the speed; for performance
/// critical applications.  
std::pair<const TsKeyFrame *, const TsKeyFrame *>
Ts_GetClosestKeyFramesSurrounding(
    const TsKeyFrameMap &keyframes,
    const TsTime targetTime );

/// \brief Returns true if the segment between the given (adjacent) key 
/// frames is flat.
bool Ts_IsSegmentFlat(const TsKeyFrame &kf1, const TsKeyFrame &kf2 );

/// \brief Returns true if the given key frame is redundant.
bool Ts_IsKeyFrameRedundant( 
    const TsKeyFrameMap &keyframes,
    const TsKeyFrame &keyFrame,
    const TsLoopParams &loopParams=TsLoopParams(),
    const VtValue& defaultValue=VtValue());

// Return a pointer to the Ts_Data object held by the keyframe.
// XXX: exported because used by templated functions starting from TsEvaluator
TS_API
Ts_Data* Ts_GetKeyFrameData(TsKeyFrame &kf);

// Return a const pointer to the Ts_Data object held by the keyframe.
// XXX: exported because used by templated functions starting from TsEvaluator
TS_API
Ts_Data const* Ts_GetKeyFrameData(TsKeyFrame const& kf);

// Uses a fixed epsilon to compare the values, iff both are float or double,
// else falls back to VtValue ==.
bool Ts_IsClose(const VtValue &v0, const VtValue &v1);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
