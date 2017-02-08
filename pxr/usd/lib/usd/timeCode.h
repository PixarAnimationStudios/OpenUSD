//
// Copyright 2016 Pixar
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
#ifndef USD_TIMECODE_H
#define USD_TIMECODE_H

#include "pxr/pxr.h"
#include "pxr/base/arch/hints.h"

#include <boost/functional/hash.hpp>

#include <limits>
#include <iosfwd>
#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdTimeCode
///
/// Represent a time value, which may be either numeric, holding a double
/// value, or a sentinel value UsdTimeCode::Default().
///
/// A UsdTimeCode does \em not represent an 
/// <a href="https://en.wikipedia.org/wiki/SMPTE_timecode">SMPTE timecode</a>,
/// although we may, in future, support conversion functions between the two.
/// Instead, UsdTimeCode is an abstraction that acknowledges that in the 
/// principal domains of use for USD, there are many different ways of encoding 
/// time, and USD must be able to capture and translate between all of them for
/// interchange, retaining as much intent of the authoring application as 
/// possible.
/// 
/// A UsdTimeCode is therefore a unitless, generic time measurement that serves 
/// as the ordinate for time-sampled data in USD files.  A client of USD relies 
/// on the UsdStage (which in turn consults metadata authored in its root layer)
/// to define the mapping of TimeCodes to units like seconds and frames. 
/// 
/// \sa UsdStage::GetStartTimeCode()
/// \sa UsdStage::GetEndTimeCode()
/// \sa UsdStage::GetTimeCodesPerSecond()
/// \sa UsdStage::GetFramesPerSecond()
/// 
/// As described in \ref Usd_ValueResolution , USD optionally provides an
/// unvarying, 'default' value for every attribute.  UsdTimeCode embodies a time
/// value that can either be a floating-point sample time, or the default.
///
/// All UsdAttribute and derived API that requires a time parameter defaults
/// to UsdTimeCode::Default() if the parameter is left unspecified, and 
/// auto-constructs from a floating-point argument.
///
/// UsdTimeCode::EarliestTime() is provided to aid clients who wish
/// to retrieve the first authored timesample for any attribute.
///
class UsdTimeCode {
public:
    /// Construct with optional time value.  Impilicitly convert from double.
    constexpr UsdTimeCode(double t = 0.0) : _value(t) {}

    /// Produce a UsdTimeCode representing the lowest/earliest possible
    /// timeCode.  Thus, for any given timeSample \em s, its time ordinate 
    /// \em t will obey: t >= UsdTimeCode::EarliestTime()
    ///
    /// This is useful for clients that wish to retrieve the first authored 
    /// timeSample for an attribute, as they can use UsdTimeCode::EarliestTime()
    /// as the \em time argument to UsdAttribute::Get() and 
    /// UsdAttribute::GetBracketingTimeSamples()
    static constexpr UsdTimeCode EarliestTime() {
        return UsdTimeCode(std::numeric_limits<double>::lowest());
    }

    /// Produce a UsdTimeCode representing the sentinel value for 'default'.
    ///
    /// \note In inequality comparisons, Default() is considered less than any
    /// numeric TimeCode, including EarliestTime(), indicative of the fact that
    /// in UsdAttribute value resolution, the sample at Default() (if any) is
    /// always weaker than any numeric timeSample in the same layer.  For
    /// more information, see \ref Usd_ValueResolution
    static constexpr UsdTimeCode Default() {
        return UsdTimeCode(std::numeric_limits<double>::quiet_NaN());
    }

    /// Produce a safe step value such that for any numeric UsdTimeCode t in
    /// [-maxValue, maxValue], t +/- (step / maxCompression) != t with a safety
    /// factor of 2.  This is shorthand for
    /// std::numeric_limits<double>::epsilon() * maxValue * maxCompression *
    /// 2.0.  Such a step value is recommended for simulating jump
    /// discontinuities in time samples.  For example, author value x at time t,
    /// and value y at time t + SafeStep().  This ensures that as the sample
    /// times are shifted and scaled, t and t + SafeStep() remain distinct so
    /// long as they adhere to the \p maxValue and \p maxCompression limits.
    static constexpr double
    SafeStep(double maxValue=1e6, double maxCompression=10.0) {
        return std::numeric_limits<double>::epsilon() *
            maxValue * maxCompression * 2.0;
    }

    /// Return true if this time represents the 'default' sentinel value, false
    /// otherwise.  This is equivalent to !IsNumeric().
    bool IsDefault() const {
        return std::isnan(_value);
    }

    /// Return true if this time represents a numeric value, false otherwise.
    /// This is equivalent to !IsDefault().
    bool IsNumeric() const {
        return !IsDefault();
    }

    /// Return the numeric value for this time.  If this time \a IsDefault(),
    /// return a quiet NaN value.
    double GetValue() const {
        if (ARCH_UNLIKELY(IsDefault()))
            _IssueGetValueOnDefaultError();
        return _value;
    }

    /// Equality comparison.
    friend bool operator==(const UsdTimeCode &lhs, const UsdTimeCode& rhs) {
        return lhs.IsDefault() == rhs.IsDefault() &&
            (lhs.IsDefault() || (lhs.GetValue() == rhs.GetValue()));
    }

    /// Inequality comparison.
    friend bool operator!=(const UsdTimeCode &lhs, const UsdTimeCode& rhs) {
        return !(lhs == rhs);
    }

    /// Less-than.  Default() times are less than all numeric times,
    /// \em including EarliestTime()
    friend bool operator<(const UsdTimeCode &lhs, const UsdTimeCode &rhs) {
        return (lhs.IsDefault() && rhs.IsNumeric()) ||
            (lhs.IsNumeric() && rhs.IsNumeric() &&
             lhs.GetValue() < rhs.GetValue());
    }

    /// Greater-equal.  Default() times are less than all numeric times,
    /// \em including EarliestTime().
    friend bool operator>=(const UsdTimeCode &lhs, const UsdTimeCode &rhs) {
        return !(lhs < rhs);
    }

    /// Less-equal.  Default() times are less than all numeric times,
    /// \em including EarliestTime().
    friend bool operator<=(const UsdTimeCode &lhs, const UsdTimeCode &rhs) {
        return lhs.IsDefault() || 
            (rhs.IsNumeric() && lhs.GetValue() <= rhs.GetValue());
    }

    /// Greater-than.  Default() times are less than all numeric times,
    /// \em including EarliestTime().
    friend bool operator>(const UsdTimeCode &lhs, const UsdTimeCode &rhs) {
        return !(lhs <= rhs);
    }

    /// Hash function.
    friend size_t hash_value(const UsdTimeCode &time) {
        return boost::hash_value(time._value);
    }

private:
    void _IssueGetValueOnDefaultError() const;

    double _value;
};

// Stream insertion.
std::ostream& operator<<(std::ostream& os, const UsdTimeCode& time);


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_TIMECODE_H
