//
// Copyright 2023 Pixar
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

#ifndef PXR_BASE_TS_KEY_FRAME_H
#define PXR_BASE_TS_KEY_FRAME_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/ts/data.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/traits.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"

#include <iostream>
#include <typeinfo>

PXR_NAMESPACE_OPEN_SCOPE

class TsSpline;

/// \class TsKeyFrame 
/// \brief Specifies the value of an TsSpline object at a particular 
/// point in time.
///
/// Keyframes also specify the shape of a spline as it passes through each
/// keyframe: the knot type specifies what interpolation technique to use
/// (TsKnotHeld, TsKnotLinear, or TsKnotBezier), and tangent handles 
/// specify the shape of the spline as it passes through the keyframe.
///
/// It is also possible for keyframes to be "dual-valued." This means
/// that a separate keyframe value -- the left-side value -- is used
/// when approaching the keyframe from lower time values. The regular
/// value is then used starting at the keyframe's time and when
/// approaching that time from higher times to the right.  Dual-value
/// knots are necessary to compensate for instantaneous shifts
/// in coordinate frames, such as the shift that occurs when there is a 
/// constraint switch.  The spline can snap to the new value required to 
/// maintain the same position in worldspace.
///
/// <b>Note:</b> TsKeyFrame is a value, not a formal object.
///
class TsKeyFrame final
{
public: // methods

    /// Constructs a default double keyframe.
    TS_API
    TsKeyFrame();

    /// \name Constructors
    /// @{
    ///
    /// There are four variations on the constructor, to support
    /// single-valued or dual-valued keyframes, with values supplied as
    /// either VtValues or a template parameter.
    
    /// Constructs a single-valued keyframe.
    template <typename T>
    TsKeyFrame( const TsTime & time,
                  const T & val,
                  TsKnotType knotType = TsKnotLinear,
                  const T & leftTangentSlope = TsTraits<T>::zero,
                  const T & rightTangentSlope = TsTraits<T>::zero,
                  TsTime leftTangentLength = 0,
                  TsTime rightTangentLength = 0);

    /// Constructs a single-valued keyframe with VtValues.
    TS_API
    TsKeyFrame( const TsTime & time,
                  const VtValue & val,
                  TsKnotType knotType = TsKnotLinear,
                  const VtValue & leftTangentSlope = VtValue(),
                  const VtValue & rightTangentSlope = VtValue(),
                  TsTime leftTangentLength = 0,
                  TsTime rightTangentLength = 0);

    /// Constructs a dual-valued keyframe.
    template <typename T>
    TsKeyFrame( const TsTime & time,
                  const T & lhv,
                  const T & rhv,
                  TsKnotType knotType = TsKnotLinear,
                  const T & leftTangentSlope = TsTraits<T>::zero,
                  const T & rightTangentSlope = TsTraits<T>::zero,
                  TsTime leftTangentLength = 0,
                  TsTime rightTangentLength = 0);

    /// Constructs a dual-valued keyframe with VtValues.
    TS_API
    TsKeyFrame( const TsTime & time,
                  const VtValue & lhv,
                  const VtValue & rhv,
                  TsKnotType knotType = TsKnotLinear,
                  const VtValue & leftTangentSlope = VtValue(),
                  const VtValue & rightTangentSlope = VtValue(),
                  TsTime leftTangentLength = 0,
                  TsTime rightTangentLength = 0);

    /// Constructs a keyframe by duplicating an existing TsKeyFrame.
    TS_API
    TsKeyFrame( const TsKeyFrame & kf );

    /// @}

    /// Non-virtual destructor; this class should not be subclassed.
    TS_API
    ~TsKeyFrame();

    /// \name Primary API
    /// @{

    /// Assignment operator.
    TS_API
    TsKeyFrame & operator=(const TsKeyFrame &rhs);

    /// Compare this keyframe with another.
    TS_API
    bool operator==(const TsKeyFrame &) const;

    TS_API
    bool operator!=(const TsKeyFrame &) const;

    /// Gets whether this key frame is at the same time and is equivalent to 
    /// \p keyFrame on the given \p side.  In other words, replacing this
    /// key frame with \p keyFrame in a spline will have no effect on how the 
    /// spline evaluates for any time on the given \p side of this key frame.
    TS_API
    bool IsEquivalentAtSide(const TsKeyFrame &keyFrame, TsSide side) const;

    /// Gets the time of this keyframe.
    TS_API
    TsTime GetTime() const {
        return _holder.Get()->GetTime();
    }

    /// Sets the time of this keyframe.
    TS_API
    void SetTime( const TsTime & newTime ) {
        _holder.GetMutable()->SetTime(newTime);
    }

