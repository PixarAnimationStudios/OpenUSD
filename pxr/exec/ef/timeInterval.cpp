//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/ef/timeInterval.h"

#include "pxr/base/tf/stringUtils.h"

#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

std::string
EfTimeInterval::GetAsString() const
{ 
    if (IsFullInterval())
        return "( full )";
    
    if (*this == EfTimeInterval())
        return "( empty )";

    return TfStringify(*this);
}

// Returns true if this time is contained in the given interval.
//
// For the default time, this returns false, as it is never contained in any
// time interval.
//
// Special care is required when the frame is on one of the interval
// boundaries, since we need to correctly handle time evaluation locations:
// E.g., 1 is *at* frame 1, but PreTime(1) is at a frame that
// is infinitesimally smaller than 1. Therefore:
// * PreTime(0) **is not** contained in (0, 1] **or** [0, 1].
// * 0 **is not** contained in (0, 1], but **is** contained in [0, 1].
// * PreTime(1) **is** contained in [0, 1) **and** [0, 1].
// * 1 **is not** contained in [0, 1), but **is** contained in [0, 1].
//
static bool _TimeIsContainedIn(const EfTime &time, const GfInterval &interval)
{
    const UsdTimeCode timeCode = time.GetTimeCode();

    if (timeCode.IsDefault() || interval.IsEmpty()) {
        return false;
    } else if (timeCode.GetValue() == interval.GetMin()) {
        return !timeCode.IsPreTime() && interval.IsMinClosed();
    } else if (timeCode.GetValue() == interval.GetMax()) {
        return timeCode.IsPreTime() || interval.IsMaxClosed();
    }

    return interval.Contains(timeCode.GetValue());
}
    
bool
EfTimeInterval::_ContainsTime(const EfTime &time) const
{
    const UsdTimeCode timeCode = time.GetTimeCode();
    if (timeCode.IsDefault() || _timeMultiInterval.IsEmpty()) {
        return false;
    }
   
    // The following code works similar to GfMultiInterval::Contains.

    // Find position of first interval >= [frame, frame].
    GfMultiInterval::const_iterator i =
        _timeMultiInterval.lower_bound(timeCode.GetValue());
    
    // Case 1: i is the first interval after d.
    if (i != _timeMultiInterval.end() && _TimeIsContainedIn(time, *i)) {
        return true;
    }

    // Case 2: the previous interval, (i-1), contains d.
    if (i != _timeMultiInterval.begin() && _TimeIsContainedIn(time, *(--i))) {
        return true;
    }
    
    return false;
}

std::ostream &
operator<<(std::ostream &os, const EfTimeInterval &timeInterval)
{
    return os << "( default=" << timeInterval.IsDefaultTimeSet()
              << " multiInterval=" << timeInterval.GetTimeMultiInterval()
              << " )";
}

PXR_NAMESPACE_CLOSE_SCOPE
