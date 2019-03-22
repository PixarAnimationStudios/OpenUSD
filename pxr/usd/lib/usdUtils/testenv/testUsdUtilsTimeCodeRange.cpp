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

#include "pxr/base/gf/math.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdUtils/timeCodeRange.h"

#include <cassert>
#include <sstream>
#include <string>
#include <vector>


PXR_NAMESPACE_USING_DIRECTIVE;


static
bool
_ValidateIteration(
        const UsdUtilsTimeCodeRange& timeCodeRange,
        const std::vector<UsdTimeCode>& timeCodes,
        const bool useIsClose=false)
{
    std::vector<UsdTimeCode> iterTimeCodes;

    for (const UsdTimeCode& timeCode : timeCodeRange) {
        iterTimeCodes.push_back(timeCode);
    }

    if (!useIsClose) {
        return iterTimeCodes == timeCodes;
    }

    if (iterTimeCodes.size() != timeCodes.size()) {
        return false;
    } else {
        constexpr double epsilon = 1e-9;

        for (size_t i = 0u; i < iterTimeCodes.size(); ++i) {
            if (!GfIsClose(
                    iterTimeCodes[i].GetValue(),
                    timeCodes[i].GetValue(),
                    epsilon)) {
                return false;
            }
        }
    }

    return true;
}

static
std::string
_GetStringByStreamInsertion(const UsdUtilsTimeCodeRange& timeCodeRange)
{
    std::ostringstream ostream;
    ostream << timeCodeRange;
    return ostream.str();
}

static
UsdUtilsTimeCodeRange
_GetTimeCodeRangeByStreamExtraction(const std::string& rangeString)
{
    std::stringstream stream;
    stream << rangeString;

    UsdUtilsTimeCodeRange timeCodeRange;
    stream >> timeCodeRange;

    return timeCodeRange;
}

static
void
_AssertIsDefaultRange(const UsdUtilsTimeCodeRange& timeCodeRange)
{
    const UsdUtilsTimeCodeRange defaultRange;

    // XXX: Using TF_AXIOM here tickles a strange bug in Apple's clang 9.0.0
    // that causes compilation to fail with an error that looks like this:
    //
    // fatal error: error in backend: unsupported relocation of undefined symbol 'LBB0_-1'
    //
    // As a result, we use assert here instead.
    assert(timeCodeRange == defaultRange);
}