    /// Gets the value at this keyframe.
    TS_API
    VtValue GetValue() const;

    /// Sets the value at this keyframe.
    TS_API
    void SetValue( VtValue val );

    /// Gets the value at this keyframe on the given side.
    TS_API
    VtValue GetValue( TsSide side ) const;

    /// Sets the value at this keyframe on the given side.
    TS_API
    void SetValue( VtValue val, TsSide side );

    /// Gets the value of the derivative at this keyframe.
    TS_API
    VtValue GetValueDerivative() const;

    /// Gets a zero for this keyframe's value type.
    TS_API
    VtValue GetZero() const;

    /// Gets the knot type
    TS_API
    TsKnotType GetKnotType() const;

    /// Sets the knot type
    TS_API
    void SetKnotType( TsKnotType knotType );

    /// Checks whether the key frame's value type supports the given knot
    /// type.
    TS_API
    bool CanSetKnotType( TsKnotType, std::string *reason=NULL ) const;

    /// @}

    /// \name Dual-value API
    ///
    /// Keyframes have a "left side" and a "right side".  The right side is
    /// conceptually later than the left, even though they occur at the same
    /// time.  The two sides most often have the same value, but it is also
    /// possible for the two sides to have different values.  The purpose of
    /// having different values on the two sides is to allow instantaneous value
    /// discontinuities.  This is useful, for example, when a constraint
    /// changes, and the meaning of another property (like an IkTx)
    /// instantaneously changes because of the constraint switch.
    ///
    /// Most spline evaluation takes place on the right side.  Calling GetValue
    /// returns the sole value for a single-valued keyframe, and the right value
    /// for a double-valued keyframe.
    ///
    /// Note the difference between, on the one hand, asking a keyframe for its
    /// left value; and on the other hand, evaluating a spline at the left side
    /// of that keyframe's time.  Usually these two methods agree.  But when a
    /// keyframe is preceded by a held segment, spline evaluation at the
    /// keyframe's left side will yield the held value from the prior segment,
    /// but the keyframe itself knows nothing about the prior segment, so
    /// GetLeftValue returns the left value stored in the keyframe (which is the
    /// right value for a single-valued knot).  Another way to look at this
    /// situation is that, when a keyframe B is preceded by a held keyframe A,
    /// the left value of B is never consulted in spline evaluation.  This
    /// arrangement ensures that the instantaneous value change at the end of a
    /// held segment occurs exactly at the time of the keyframe that ends the
    /// segment.
    ///
    /// Note also the difference between GetIsDualValued and
    /// TsSpline::DoSidesDiffer.  Usually these two methods agree.  But in the
    /// after-held-knot case described above, they do not.  They also do not
    /// agree when SetIsDualValued(true) has been called, but the keyframe has
    /// the same value on both sides.
    ///
    /// @{

    /// Gets whether this knot is dual-valued.  See the note above about
    /// TsSpline::DoSidesDiffer.
    TS_API
    bool GetIsDualValued() const;

    /// Sets whether this knot is dual-valued.  When a knot is first made
    /// dual-valued, the left value is copied from the right value.
    TS_API
    void SetIsDualValued( bool isDual );

    /// Gets the left value of this dual-valued knot.  Returns the right value
    /// if this is not a dual-valued knot.
    TS_API
    VtValue GetLeftValue() const;

    /// Sets the left value of this dual-valued knot.  It is an error to call
    /// this method on single-valued knots.
    TS_API
    void SetLeftValue( VtValue val );

    /// Gets the value of the derivative on the left side.  This is a synonym
    /// for GetLeftTangentSlope for knot types that support tangents; for other
    /// types, this method returns zero.
    TS_API
    VtValue GetLeftValueDerivative() const;

    /// @}

    /// \name Tangents
    /// @{

    /// Gets whether the value type of this keyframe is interpolatable.
    TS_API
    bool IsInterpolatable() const;

    /// Gets whether the value type of this keyframe supports tangents.  This
    /// will return true not only for Bezier, but also for Linear and Held,
    /// because when authors switch from Bezier to Linear/Held and back to
    /// Bezier, we want to preserve the original tangents, and thus we track
    /// tangent data for Linear and Held knots.  If you really want to write
    /// just to Beziers, call HasTangents().
    TS_API
    bool SupportsTangents() const;

    /// Gets whether the knot of this keyframe has tangents.  This is true when
    /// the value type supports tangents, and the knot is a Bezier.
    TS_API
    bool HasTangents() const;

    /// Gets the length of the projection of the knot's left tangent onto the
    /// time axis.
    TS_API
    TsTime GetLeftTangentLength() const;

