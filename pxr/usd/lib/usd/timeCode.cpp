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

