//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_AR_TIMESTAMP_H
#define PXR_USD_AR_TIMESTAMP_H

/// \file ar/timestamp.h

#include "pxr/pxr.h"
#include "pxr/usd/ar/api.h"

#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/hash.h"

#include <limits>

PXR_NAMESPACE_OPEN_SCOPE

/// \class ArTimestamp
/// Represents a timestamp for an asset. Timestamps are represented by
/// Unix time, the number of seconds elapsed since 00:00:00 UTC 1/1/1970.
class ArTimestamp
{
public:
    /// Create an invalid timestamp.
    ArTimestamp() 
        : _time(std::numeric_limits<double>::quiet_NaN())
    {
    }

    /// Create a timestamp at \p time, which must be a Unix time value.
    explicit ArTimestamp(double time)
        : _time(time)
    {
    }

    /// Return true if this timestamp is valid, false otherwise.
    bool IsValid() const
    {
        return !std::isnan(_time);
    }

    /// Return the time represented by this timestamp as a double.
    /// If this timestamp is invalid, issue a coding error and
    /// return a quiet NaN value.
    double GetTime() const
    {
        if (ARCH_UNLIKELY(!IsValid())) {
            _IssueInvalidGetTimeError();
        }
        return _time;
    }

    /// Comparison operators
    /// Note that invalid timestamps are considered less than all
    /// other timestamps.
    /// @{

    friend bool operator==(const ArTimestamp& lhs, const ArTimestamp& rhs)
    {
        return (!lhs.IsValid() && !rhs.IsValid()) ||
            (lhs.IsValid() && rhs.IsValid() && lhs._time == rhs._time);
    }

    friend bool operator!=(const ArTimestamp& lhs, const ArTimestamp& rhs)
    {
        return !(lhs == rhs);
    }

    friend bool operator<(const ArTimestamp& lhs, const ArTimestamp& rhs)
    {
        return (!lhs.IsValid() && rhs.IsValid()) ||
            (lhs.IsValid() && rhs.IsValid() && lhs._time < rhs._time);
    }

    friend bool operator>=(const ArTimestamp& lhs, const ArTimestamp& rhs)
    {
        return !(lhs < rhs);
    }

    friend bool operator<=(const ArTimestamp& lhs, const ArTimestamp& rhs)
    {
        return !lhs.IsValid() || (rhs.IsValid() && lhs._time <= rhs._time);
    }

    friend bool operator>(const ArTimestamp& lhs, const ArTimestamp& rhs)
    {
        return !(lhs <= rhs);
    }

    /// @}

private:
    AR_API
    void _IssueInvalidGetTimeError() const;

    // TfHash support.
    template <class HashState>
    friend void TfHashAppend(HashState& h, const ArTimestamp& t)
    {
        h.Append(t._time);
    }

    double _time;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
