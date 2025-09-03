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
#include "pxr/base/tf/stringUtils.h"
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
    } else {
        if (time.IsPreTime()) {
            os << UsdTimeCodeTokens->PRE_TIME << ' ';
        }
        if (time.IsEarliestTime()) {
            os << UsdTimeCodeTokens->EARLIEST;
        } else {
            os << time.GetValue();
        }
    }

    return os;
}

std::istream& operator>>(std::istream& is, UsdTimeCode& time)
{
    std::string valueString;
    is >> valueString;

    bool isPreTime = false;
    // Check if the token is PRE_TIME
    if (valueString == UsdTimeCodeTokens->PRE_TIME.GetString()) {
        isPreTime = true;
        // read in the next token (the time value)
        is >> valueString;
    }

    const TfToken valueToken(valueString);

    if (valueToken == UsdTimeCodeTokens->DEFAULT) {
        if (isPreTime) {
            is.setstate(std::ios::failbit);
            return is;
        }
        time = UsdTimeCode::Default();
    } else if (valueToken == UsdTimeCodeTokens->EARLIEST) {
        time = isPreTime ? 
            UsdTimeCode::PreTime(UsdTimeCode::EarliestTime().GetValue()) : 
            UsdTimeCode::EarliestTime();
    } else {
        try {
            size_t pos = 0;
            double value = valueString.empty() ? 
                0.0 : std::stod(valueString, &pos);
            if (pos != valueString.size()) {
                is.setstate(std::ios::failbit);
                return is;
            }
            time = isPreTime ? 
                UsdTimeCode::PreTime(value) : 
                UsdTimeCode(value);
        } catch (const std::exception&) {
            is.setstate(std::ios::failbit);
        }
    }

    return is;
}


PXR_NAMESPACE_CLOSE_SCOPE

