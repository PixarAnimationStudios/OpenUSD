//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_SPLINE_H
#define PXR_BASE_TS_SPLINE_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/keyFrame.h"
#include "pxr/base/ts/keyFrameMap.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/ts/loopParams.h"
#include "pxr/base/vt/value.h"

#include <vector>
#include <limits>
#include <map>
#include <typeinfo>
#include <iostream>
#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

class TsSpline_KeyFrames;

/// \class TsSpline 
///
/// Represents a spline value object.  
///
/// The TsSpline class defines spline representations.  Use this class to 
/// define and manipulate avars over time.  An TsSpline object is an 
/// anonymous value that can be freely passed around.  It has no owning object.
///
/// Internally TsSpline is copy-on-write. This means that making a copy
/// of an TsSpline is nearly free in both time and memory, but making an
/// edit to a copy of an TsSpline may incur the cost of copying all the
/// data.
///
/// TsSpline provides the basic thread safety guarantee: Multiple threads
/// may read and copy an TsSpline object concurrently, but it's not safe
/// to read from an TsSpline which another thread is concurrently writing
/// to. Internally that means that any data which may be mutated by a const
/// accessor must be protected by a mutex. Currently TsSpline has an immutable
/// lock-free implementation.
///
class TsSpline final
{
public:
    /// Constructs a spline with no key frames and held extrapolation.
    TS_API
    TsSpline();

    /// Copy construct.
    TS_API
    TsSpline(const TsSpline &other);

    /// Constructs a spline with the key frames \e keyFrames,
    /// and optionally given extrapolation and looping parameters.
    TS_API
    explicit TsSpline( const TsKeyFrameMap & keyFrames,
             TsExtrapolationType leftExtrapolation = TsExtrapolationHeld,
             TsExtrapolationType rightExtrapolation = TsExtrapolationHeld,
             const TsLoopParams &loopParams = TsLoopParams());

    /// Constructs a spline with the key frames \e keyFrames,
    /// and optionally given extrapolation and looping parameters.
    TS_API
    explicit TsSpline( const std::vector<TsKeyFrame> & keyFrames,
             TsExtrapolationType leftExtrapolation = TsExtrapolationHeld,
             TsExtrapolationType rightExtrapolation = TsExtrapolationHeld,
             const TsLoopParams &loopParams = TsLoopParams());

    /// Equality operator.
    TS_API
    bool operator==(const TsSpline &rhs) const;

    /// Inequality operator.
    TS_API
    bool operator!=(const TsSpline &rhs) const;

    /// Returns whether there are any keyframes.
    TS_API
    bool IsEmpty() const;

    /// Replaces the KeyFrames in this TsSpline with those in
    /// swapInto, and puts the KeyFrames in this TsSpline into
    /// swapInto.  Requires that the vectors in swapInto are sorted
    /// in ascending order according to their time.
    TS_API
    void SwapKeyFrames(std::vector<TsKeyFrame>* swapInto);

    /// Removes redundant keyframes from the spline in the specified
    /// multi-interval.
    /// \return True if the spline was changed, false if not.
    /// \param defaultValue Used only to decide whether to remove the final
    /// keyframe.  The final keyframe is removed if defaultValue is specified
    /// and the final keyframe has this value.
    /// \param intervals Only keyframes in the given multiInterval will be
    /// removed, although all keyframes will be considered in computing what
    /// is redundant.
    TS_API
    bool ClearRedundantKeyFrames( const VtValue &defaultValue = VtValue(),
                                  const GfMultiInterval &intervals =
                                        GfMultiInterval(GfInterval(
                                    -std::numeric_limits<double>::infinity(),
                                    std::numeric_limits<double>::infinity())));


    /// Returns the keyframes in this spline.
    ///
    /// Note that any non-const method invalidates the reference to
    /// the KeyFrameMap. Do not hold on to the KeyFrameMap reference,
    /// make a modification to the spline, and then use the original
    /// KeyFrameMap reference.
    TS_API
    const TsKeyFrameMap& GetKeyFrames() const;

    /// Returns the "raw" keyframes in this spline whether or not this
    /// spline is looping.  Note that this method is the sole exception to the
    /// rule that the API presents the looped view of the spline when it is 
    /// a looping spline.
    ///
    /// Note that any non-const method invalidates the reference to
    /// the KeyFrameMap. Do not hold on to the KeyFrameMap reference,
    /// make a modification to the spline, and then use the original
    /// KeyFrameMap reference.
    TS_API
    const TsKeyFrameMap& GetRawKeyFrames() const;

