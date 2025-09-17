//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_VALIDATION_USD_VALIDATION_TIME_RANGE_H
#define PXR_USD_VALIDATION_USD_VALIDATION_TIME_RANGE_H

/// \file

#include "pxr/pxr.h"
#include "pxr/base/gf/interval.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usdValidation/usdValidation/api.h"
#include "pxr/usdValidation/usdValidation/timeRange.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdValidationTimeRange
///
/// A class which describes the interval at which validation is to be performed.
///
/// Clients can validate using a GfInterval, which can also encode a closed
/// single time code as well. 
/// The interval can be passed to UsdAttribute::GetTimeSamplesInInterval() to 
/// retrieve relevant samples. Time dependent attributes can then be validated 
/// at each time sample in the validation callback itself by the clients.
///
/// UsdValidationTimeRange can also specify if UsdTimeCode::Default() should be
/// included in the validation interval or not.
///
/// To validate for all time samples, clients can provide an interval using
/// GfInterval::GetFullInterval().
///
/// \sa UsdAttribute::GetTimeSamplesInInterval
class UsdValidationTimeRange
{
    GfInterval _interval;
    bool _includeTimeCodeDefault;

public:

    /// Construct an UsdValidationTimeRange, which signifies
    /// validate full time interval, including UsdTimeCode::Default().
    USDVALIDATION_API
    UsdValidationTimeRange();

    /// Construct a UsdValidationTimeRange with a specific \p timeCode.
    ///
    /// If \p timeCode is UsdTimeCode::Default(), the time range will be
    /// empty, i.e. no time to validate, but _includeTimeCodeDefault will be
    /// true.
    USDVALIDATION_API
    explicit UsdValidationTimeRange(const UsdTimeCode &timeCode);

    /// Construct a UsdValidationTimeRange with a specific \p interval.
    ///
    /// \p includeTimeCodeDefault is used to determine if UsdTimeCode::Default()
    /// should be included in the UsdValidationTimeRange or not 
    /// (default is true).
    USDVALIDATION_API
    explicit UsdValidationTimeRange(const GfInterval &interval,
                                    bool includeTimeCodeDefault = true);

    /// Return true if the time range includes UsdTimeCode::Default().
    USDVALIDATION_API
    bool IncludesTimeCodeDefault() const;

    /// Return the interval to validate.
    USDVALIDATION_API
    GfInterval GetInterval() const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_VALIDATION_USD_VALIDATION_TIME_RANGE_H
