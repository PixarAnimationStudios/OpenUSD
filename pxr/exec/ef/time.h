//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EF_TIME_H
#define PXR_EXEC_EF_TIME_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/ef/api.h"

#include "pxr/usd/usd/timeCode.h"

#include <cstdint>
#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
///
/// A class that represents a point in time for execution.
///
/// Time has a frame value that can be double-valued or set to "default". The
/// "default" frame can be thought to represent a point that is not on the
/// timeline.
///
/// EfTime also contains spline evaluation flags that cause splines to be
/// evaluated in application-specific, special ways. These flags should
/// be consumed when spline evaluation is dispatched to the application-level
/// evaluation logic.
///
class EfTime
{
public:
    /// Data type for storing app-specific spline evaluation flags.
    ///
    using SplineEvaluationFlags = uint8_t;

    /// A default constructed EfTime is set to the default frame value.
    ///
    EfTime() :
        _timeCode(UsdTimeCode::Default()),
        _splineFlags(0)
    {}

    /// Constructs an EfTime object for a specific frame with an optional
    /// evaluation location and set of spline flags.
    ///
    explicit EfTime(
        const UsdTimeCode timeCode,
        SplineEvaluationFlags splineFlags = 0) :
        _timeCode(timeCode),
        _splineFlags(splineFlags)
    {}


    /// \name Time code
    /// @{

    /// Returns the time code.
    ///
    const UsdTimeCode GetTimeCode() const {
        return _timeCode;
    }

    /// Sets the time code to \p timeCode.
    ///
    void SetTimeCode(const UsdTimeCode timeCode) {
        _timeCode = timeCode;
    }

    /// @}


    /// \name Spline evaluation flags
    /// @{

    /// Returns the spline evaluation flags that will be used during evaluation.
    ///
    SplineEvaluationFlags GetSplineEvaluationFlags() const {
        return _splineFlags;
    }

    /// Sets the spline evaluation flags that will be used during evaluation.
    ///
    void SetSplineEvaluationFlags(SplineEvaluationFlags flags) {
        _splineFlags = flags;
    }

    /// @}


    /// Returns \c true if *this == rhs.
    ///
    bool operator==(const EfTime &rhs) const {
        if (!_timeCode.IsDefault() && !rhs._timeCode.IsDefault()) {
            return _timeCode == rhs._timeCode
                && _splineFlags == rhs._splineFlags;
        }
        return _timeCode.IsDefault() == rhs._timeCode.IsDefault();
    }

    bool operator!=(const EfTime &rhs) const {
        return !(*this == rhs);
    }

    /// Returns \c true if *this < rhs.
    ///
    /// Note that a time with frame set to "default" is lesser than all
    /// non-default times.
    ///
    /// Also note that evaluation location and spline flags have no effect on
    /// ordering for the default frame. Spline flags are only used for stable
    /// ordering, but there is no logical ordering between two sets of spline
    /// flags.
    ///
    bool operator<(const EfTime &rhs) const {
        if (_timeCode.IsDefault() || rhs._timeCode.IsDefault()) {
            return _timeCode < rhs._timeCode;
        }

        return _timeCode < rhs._timeCode ||
            (_timeCode == rhs._timeCode && _splineFlags < rhs._splineFlags);
    }

    bool operator<=(const EfTime &rhs) const {
        return !(rhs < *this);
    }

    bool operator>(const EfTime &rhs) const {
        return rhs < *this;
    }

    bool operator>=(const EfTime &rhs) const {
        return !(*this < rhs);
    }

    /// Provides a hash function for EfTime.
    ///
    template <class HashState>
    friend void TfHashAppend(HashState &h, const EfTime &t) {
        h.Append(t._timeCode);
        if (!t._timeCode.IsDefault()) {
            h.Append(t._splineFlags);
        }
    }

    /// Returns this object as string. Note that evaluation location will only
    /// be denoted in the output string if the time code IsPreTime().
    ///
    EF_API
    std::string GetAsString() const;

private:

    // The time code value.
    UsdTimeCode _timeCode;

    // The spline evaluation flags to use during computation.
    SplineEvaluationFlags _splineFlags;
};

/// Output an EfTime.
///
EF_API
std::ostream &operator<<(std::ostream &os, const EfTime &time);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