    /// Returns the keyframes contained in the given GfMultiInterval.
    TS_API
    std::vector<TsKeyFrame>
    GetKeyFramesInMultiInterval(const GfMultiInterval &) const;

    /// Returns the minimum and maximum keyframe frames in the spline.  If there
    /// are no keyframes, the returned range will be empty.
    TS_API
    GfInterval GetFrameRange() const;

    /// Sets a keyframe, optionally returning the time range affected.
    /// If a keyframe already exists at the specified time, it will be
    /// replaced.  If the keyframe is not a valid type to set, an error
    /// will be emitted; to avoid this, call CanSetKeyFrame() first.
    TS_API
    void SetKeyFrame(
        TsKeyFrame kf, GfInterval *intervalAffected=nullptr );

    /// Checks if the given keyframe is a valid candidate to set,
    /// optionally returning the reason if it cannot.
    TS_API
    bool CanSetKeyFrame(
        const TsKeyFrame & kf, std::string *reason=nullptr ) const;

    /// \brief Breakdown at time \e x.
    ///
    /// If a key frame exists at \e x then this does nothing, otherwise it
    /// inserts a key frame of type \e type at \e x. If the provided \e value is
    /// empty (the default), the new key frame's value is chosen such that the
    /// value at \e x doesn't change. If \e value is not empty, the new keyframe
    /// is always given that value.
    ///
    /// If \e flatTangents is \c false and \e x is between the first and last
    /// key frames then it will also try to preserve the shape of the spline as
    /// much as possible.  Otherwise, if the key frame type and value type
    /// support tangents, the key frame will have tangents with zero slope and
    /// length \e tangentLength.
    ///
    /// The return value is either the newly broken down keyframe, or the
    /// existing keyframe at the given time. If an error has occurred, an
    /// empty value may be returned.
    TS_API
    std::optional<TsKeyFrame>
    Breakdown( double x, TsKnotType type,
               bool flatTangents, double tangentLength,
               const VtValue &value = VtValue(),
               GfInterval *intervalAffected=nullptr );

    /// Breaks down simultaneously at several times.
    ///
    /// When creating knots with flat tangents, the shape of the spline may
    /// change between the new knot and its adjacent knots. Simply breaking
    /// down a spline several times in a loop may result in key frame values
    /// that drift away from their original values. This function samples the
    /// spline first, ensuring that each new key frame will preserve the value
    /// at that time.
    ///
    /// If \e value is not empty, \e value is used instead of sampling the
    /// spline. For each time, if there is already a key frame at that time, the
    /// value and type of that keyframe will not be changed.
    ///
    /// The arguments are the same as Breakdown(). If \p keyFramesAtTimes is
    /// given, it will be populated with the newly broken down or previously 
    /// existing key frames at the given times.
    TS_API
    void
    Breakdown( const std::set<double> & times, TsKnotType type,
               bool flatTangents, double tangentLength,
               const VtValue &value = VtValue(), 
               GfInterval *intervalAffected=nullptr,
               TsKeyFrameMap *keyFramesAtTimes=nullptr);

    /// Breaks down simultaneously at several times.
    ///
    /// Caller can provide a value for each time. If a value is not provided
    /// at a given time (it is empty), this function will sample the spline.
    /// If a knot already exists at a given time, its value is not modified.
    ///
    /// The arguments are the same as Breakdown(). If \p keyFramesAtTimes is
    /// given, it will be populated with the newly broken down or previously 
    /// existing key frames at the given times.
    TS_API
    void
    Breakdown( const std::vector<double> & times, TsKnotType type,
               bool flatTangents, double tangentLength,
               const std::vector<VtValue> & values,
               GfInterval *intervalAffected=nullptr,
               TsKeyFrameMap *keyFramesAtTimes=nullptr);

    /// Breaks down simultaneously at several times with knot types specified 
    /// for each time.
    ///
    /// A knot type for each time must be provided, else it is a coding error.
    ///
    /// Caller can provide a value for each time. If a value is not provided
    /// at a given time (it is empty), this function will sample the spline.
    /// If a knot already exists at a given time, its value is not modified.
    ///
    /// The arguments are the same as Breakdown(). If \p keyFramesAtTimes is
    /// given, it will be populated with the newly broken down or previously 
    /// existing key frames at the given times.
    TS_API
    void
    Breakdown( const std::vector<double> & times, 
               const std::vector<TsKnotType> & types,
               bool flatTangents, double tangentLength,
               const std::vector<VtValue> & values,
               GfInterval *intervalAffected=nullptr,
               TsKeyFrameMap *keyFramesAtTimes=nullptr);

