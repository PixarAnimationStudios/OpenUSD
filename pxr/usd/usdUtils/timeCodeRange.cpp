//
// Copyright 2019 Pixar
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
#include "pxr/usd/usdUtils/timeCodeRange.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/usd/timeCode.h"

#include <iostream>
#include <string>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(
    UsdUtilsTimeCodeRangeTokens,
    USDUTILS_TIME_CODE_RANGE_TOKENS);


/// Attempts to convert the string \p valueString to a double.
///
/// If the string converts successfully *and* the entire string is consumed by
/// the conversion, then the result is stored in \p value and true is returned.
/// Otherwise, \p value is left unchanged and false is returned.
static
bool
_StringToDouble(const std::string& valueString, double& value)
{
    double tmpValue = 0.0;
    size_t numCharsConverted = 0u;

    try {
        tmpValue = std::stod(valueString, &numCharsConverted);
    } catch (const std::exception& /* e */) {
        return false;
    }

    // Verify that the entire string was converted.
    if (numCharsConverted != valueString.size()) {
        return false;
    }

    value = tmpValue;
    return true;
}

/* static */
UsdUtilsTimeCodeRange
UsdUtilsTimeCodeRange::CreateFromFrameSpec(const std::string& frameSpec)
{
    if (frameSpec.empty()) {
        return UsdUtilsTimeCodeRange();
    }

    // FrameSpecs must contain at least one time code value, but check whether
    // it also contains the range separator. Issue an error and return an
    // invalid empty range if there is more than one separator.
    std::vector<std::string> frameSpecParts =
        TfStringSplit(frameSpec, UsdUtilsTimeCodeRangeTokens->RangeSeparator);
    if (frameSpecParts.size() > 2u) {
        TF_CODING_ERROR("Invalid FrameSpec: \"%s\"", frameSpec.c_str());
        return UsdUtilsTimeCodeRange();
    }

    double startTimeCode = 0.0;
    if (!_StringToDouble(frameSpecParts[0], startTimeCode)) {
        TF_CODING_ERROR("Invalid FrameSpec: \"%s\"", frameSpec.c_str());
        return UsdUtilsTimeCodeRange();
    }

    // If the FrameSpec did not contain the range separator, we're done.
    if (frameSpecParts.size() == 1u) {
        return UsdUtilsTimeCodeRange(startTimeCode);
    }

    // Check whether the FrameSpec contains the stride separator. Issue an
    // error and return an invalid empty range if there is more than one
    // separator.
    frameSpecParts =
        TfStringSplit(
            frameSpecParts[1],
            UsdUtilsTimeCodeRangeTokens->StrideSeparator);
    if (frameSpecParts.size() > 2u) {
        TF_CODING_ERROR("Invalid FrameSpec: \"%s\"", frameSpec.c_str());
        return UsdUtilsTimeCodeRange();
    }

    double endTimeCode = startTimeCode;
    if (!_StringToDouble(frameSpecParts[0], endTimeCode)) {
        TF_CODING_ERROR("Invalid FrameSpec: \"%s\"", frameSpec.c_str());
        return UsdUtilsTimeCodeRange();
    }

    double stride = 1.0;
    if (frameSpecParts.size() > 1u) {
        if (!_StringToDouble(frameSpecParts[1], stride)) {
            TF_CODING_ERROR("Invalid FrameSpec: \"%s\"", frameSpec.c_str());
            return UsdUtilsTimeCodeRange();
        }
    } else {
        if (endTimeCode < startTimeCode) {
            stride = -1.0;
        }
    }

    return UsdUtilsTimeCodeRange(startTimeCode, endTimeCode, stride);
}

std::ostream&
operator<<(std::ostream& os, const UsdUtilsTimeCodeRange& timeCodeRange)
{
    if (timeCodeRange.empty()) {
        os << UsdUtilsTimeCodeRangeTokens->EmptyTimeCodeRange;
        return os;
    }

    const UsdTimeCode startTimeCode = timeCodeRange.GetStartTimeCode();
    const UsdTimeCode endTimeCode = timeCodeRange.GetEndTimeCode();
    const double stride = timeCodeRange.GetStride();

    os << startTimeCode;

    if (endTimeCode != startTimeCode) {
        os << UsdUtilsTimeCodeRangeTokens->RangeSeparator << endTimeCode;
    }

    if (stride != 1.0 && stride != -1.0) {
        os << UsdUtilsTimeCodeRangeTokens->StrideSeparator << stride;
    }

    return os;
}

std::istream&
operator>>(std::istream& is, UsdUtilsTimeCodeRange& timeCodeRange)
{
    std::string rangeString;
    is >> rangeString;
    timeCodeRange = UsdUtilsTimeCodeRange::CreateFromFrameSpec(rangeString);
    return is;
}


PXR_NAMESPACE_CLOSE_SCOPE
