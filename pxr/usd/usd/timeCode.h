//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_TIME_CODE_H
#define PXR_USD_USD_TIME_CODE_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/sdf/timeCode.h"
#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/hash.h"

#include <limits>
#include <iosfwd>
#include <cmath>


PXR_NAMESPACE_OPEN_SCOPE


#define USD_TIME_CODE_TOKENS \
    (DEFAULT) \
    (EARLIEST) \
    (PRE_TIME)

TF_DECLARE_PUBLIC_TOKENS(UsdTimeCodeTokens, USD_API, USD_TIME_CODE_TOKENS);


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
/// A UsdTimeCode can also represent a 'pre-time' value, which means the limit 
/// as time approaches the value from the left. Refer UsdAttribute::Get() for
/// details on usage of UsdTimeCode::PreTime().
///
class UsdTimeCode {
public:
    /// Construct with optional time value.  Impilicitly convert from double.
    constexpr UsdTimeCode(double t = 0.0) noexcept : _value(t) {}

    /// Construct and implicitly cast from SdfTimeCode.
    constexpr UsdTimeCode(const SdfTimeCode &sdfTimeCode) noexcept 
        : _value(sdfTimeCode.GetValue()) {}

    /// Produces a UsdTimeCode representing a pre-time at \p t.
    ///
    /// \sa UsdAttribute::Get()
    static constexpr UsdTimeCode PreTime(double t) noexcept {
        return UsdTimeCode(t, /*isPreTime=*/true);
    }

    /// Produces a UsdTimeCode representing a pre-time using SdfTimeCode \p 
    /// timeCode.
    static constexpr UsdTimeCode PreTime(const SdfTimeCode& timeCode) noexcept {
        return UsdTimeCode(timeCode.GetValue(), /*isPreTime=*/true);
    }

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

    /// Return true if this timeCode represents a pre-value, false otherwise.
    bool IsPreTime() const {
        return _isPreTime;
    }

    /// Return true if this time represents the lowest/earliest possible
    /// timeCode, false otherwise.
    bool IsEarliestTime() const {
        return IsNumeric() && (_value == std::numeric_limits<double>::lowest());
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
        if (lhs.IsDefault() && rhs.IsDefault()) {
            return true;
        }
        return lhs._value == rhs._value && 
               lhs._isPreTime == rhs._isPreTime;
    }

    /// Inequality comparison.
    friend bool operator!=(const UsdTimeCode &lhs, const UsdTimeCode& rhs) {
        return !(lhs == rhs);
    }

    /// Less-than.  
    ///
    /// Default() times are less than all numeric times,
    /// Numeric times are ordered by their value,
    /// If numeric times are equal, pre-time times are less than non pre-time 
    /// times.
    /// \em including EarliestTime()
    friend bool operator<(const UsdTimeCode &lhs, const UsdTimeCode &rhs) {
        if (lhs.IsDefault() || rhs.IsDefault()) {
            return lhs.IsDefault() && !rhs.IsDefault();
        }
        return lhs._value < rhs._value ||
               (lhs._value == rhs._value && 
                lhs._isPreTime && !rhs._isPreTime);
    }

    /// Greater-equal.  Default() times are less than all numeric times,
    /// \em including EarliestTime().
    friend bool operator>=(const UsdTimeCode &lhs, const UsdTimeCode &rhs) {
        return !(lhs < rhs);
    }

    /// Less-equal.  
    friend bool operator<=(const UsdTimeCode &lhs, const UsdTimeCode &rhs) {
        return !(rhs < lhs);
    }

    /// Greater-than.  Default() times are less than all numeric times,
    /// \em including EarliestTime().
    friend bool operator>(const UsdTimeCode &lhs, const UsdTimeCode &rhs) {
        return !(lhs <= rhs);
    }

    /// Hash function.
    friend size_t hash_value(const UsdTimeCode &time) {
        return TfHash::Combine(time._value, time._isPreTime);
    }

private:
    constexpr UsdTimeCode(double t, bool isPreTime) noexcept 
        : _value(t), _isPreTime(isPreTime) {}

    USD_API
    void _IssueGetValueOnDefaultError() const;

    double _value;
    bool _isPreTime = false;
};

// Stream I/O operators.
USD_API
std::ostream& operator<<(std::ostream& os, const UsdTimeCode& time);

USD_API
std::istream& operator>>(std::istream& is, UsdTimeCode& time);


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_TIME_CODE_H