    /// Removes the keyframe at the given time, optionally returning the
    /// time range affected.
    TS_API
    void RemoveKeyFrame(
        TsTime time, GfInterval *intervalAffected=nullptr);

    /// Removes all keyframes.  This does not affect extrapolation.  If this
    /// spline is looping, knots hidden under the loop echos will not be
    /// removed.
    TS_API
    void Clear();

    /// \brief Finds the keyframe closest to the given time.
    /// Returns an empty value if there are no keyframes.
    TS_API
    std::optional<TsKeyFrame>
    GetClosestKeyFrame( TsTime targetTime ) const;

    /// \brief Finds the closest keyframe before the given time.
    /// Returns an empty value if no such keyframe exists.
    TS_API
    std::optional<TsKeyFrame>
    GetClosestKeyFrameBefore( TsTime targetTime )const;

    /// \brief Finds the closest keyframe after the given time.
    /// Returns an empty value if no such keyframe exists.
    TS_API
    std::optional<TsKeyFrame>
    GetClosestKeyFrameAfter( TsTime targetTime ) const;

    /// \brief Returns true if the given key frame is redundant.
    ///
    /// A key frame is redundant if it can be removed without affecting the
    /// value of the spline at any time. If a spline has only one key frame
    /// and that key frame has the same value as this spline's default
    /// value, then that key frame is considered redundant.  If a 
    /// \c defaultValue parameter is not supplied, the last knot on a spline
    /// is never considered redundant.
    TS_API
    bool IsKeyFrameRedundant( const TsKeyFrame &keyFrame,
                              const VtValue &defaultValue = VtValue() ) const;

    /// \brief Returns true if the key frame at the given time is redundant.
    ///
    /// This is a convenience function for the version that takes a
    /// TsKeyFrame.  If there is no key frame at the indicated time a
    /// TF_CODING_ERROR will occur and false is returned.
    TS_API
    bool IsKeyFrameRedundant( TsTime keyFrameTime,
                              const VtValue &defaultValue = VtValue() ) const;

    /// \brief Returns true if any of this spline's key frames are redundant.
    TS_API
    bool HasRedundantKeyFrames( const VtValue &defaultValue = VtValue() ) const;

    /// \brief Returns true if the segment between the given (adjacent) key 
    /// frames is flat.
    TS_API
    bool IsSegmentFlat( const TsKeyFrame &kf1, 
                        const TsKeyFrame &kf2 ) const;

    /// \brief Returns true if the segment between the given (adjacent) key
    /// frames is flat.
    ///
    /// This function will log a TF_CODING_ERROR if there is no key frame at
    /// either of the indicated times.
    TS_API
    bool IsSegmentFlat( TsTime startTime, TsTime endTime ) const;
                         
    /// \brief Returns true if the segment between the given (adjacent) key 
    /// frames is monotonic (i.e. no extremes).
    ///
    /// This function will log a TF_CODING_ERROR if kf1 >= kf2 
    /// TODO describe the preconditions
    /// 
    TS_API
    bool
    IsSegmentValueMonotonic( const TsKeyFrame &kf1, 
                             const TsKeyFrame &kf2 ) const;

    /// \brief Returns true if the segment between the given (adjacent) key
    /// frames is monotonic (i.e. no extremes).
    ///
    /// Given times must correspond to key frames.
    /// see also IsSegmentValueMonotonic(kf1, kf2)
    TS_API
    bool 
    IsSegmentValueMonotonic( TsTime startTime, TsTime endTime ) const;

    /// \brief Returns true if the value of the spline changes over time,
    /// whether due to differing values among keyframes or knot sides, 
    /// or value changes via non-flat tangents.  If allowEpsilonDifferences is
    /// true, then if the spline is of type double, then knot value
    /// differences that are tiny will count as 0.
    TS_API
    bool
    IsVarying() const;

    /// \brief Like IsVarying(), but for splines of type double, allows tiny
    /// value differences.
    TS_API
    bool
    IsVaryingSignificantly() const;

