//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/timeCode.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"

#include <cstdlib>
#include <iostream>
#include <string>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(UsdTimeCodeTokens, USD_TIME_CODE_TOKENS);


void
UsdTimeCode::_IssueGetValueOnDefaultError() const
{
    // TF_CODING_ERROR("Called UsdTimeCode::GetValue() on a Default UsdTimeCode.  "
    //                 "Returning a quiet NaN.");
}

std::ostream& operator<<(std::ostream& os, const UsdTimeCode& time)
{
    if (time.IsDefault()) {
        os << UsdTimeCodeTokens->DEFAULT;
    } else if (time.IsEarliestTime()) {
        os << UsdTimeCodeTokens->EARLIEST;
    } else {
        os << time.GetValue();
    }

    return os;
}

std::istream& operator>>(std::istream& is, UsdTimeCode& time)
{
    std::string valueString;
    is >> valueString;
    const TfToken valueToken(valueString);

    if (valueToken == UsdTimeCodeTokens->DEFAULT) {
        time = UsdTimeCode::Default();
    } else if (valueToken == UsdTimeCodeTokens->EARLIEST) {
        time = UsdTimeCode::EarliestTime();
    } else {
        try {
            const double value = std::stod(valueString);
            time = UsdTimeCode(value);
        } catch (const std::exception& /* e */) {
            // Leave time unchanged on error.
        }
    }

    return is;
}


PXR_NAMESPACE_CLOSE_SCOPE