int
main(int argc, char* argv[])
{
    const UsdUtilsTimeCodeRange defaultRange;
    TF_AXIOM(_ValidateIteration(defaultRange, std::vector<UsdTimeCode>()));
    std::string emptyRangeString = _GetStringByStreamInsertion(defaultRange);
    TF_AXIOM(emptyRangeString == "NONE");

    const UsdUtilsTimeCodeRange singleRange(UsdTimeCode(123.0));
    TF_AXIOM(
        _ValidateIteration(singleRange, {
            UsdTimeCode(123.0)})
    );
    std::string frameSpec = _GetStringByStreamInsertion(singleRange);
    TF_AXIOM(frameSpec == "123");
    UsdUtilsTimeCodeRange frameSpecRange =
        _GetTimeCodeRangeByStreamExtraction(frameSpec);
    TF_AXIOM(frameSpecRange == singleRange);

    const UsdUtilsTimeCodeRange ascendingRange(
        UsdTimeCode(101.0),
        UsdTimeCode(105.0));
    TF_AXIOM(
        _ValidateIteration(ascendingRange, {
            UsdTimeCode(101.0),
            UsdTimeCode(102.0),
            UsdTimeCode(103.0),
            UsdTimeCode(104.0),
            UsdTimeCode(105.0)})
    );
    frameSpec = _GetStringByStreamInsertion(ascendingRange);
    TF_AXIOM(frameSpec == "101:105");
    frameSpecRange = _GetTimeCodeRangeByStreamExtraction(frameSpec);
    TF_AXIOM(frameSpecRange == ascendingRange);

    const UsdUtilsTimeCodeRange descendingRange(
        UsdTimeCode(105.0),
        UsdTimeCode(101.0));
    TF_AXIOM(
        _ValidateIteration(descendingRange, {
            UsdTimeCode(105.0),
            UsdTimeCode(104.0),
            UsdTimeCode(103.0),
            UsdTimeCode(102.0),
            UsdTimeCode(101.0)})
    );
    frameSpec = _GetStringByStreamInsertion(descendingRange);
    TF_AXIOM(frameSpec == "105:101");
    frameSpecRange = _GetTimeCodeRangeByStreamExtraction(frameSpec);
    TF_AXIOM(frameSpecRange == descendingRange);

    const UsdUtilsTimeCodeRange twosRange(
        UsdTimeCode(101.0),
        UsdTimeCode(109.0),
        2.0);
    TF_AXIOM(
        _ValidateIteration(twosRange, {
            UsdTimeCode(101.0),
            UsdTimeCode(103.0),
            UsdTimeCode(105.0),
            UsdTimeCode(107.0),
            UsdTimeCode(109.0)})
    );
    frameSpec = _GetStringByStreamInsertion(twosRange);
    TF_AXIOM(frameSpec == "101:109x2");
    frameSpecRange = _GetTimeCodeRangeByStreamExtraction(frameSpec);
    TF_AXIOM(frameSpecRange == twosRange);

    // Make sure we yield the same time codes if the endTimeCode does not align
    // with the last time code in the range based on the stride.
    const UsdUtilsTimeCodeRange twosPlusRange(
        UsdTimeCode(101.0),
        UsdTimeCode(110.0),
        2.0);
    TF_AXIOM(
        _ValidateIteration(twosPlusRange, {
            UsdTimeCode(101.0),
            UsdTimeCode(103.0),
            UsdTimeCode(105.0),
            UsdTimeCode(107.0),
            UsdTimeCode(109.0)})
    );
    frameSpec = _GetStringByStreamInsertion(twosPlusRange);
    TF_AXIOM(frameSpec == "101:110x2");
    frameSpecRange = _GetTimeCodeRangeByStreamExtraction(frameSpec);
    TF_AXIOM(frameSpecRange == twosPlusRange);

    const UsdUtilsTimeCodeRange fractionalStrideRange(
        UsdTimeCode(101.0),
        UsdTimeCode(104.0),
        0.5);
    TF_AXIOM(
        _ValidateIteration(fractionalStrideRange, {
            UsdTimeCode(101.0),
            UsdTimeCode(101.5),
            UsdTimeCode(102.0),
            UsdTimeCode(102.5),
            UsdTimeCode(103.0),
            UsdTimeCode(103.5),
            UsdTimeCode(104.0)})
    );
    frameSpec = _GetStringByStreamInsertion(fractionalStrideRange);
    TF_AXIOM(frameSpec == "101:104x0.5");
    frameSpecRange = _GetTimeCodeRangeByStreamExtraction(frameSpec);
    TF_AXIOM(frameSpecRange == fractionalStrideRange);

    const UsdUtilsTimeCodeRange floatErrorStrideRange(
        UsdTimeCode(0.0),
        UsdTimeCode(7.0),
        0.7);
    TF_AXIOM(
        _ValidateIteration(floatErrorStrideRange, {
            UsdTimeCode(0.0),
            UsdTimeCode(0.7),
            UsdTimeCode(1.4),
            UsdTimeCode(2.1),
            UsdTimeCode(2.8),
            UsdTimeCode(3.5),
            UsdTimeCode(4.2),
            UsdTimeCode(4.9),
            UsdTimeCode(5.6),
            UsdTimeCode(6.3),
            UsdTimeCode(7.0)},
            /* useIsClose = */ true));
    frameSpec = _GetStringByStreamInsertion(floatErrorStrideRange);
    TF_AXIOM(frameSpec == "0:7x0.7");
    frameSpecRange = _GetTimeCodeRangeByStreamExtraction(frameSpec);
    TF_AXIOM(frameSpecRange == floatErrorStrideRange);

    const UsdUtilsTimeCodeRange floatErrorValuesRange(
        UsdTimeCode(456.7),
        UsdTimeCode(890.1),
        108.35);
    TF_AXIOM(
        _ValidateIteration(floatErrorValuesRange, {
            UsdTimeCode(456.7),
            UsdTimeCode(565.05),
            UsdTimeCode(673.4),
            UsdTimeCode(781.75),
            UsdTimeCode(890.1)},
            /* useIsClose = */ true));
    frameSpec = _GetStringByStreamInsertion(floatErrorValuesRange);
    TF_AXIOM(frameSpec == "456.7:890.1x108.35");
    frameSpecRange = _GetTimeCodeRangeByStreamExtraction(frameSpec);
    TF_AXIOM(frameSpecRange == floatErrorValuesRange);

    const UsdUtilsTimeCodeRange floatErrorStrideLongRange(
        UsdTimeCode(0.0),
        UsdTimeCode(9999.9),
        0.1);
    size_t numTimeCodes = 0u;
    for (const UsdTimeCode& timeCode : floatErrorStrideLongRange) {
        TF_AXIOM(timeCode.GetValue() == numTimeCodes * 0.1);
        ++numTimeCodes;
    }
    TF_AXIOM(numTimeCodes == 100000);
    frameSpec = _GetStringByStreamInsertion(floatErrorStrideLongRange);
    TF_AXIOM(frameSpec == "0:9999.9x0.1");
    frameSpecRange = _GetTimeCodeRangeByStreamExtraction(frameSpec);
    TF_AXIOM(frameSpecRange == floatErrorStrideLongRange);


    // Now test bad constructions and make sure we get an empty invalid range
    // (equivalent to defaultRange from above).

    // EarliestTime and Default cannot be used as the start or end.
    {
        const UsdUtilsTimeCodeRange badRange(
            UsdTimeCode::EarliestTime(), 104.0);
        _AssertIsDefaultRange(badRange);
    }

    {
        const UsdUtilsTimeCodeRange badRange(
            UsdTimeCode::Default(), 104.0);
        _AssertIsDefaultRange(badRange);
    }

    {
        const UsdUtilsTimeCodeRange badRange(
            101.0, UsdTimeCode::EarliestTime());
        _AssertIsDefaultRange(badRange);
    }

    {
        const UsdUtilsTimeCodeRange badRange(
            101.0, UsdTimeCode::Default());
        _AssertIsDefaultRange(badRange);
    }

    // The end must be greater than the start with a positive stride
    {
        const UsdUtilsTimeCodeRange badRange(104.0, 101.0, 1.0);
        _AssertIsDefaultRange(badRange);
    }

    // The end must be less than the start with a negative stride
    {
        const UsdUtilsTimeCodeRange badRange(101.0, 104.0, -1.0);
        _AssertIsDefaultRange(badRange);
    }

    // The stride cannot be zero.
    {
        const UsdUtilsTimeCodeRange badRange(101.0, 104.0, 0.0);
        _AssertIsDefaultRange(badRange);
    }


    // Finally, test some bad FrameSpecs and make sure we get an invalid empty
    // range (equivalent to defaultRange from above).

    {
        const UsdUtilsTimeCodeRange badRange =
            UsdUtilsTimeCodeRange::CreateFromFrameSpec("foobar");
        _AssertIsDefaultRange(badRange);
    }

    {
        const UsdUtilsTimeCodeRange badRange =
            UsdUtilsTimeCodeRange::CreateFromFrameSpec("101:102:103");
        _AssertIsDefaultRange(badRange);
    }

    {
        const UsdUtilsTimeCodeRange badRange =
            UsdUtilsTimeCodeRange::CreateFromFrameSpec("101foobar:104");
        _AssertIsDefaultRange(badRange);
    }

    {
        const UsdUtilsTimeCodeRange badRange =
            UsdUtilsTimeCodeRange::CreateFromFrameSpec("foobar101:104");
        _AssertIsDefaultRange(badRange);
    }

    {
        const UsdUtilsTimeCodeRange badRange =
            UsdUtilsTimeCodeRange::CreateFromFrameSpec("101:104foobar");
        _AssertIsDefaultRange(badRange);
    }

    {
        const UsdUtilsTimeCodeRange badRange =
            UsdUtilsTimeCodeRange::CreateFromFrameSpec("101:foobar104");
        _AssertIsDefaultRange(badRange);
    }

    {
        const UsdUtilsTimeCodeRange badRange =
            UsdUtilsTimeCodeRange::CreateFromFrameSpec("101x2.0");
        _AssertIsDefaultRange(badRange);
    }

    {
        const UsdUtilsTimeCodeRange badRange =
            UsdUtilsTimeCodeRange::CreateFromFrameSpec("101:109x2.0x3.0");
        _AssertIsDefaultRange(badRange);
    }

    {
        const UsdUtilsTimeCodeRange badRange =
            UsdUtilsTimeCodeRange::CreateFromFrameSpec("101:109x2.0foobar");
        _AssertIsDefaultRange(badRange);
    }

    {
        const UsdUtilsTimeCodeRange badRange =
            UsdUtilsTimeCodeRange::CreateFromFrameSpec("101:109xfoobar2.0");
        _AssertIsDefaultRange(badRange);
    }

    return EXIT_SUCCESS;
}