    /// Sets the spline's extrapolation type on each side.
    TS_API
    void SetExtrapolation(
            TsExtrapolationType left, TsExtrapolationType right);

    /// Returns the spline's extrapolation type on each side (\c first
    /// is the left side).
    TS_API
    std::pair<TsExtrapolationType, TsExtrapolationType>
    GetExtrapolation() const;

    /// Returns the typeid of the value type for keyframes in this spline.
    /// If no keyframes have been set, this will return typeid(void).
    TS_API
    const std::type_info &
    GetTypeid() const;

    /// Returns the TfType of the value type for keyframes in this spline.
    /// If no keyframes have been set, this will return unknown type
    TS_API
    TfType
    GetType() const;

    /// Returns the typename of the value type for keyframes in this spline,
    /// If no keyframes have been set, this will return "void".
    TS_API
    std::string GetTypeName() const;
    
    /// Evaluates the value of the spline at the given time, interpolating the
    /// keyframes.  If there are no keyframes, an empty VtValue is returned.
    TS_API
    VtValue Eval(
        TsTime time, TsSide side=TsRight ) const;

    /// Evaluates the value of the spline at the given time without any
    /// interpolation, as if all keyframes and extrapolation modes were of
    /// type "held".
    ///
    /// If there are no keyframes, an empty VtValue is returned.
    TS_API
    VtValue EvalHeld( TsTime time, TsSide side=TsRight ) const;

    /// Evaluates the derivative of the spline at the given time, interpolating
    /// the keyframes.  If there are no keyframes, an empty VtValue is returned.
    TS_API
    VtValue EvalDerivative(
        TsTime time, TsSide side=TsRight ) const;

    /// Returns whether the left-side value and the right-side value at the
    /// specified time are different.  This is always false for a time where
    /// there is no keyframe.  For a keyframe time, the sides differ if (1)
    /// there is a dual-valued keyframe with different values on the left and
    /// right side; or (2) the keyframe follows a held segment whose value does
    /// not match the keyframe's right-side value.  Contrast this method with
    /// TsKeyFrame::GetIsDualValued, which only reports whether a keyframe is
    /// configured to have dual values.
    TS_API
    bool DoSidesDiffer(TsTime time) const;

    /// \brief Evaluates the value of the spline over the given time interval.
    /// When the returned samples are scaled by \e timeScale and 
    /// \e valueScale and linearly interpolated, the reconstructed curve
    /// will nowhere have an error greater than \e tolerance.
    ///
    /// Samples may be point samples or "blur" samples.  A blur sample
    /// covers a finite time domain and a value range.  It indicates that
    /// the value varies very quickly in the domain and that,  to the given
    /// tolerance, only the minimum and maximum values are of interest.
    /// Blur domains are always half-open on the right.
    ///
    /// Samples are returned in non-decreasing time order.  Two samples
    /// may have equal time in two cases.  First, if both are point samples
    /// then the first is the left side evaluation of the value at time
    /// and the second is the right side evaluation.  Second, if the first
    /// sample is a point sample and second is a blur sample then the point
    /// sample is the left side evaluation of time.  Blur domains will not
    /// overlap and point samples, with the above exception, will not be
    /// inside any blur domain.
    ///
    /// Samples may be returned outside the given time interval.
    TS_API
    TsSamples Sample(
        TsTime startTime, TsTime endTime,
        double timeScale, double valueScale,
        double tolerance ) const;

    TS_API
    std::pair<VtValue, VtValue>
    GetRange( TsTime startTime, TsTime endTime ) const;

    /// Returns whether spline represents a simple linear relationship.
    TS_API
    bool IsLinear() const;

    /// Returns whether the given key frame is in the looped interval, but
    /// not in the master interval.
    TS_API
    bool KeyFrameIsInLoopedRange(const TsKeyFrame & kf);

    /// \brief Return an object describing all the looping parameters for
    /// this spline.
    TS_API
    TsLoopParams GetLoopParams() const;

    /// \brief Set the looping parameters for this spline.
    TS_API
    void SetLoopParams(const TsLoopParams&);

    // If this spline is a looping spline, bakes the looped key frames
    // out and turns looping off.  Hidden keyframes will be lost.
    TS_API
    void BakeSplineLoops();

    /// \brief Is the given time in the "unrolled" region of a spline that is
    /// looping; i.e. not in the master region
    TS_API
    bool IsTimeLooped(TsTime time) const;