    /// Gets the left-side tangent slope (in units per frame) of this knot.
    TS_API
    VtValue GetLeftTangentSlope() const;

    /// Gets the length of the projection of the knot's right tangent onto the
    /// time axis.
    TS_API
    TsTime GetRightTangentLength() const;

    /// Gets the right-side tangent slope (in units per frame) of this knot.
    TS_API
    VtValue GetRightTangentSlope() const;

    /// Sets the left-side tangent length (in time) of this knot.  Issues a
    /// coding error if this knot does not support tangents
    TS_API
    void SetLeftTangentLength( TsTime );

    /// Sets the left-side tangent slope (in units per frame) of this knot.
    /// Issues a coding error if this knot does not support tangents
    TS_API
    void SetLeftTangentSlope( VtValue );

    /// Sets the right-side tangent length (in time) of this knot.
    /// Issues a coding error if this knot does not support tangents
    TS_API
    void SetRightTangentLength( TsTime );

    /// Sets the right-side tangent slope (in units per frame) of this knot.
    /// Issues a coding error if this knot does not support tangents
    TS_API
    void SetRightTangentSlope( VtValue newSlope);

    /// Gets whether tangent symmetry has been broken.  In this context,
    /// "symmetric" refers to the tangents having equal slope but not
    /// necessarily equal length.
    ///
    /// If tangent symmetry is broken, tangent handles will not 
    /// automatically stay symmetric as they are changed.
    TS_API
    bool GetTangentSymmetryBroken() const;

    /// Sets whether tangent symmetry is broken.  Setting this to false
    /// will make the tangents symmetric if they are not already by
    /// reflecting the right tangent to the left side. Issues a
    /// coding error if this knot does not support tangents
    TS_API
    void SetTangentSymmetryBroken( bool broken );

    /// Sets the flag that enforces tangent symmetry based on whether
    /// the tangets are already symmetric. If they are symmetric, the
    /// 'broken' flag will be cleared so that future edits maintain
    /// symmetry.  If they are not symmetric, they will be marked as
    /// 'broken'.
    ///
    /// The intent is to help provide policy for newly received
    /// tangent data: if the tangents happen to be symmetric, keep them
    /// so; but if they are asymmetric, don't bother. Issues a
    /// coding error if this knot does not support tangents.
    TS_API
    void ResetTangentSymmetryBroken();

private:

    // Give the rest of the library access to the Ts_Data object held
    // in this keyframe through the Ts_GetKeyFrameData function.
    friend Ts_Data* Ts_GetKeyFrameData(TsKeyFrame &kf);
    friend Ts_Data const* Ts_GetKeyFrameData(TsKeyFrame const& kf);

    // Shared initialization
    void _Initialize(
        const TsTime & time,
        TsKnotType knotType,
        const VtValue & leftTangentSlope,
        const VtValue & rightTangentSlope,
        TsTime leftTangentLength,
        TsTime rightTangentLength);

    // XXX: exported because called from inlined templated constructors
    TS_API
    void _InitializeKnotType(TsKnotType knotType);
    TS_API
    void _InitializeTangentLength(TsTime leftTangentLength,
        TsTime rightTangentLength);

    // Helper function which tests the setability of tangents for this knot,
    // and reports an error if tangents not supported
    bool _ValidateTangentSetting() const;

private:

    Ts_PolymorphicDataHolder _holder;
};

////////////////////////////////////////////////////////////////////////

TS_API
std::ostream& operator<<(std::ostream &out, const TsKeyFrame &val);

template <typename T>
TsKeyFrame::TsKeyFrame( const TsTime & time,
                            const T & val,
                            TsKnotType knotType,
                            const T & leftTangentSlope,
                            const T & rightTangentSlope,
                            TsTime leftTangentLength,
                            TsTime rightTangentLength)
{
    static_assert( TsTraits<T>::isSupportedSplineValueType );

    _holder.New(time, false /*isDual*/,
        val, val, leftTangentSlope, rightTangentSlope);

    _InitializeKnotType(knotType);
    _InitializeTangentLength(leftTangentLength,rightTangentLength);
}

template <typename T>
TsKeyFrame::TsKeyFrame( const TsTime & time,
                            const T & lhv,
                            const T & rhv,
                            TsKnotType knotType,
                            const T & leftTangentSlope,
                            const T & rightTangentSlope,
                            TsTime leftTangentLength,
                            TsTime rightTangentLength)
{
    static_assert( TsTraits<T>::isSupportedSplineValueType );

    _holder.New(time, true /*isDual*/, lhv, rhv,
        leftTangentSlope, rightTangentSlope);
    
    _InitializeKnotType(knotType);
    _InitializeTangentLength(leftTangentLength,rightTangentLength);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
