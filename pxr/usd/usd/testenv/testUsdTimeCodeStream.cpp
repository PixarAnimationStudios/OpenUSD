//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/usd/timeCode.h"

#include <sstream>
#include <string>


PXR_NAMESPACE_USING_DIRECTIVE;


static
std::string
_GetStringByStreamInsertion(const UsdTimeCode timeCode)
{
    std::ostringstream ostream;
    ostream << timeCode;
    return ostream.str();
}

static
UsdTimeCode
_GetTimeCodeByStreamExtraction(
        const std::string& value,
        const UsdTimeCode inputTime)
{
    UsdTimeCode timeCode = inputTime;
    std::stringstream stream;
    stream << value;
    stream >> timeCode;
    return timeCode;
}

int
main(int argc, char* argv[])
{
    const UsdTimeCode defaultConstructedTime;
    const UsdTimeCode defaultTime = UsdTimeCode::Default();
    const UsdTimeCode earliestTime = UsdTimeCode::EarliestTime();

    const double defaultTimeValue = 123.0;
    const UsdTimeCode numericTime(defaultTimeValue);
    const std::string numericValueString =
        TfStringPrintf("%.f", defaultTimeValue);


    // Stream insertion tests.
    std::string testString = _GetStringByStreamInsertion(defaultConstructedTime);
    TF_AXIOM(testString == "0");

    testString = _GetStringByStreamInsertion(defaultTime);
    TF_AXIOM(testString == UsdTimeCodeTokens->DEFAULT.GetString());

    testString = _GetStringByStreamInsertion(earliestTime);
    TF_AXIOM(testString == UsdTimeCodeTokens->EARLIEST.GetString());

    testString = _GetStringByStreamInsertion(numericTime);
    TF_AXIOM(testString == numericValueString);


    // Stream extraction tests.
    UsdTimeCode testTime = _GetTimeCodeByStreamExtraction("0", numericTime);
    TF_AXIOM(testTime == defaultConstructedTime);

    testTime = _GetTimeCodeByStreamExtraction(
        UsdTimeCodeTokens->DEFAULT.GetString(),
        numericTime);
    TF_AXIOM(testTime == defaultTime);

    testTime = _GetTimeCodeByStreamExtraction(
        UsdTimeCodeTokens->EARLIEST.GetString(),
        numericTime);
    TF_AXIOM(testTime == earliestTime);

    testTime = _GetTimeCodeByStreamExtraction("123", defaultTime);
    TF_AXIOM(testTime == numericTime);

    // Bad data should leave the input time code unchanged.
    testTime = _GetTimeCodeByStreamExtraction("bogus", numericTime);
    TF_AXIOM(testTime == numericTime);


    return EXIT_SUCCESS;
}