    /// Our iterators are simply iterators into the contained TsKeyFrameMap
    /// We only expose const iterators because when a KeyFrame changes we
    /// need to update other internal state.
    typedef TsKeyFrameMap::const_iterator const_iterator;
    typedef TsKeyFrameMap::const_reverse_iterator const_reverse_iterator;

    /// Some utilities (such as TfIterator) expect a class named 'iterator'.
    /// We provide that as a typdef to const_iterator in order to avoid exposing
    /// real non-const iterators.
    typedef const_iterator iterator;
    typedef const_reverse_iterator reverse_iterator;

    /// \group Container API
    ///
    /// Provide STL container compliant API.
    ///
    /// Some of these methods are inlined because they are often called
    /// as part of looping constructions where the cost of function calls
    /// could be high.
    /// @{

    /// Returns the number of KeyFrames in this spline.
    TS_API
    size_t size() const {
        return GetKeyFrames().size();
    }

    /// Return true if this spline has no KeyFrames.
    TS_API
    bool empty() const {
        return GetKeyFrames().empty();
    }
    
    /// Return a const_iterator pointing to the beginning of the spline.
    TS_API
    const_iterator begin() const {
        return const_iterator(GetKeyFrames().begin());
    }

    /// Returns a const_iterator pointing to the end of the spline. (one past
    /// the last KeyFrame)
    TS_API
    const_iterator end() const {
        return const_iterator(GetKeyFrames().end());
    }

    /// Return a const_reverse_iterator pointing to the end of the spline.
    TS_API
    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(GetKeyFrames().rbegin());
    }

    /// Returns a const_reverse_iterator pointing to the beginning of the 
    /// spline. (one before the first KeyFrame)
    TS_API
    const_reverse_iterator rend() const {
        return const_reverse_iterator(GetKeyFrames().rend());
    }


    /// Returns a const_iterator to the KeyFrame at time \p t. This
    /// will return end() if no KeyFrame exists at that time.
    TS_API
    const_iterator find(const TsTime &t) const;

    /// Returns a const_iterator to the first KeyFrame with a time
    /// that is not less than \p t.
    TS_API
    const_iterator lower_bound(const TsTime &t) const;

    /// Returns a const_iterator to the first KeyFrame with a time
    /// that is greater than \p t.
    TS_API
    const_iterator upper_bound(const TsTime &t) const;

    /// Returns the number (either 0 or 1) of KeyFrames with time
    /// \p t.
    TS_API
    size_t count(const TsTime &t) const {
        return GetKeyFrames().find(t) != GetKeyFrames().end();
    }

    /// @}

private:

    void _BreakdownMultipleValues( const std::vector<double> &times,
       TsKnotType type, bool flatTangents, double tangentLength,
       const std::vector<VtValue> &values,
       GfInterval *intervalAffected,
       TsKeyFrameMap *keyFramesAtTimes);

    void _BreakdownMultipleKnotTypes( const std::vector<double> &times,
       const std::vector<TsKnotType> &types,
       bool flatTangents, double tangentLength,
       const std::vector<VtValue> &values,
       GfInterval *intervalAffected,
       TsKeyFrameMap *keyFramesAtTimes);

    // Fills \p keyframes with the new keyframes to effect a breakdown
    // at \p x.  Subclasses can use this to implement \c Breakdown();
    // they'll call this then set each key frame in \p keyframes.
    void
    _GetBreakdown( TsKeyFrameMap* newKeyframes, double x, TsKnotType type,
                   bool flatTangents, double tangentLength,
                   const VtValue &value ) const;

    typedef std::vector<std::pair<TsTime, VtValue>> _Samples;

    // Helper for _BreakdownMultipleKnotTypes. Performs the Breakdown on the 
    // given list of samples, i.e. time/value pairs.
    void _BreakdownSamples(
        const _Samples &samples,
        TsKnotType type, 
        bool flatTangents,
        double tangentLength,
        GfInterval *intervalAffected,
        TsKeyFrameMap *keyFramesAtTimes);

    // Helper for the forms of IsVarying*; allows subseqent keyframes to vary
    // by 'tolerance'.
    bool _IsVarying(double tolerance) const;

    /// Ensure that _data is not shared with any other spline.
    /// If it is, make our own copy and drop our reference to the shared one.
    void _Detach();

private:
    std::shared_ptr<TsSpline_KeyFrames> _data;
};

TS_API
std::ostream& operator<<(std::ostream &out, const TsSpline &val);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
