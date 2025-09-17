//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EF_TIME_INTERVAL_H
#define PXR_EXEC_EF_TIME_INTERVAL_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/ef/api.h"
#include "pxr/exec/ef/time.h"

#include "pxr/base/gf/interval.h"
#include "pxr/base/gf/multiInterval.h"

#include <initializer_list>
#include <iosfwd>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
///
/// A class that represents an interval in EfTime.
///
/// This class holds a GfMultiInterval that represents intervals on the frame
/// timeline and a bool that represents a single point, not on the timeline,
/// for the "default" time.
///
class EfTimeInterval
{
public:

    explicit EfTimeInterval(
        const GfMultiInterval &timeMultiInterval = GfMultiInterval(),
        bool defaultTime = false) :
        _timeMultiInterval(timeMultiInterval),
        _defaultTime(defaultTime)
    {}

    explicit EfTimeInterval(
        const GfInterval &timeInterval,
        bool defaultTime = false) :
        _timeMultiInterval(timeInterval),
        _defaultTime(defaultTime)
    {}

    /// For convenience, constructs a multiInterval from the discrete \p times.
    ///
    explicit EfTimeInterval(
        const std::vector<double> &times,
        bool defaultTime = false) :
        _defaultTime(defaultTime)
    {
        for(double t : times)
            _timeMultiInterval.Add(GfInterval(t));
    }

    /// For convenience, constructs a multiInterval from the discrete \p times.
    ///
    explicit EfTimeInterval(
        const std::initializer_list<double> &times,
        bool defaultTime = false) :
        _defaultTime(defaultTime)
    {
        for(double t : times)
            _timeMultiInterval.Add(GfInterval(t));
    }

    /// Clears the time interval to an empty interval.
    ///
    void Clear() { 
        _timeMultiInterval.Clear();
        _defaultTime = false;
    }

    /// Returns true if the interval is empty.
    ///
    bool IsEmpty() const {
        return !_defaultTime && _timeMultiInterval.IsEmpty();
    }
    
    /// Returns the multi interval that represents intervals on the frame
    /// timeline.
    ///
    const GfMultiInterval &GetTimeMultiInterval() const {
        return _timeMultiInterval;
    }

    /// Returns true if the interval contains the default time.
    ///
    bool IsDefaultTimeSet() const {
        return _defaultTime;
    }

    /// Returns true if this time interval contains \p time, with special
    /// treatment for the default time and for left- and right-side time
    /// values.
    ///
    /// The default time is treated as a separate time outside of the frame
    /// timeline.
    ///
    /// See EfTime::IsContainedIn() for details on how side is handled.
    /// 
    bool Contains(const EfTime &time) const {
        return time.GetTimeCode().IsDefault()
            ? _defaultTime
            : _ContainsTime(time);
    }

    /// Returns true if this time interval fully contains the time interval
    /// \p rhs.
    ///
    /// The default time is treated as a separate time outside of the frame
    /// timeline.
    ///
    bool Contains(const EfTimeInterval &rhs) const {
        if (!_defaultTime && rhs.IsDefaultTimeSet()) {
            return false;
        }
        return _timeMultiInterval.Contains(rhs.GetTimeMultiInterval());
    }

    /// Returns true if the time interval is the full interval.
    ///
    /// I.e., this returns true if the time interval contains the full frame
    /// timeline **and** the default time.
    ///
    bool IsFullInterval() const {
        return _defaultTime && 
            _timeMultiInterval == GfMultiInterval::GetFullInterval();
    }

    /// Returns the full time interval: (-inf, inf) with the default time.
    ///
    static EfTimeInterval GetFullInterval() {
        return EfTimeInterval(GfMultiInterval::GetFullInterval(), true);
    }

    bool operator==(const EfTimeInterval &rhs) const {
        return _timeMultiInterval == rhs._timeMultiInterval &&
            _defaultTime == rhs._defaultTime;
    }

    bool operator!=(const EfTimeInterval &rhs) const {
        return !(*this == rhs);
    }

    /// Unions this time interval and the EfTimeInterval \p rhs.
    ///
    void operator|=(const EfTimeInterval &rhs) {
        _timeMultiInterval.Add(rhs._timeMultiInterval);
        _defaultTime |= rhs._defaultTime;
    }

    /// Unions this time interval and the GfInterval \p interval.
    ///
    void operator|=(const GfInterval &interval) {
        _timeMultiInterval.Add(interval);
    }
    
    /// Unions this time interval and the EfTime \p time.
    ///
    void operator|=(const EfTime &time)  {
        const UsdTimeCode timeCode = time.GetTimeCode();
        if (timeCode.IsDefault()) {
            _defaultTime = true;
        } else {
            _timeMultiInterval.Add(GfInterval(timeCode.GetValue()));
        }
    }

    /// Computes the intersection of this and the EfTimeInterval \p rhs.
    ///
    void operator&=(const EfTimeInterval &rhs) {
        _timeMultiInterval.Intersect(rhs._timeMultiInterval);
        _defaultTime &= rhs._defaultTime;    
    }

    /// Less-than operator.
    bool operator<(const EfTimeInterval &rhs) const {
        if (_timeMultiInterval < rhs._timeMultiInterval) return true;
        if (_timeMultiInterval != rhs._timeMultiInterval) return false;
        if (_defaultTime != rhs._defaultTime) return _defaultTime;

        // Equal
        return false;
    }

    /// Extends the interval by the specified number of frames in each
    /// direction.
    ///
    /// E.g., extends (-100, 100) to (-110, 105) when leftFrames = 10 and
    /// rightFrames = 5; or the multi interval (-100, 100), (200, 300) becomes
    /// (-110, 105), (190, 305).
    ///
    EfTimeInterval &Extend(double leftFrames, double rightFrames) {
        const GfInterval extension( -leftFrames, +rightFrames, true, true);
	    _timeMultiInterval.ArithmeticAdd(extension);
        return *this;
    }
    
    /// Get time interval as string, for debugging.
    EF_API
    std::string GetAsString() const; 

private:

    // Returns true if this time is contained in _timeMultiInterval. If frame
    // is "default" this returns false.
    EF_API
    bool _ContainsTime(const EfTime &time) const;

    // The time multi interval.
    GfMultiInterval _timeMultiInterval;

    // Whether or not the default time is set.
    bool _defaultTime;
};

/// Output an EfTimeInterval.
///
EF_API
std::ostream &operator<<(std::ostream &os, const EfTimeInterval &timeInterval);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
