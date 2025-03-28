//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/tangentConversions.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/tf/diagnosticLite.h"

#include <iostream>
#include <array>
#include <cmath>

PXR_NAMESPACE_USING_DIRECTIVE

// Compare the ratio of the two values to 1.0. In the second test, the
// values get larger and an epsilon based on subtraction would need to
// increase as well. The ratio, however, stays close to 1.0 for close
// values.
inline bool
IsClose(double a, double b, double epsilon) {
    return fabs(1.0 - (a/b)) < epsilon;
}

template <typename T>
bool ExerciseConversions(TsTime width, T value)
{
    double epsilon = 1.0e-6;
    
    double widthOut;
    T valueOut;

    // Test noop conversions. Operations are exact so it should
    // not perform any rounding.
    TsConvertToStandardTangent(width, value,
                               false,  // convertHeightToSlope
                               false,  // divideValuesByThree
                               false,  // negateHeight
                               &widthOut, &valueOut);
    TF_AXIOM(width == widthOut);
    TF_AXIOM(value == valueOut);

    TsConvertFromStandardTangent(width, value,
                                 false,  // convertSlopeToHeight
                                 false,  // multiplyValuesByThree
                                 false,  // negateHeight
                                 &widthOut, &valueOut);
    TF_AXIOM(width == widthOut);
    TF_AXIOM(value == valueOut);

    // Test negating conversions. Also no rounding.
    TsConvertToStandardTangent(width, value,
                               false,  // convertHeightToSlope
                               false,  // divideValuesByThree
                               true,   // negateHeight
                               &widthOut, &valueOut);
    TF_AXIOM(width == widthOut);
    TF_AXIOM(value == -valueOut);

    TsConvertFromStandardTangent(width, value,
                                 false,  // convertSlopeToHeight
                                 false,  // multiplyValuesByThree
                                 true,   // negateHeight
                                 &widthOut, &valueOut);
    TF_AXIOM(width == widthOut);
    TF_AXIOM(value == -valueOut);

    // Verify transforms
    TsConvertToStandardTangent(width, value,
                               true,   // convertHeightToSlope
                               true,   // divideValuesByThree
                               true,   // negateHeight
                               &widthOut, &valueOut);
    if (!IsClose(width/3, widthOut, epsilon)) {
        std::cout << std::hexfloat
                  << "ConvertToStandardTangent not close:\n"
                  << "    width             = " << width << "\n"
                  << "    width/3           = " << width/3 << "\n"
                  << "    widthOut          = " << widthOut << "\n"
                  << std::endl;
    }

    TF_AXIOM(IsClose(width/3, widthOut, epsilon));
    TF_AXIOM(IsClose(T(value/width), -valueOut, epsilon));

    TsConvertFromStandardTangent(width, value,
                                 true,   // convertSlopeToHeight
                                 true,   // multiplyValuesByThree
                                 true,   // negateHeight
                                 &widthOut, &valueOut);
    if (!IsClose(width*3, widthOut, epsilon)) {
        std::cout << std::hexfloat
                  << "ConvertFromStandardTangent not close:\n"
                  << "    width             = " << width << "\n"
                  << "    width*3           = " << width*3 << "\n"
                  << "    widthOut          = " << widthOut << "\n"
                  << std::endl;
    }
    TF_AXIOM(IsClose(width*3, widthOut, epsilon));
    TF_AXIOM(IsClose(T(-value*3*width), valueOut, epsilon));

    // Verify safe round trips. This requires converting first to get rounded
    // values then, converting those back and forth and verifying that we get
    // unchanged values.
    double widthRounded, widthIntermediate;
    T valueRounded, valueIntermediate;
    TsConvertToStandardTangent(width, value,
                               true,   // convertHeightToSlope
                               true,   // divideValuesByThree
                               true,   // negateHeight
                               &widthRounded, &valueRounded);
    TsConvertFromStandardTangent(widthRounded, valueRounded,
                                 true,   // convertSlopeToHeight
                                 true,   // multiplyValuesByThree
                                 true,   // negateHeight
                                 &widthIntermediate, &valueIntermediate);
    TsConvertToStandardTangent(widthIntermediate, valueIntermediate,
                               true,   // convertHeightToSlope
                               true,   // divideValuesByThree
                               true,   // negateHeight
                               &widthOut, &valueOut);

    if (widthRounded != widthOut || valueRounded != valueOut) {
        std::cout << std::hexfloat
                  << "Round trip failed for to - from - to:\n"
                  << "    sizeof(value)     = " << sizeof(value) << "\n"
                  << "    width             = " << width << "\n"
                  << "    value             = " << double(value) << "\n"
                  << "    widthRounded      = " << widthRounded << "\n"
                  << "    valueRounded      = " << double(valueRounded) << "\n"
                  << "    widthIntermediate = " << widthIntermediate << "\n"
                  << "    valueIntermediate = " << double(valueIntermediate) << "\n"
                  << "    widthOut          = " << widthOut << "\n"
                  << "    valueOut          = " << double(valueOut) << "\n"
                  << std::endl;
    }
    // TF_AXIOM(widthRounded == widthOut);
    // TF_AXIOM(valueRounded == valueOut);

    TsConvertFromStandardTangent(width, value,
                                 true,   // convertHeightToSlope
                                 true,   // divideValuesByThree
                                 true,   // negateHeight
                                 &widthRounded, &valueRounded);
    TsConvertToStandardTangent(widthRounded, valueRounded,
                               true,   // convertSlopeToHeight
                               true,   // multiplyValuesByThree
                               true,   // negateHeight
                               &widthIntermediate, &valueIntermediate);
    TsConvertFromStandardTangent(widthIntermediate, valueIntermediate,
                                 true,   // convertHeightToSlope
                                 true,   // divideValuesByThree
                                 true,   // negateHeight
                                 &widthOut, &valueOut);

    if (widthRounded != widthOut || valueRounded != valueOut) {
        std::cout << std::hexfloat
                  << "Round trip failed for from - to - from:\n"
                  << "    sizeof(value)     = " << sizeof(value) << "\n"
                  << "    width             = " << width << "\n"
                  << "    value             = " << double(value) << "\n"
                  << "    widthRounded      = " << widthRounded << "\n"
                  << "    valueRounded      = " << double(valueRounded) << "\n"
                  << "    widthIntermediate = " << widthIntermediate << "\n"
                  << "    valueIntermediate = " << double(valueIntermediate) << "\n"
                  << "    widthOut          = " << widthOut << "\n"
                  << "    valueOut          = " << double(valueOut) << "\n"
                  << std::endl;
    }
    // TF_AXIOM(widthRounded == widthOut);
    // TF_AXIOM(valueRounded == valueOut);

    return true;
}
    

    
// Test a range of consecutive floating point numbers to verify
// that conversion and rounding works correctly for all the
// low order bit patterns.
template <typename T>
void TestNearbyValues()
{
    // std::numeric_limits<T>::epsilon() is the distance between consecutive
    // floating point numbers in the range between 1.0 and 2.0. That distance
    // is doubled between 2.0 and 4.0.
    const T valueEps = std::numeric_limits<T>::epsilon();
    const double widthEps = std::numeric_limits<T>::epsilon();

    // Fill values with 33 consecutive floating point values around 2.0
    std::array<T, 33> values;
    for (int i = 0; i < 16; ++i) {
        values[i] = 2.0 + (i - 16) * valueEps;
    }
    values[16] = 2.0;
    for (int i = 17; i < 33; ++i) {
        values[i] = 2.0 + (i - 16) * 2 * valueEps;
    }

    // Similarly fill in widths with consecutive double values around 2.0
    std::array<double, 33> widths;
    for (int i = 0; i < 16; ++i) {
        widths[i] = 2.0 + (i - 16) * widthEps;
    }
    widths[16] = 2.0;
    for (int i = 17; i < 33; ++i) {
        widths[i] = 2.0 + (i - 16) * 2 * widthEps;
    }

    for (double width: widths) {
        for (T value: values) {
            ExerciseConversions(width, value);
        }
    }
}
    
// Test a range of larger values. Note that they're note really all that
// large since we're trying to make sur we don't overflow a half value
// which tops out at 65504. So we need 3 * width * value < 65504.
template <typename T>
void TestLargeValues()
{
    // Back to epsilon again. Use 2.0 - epsilon as a base value because it
    // has lots (all) of the mantissa bits set.
    const T valueBase = 2.0 - std::numeric_limits<T>::epsilon();
    const double widthBase = 2.0 - std::numeric_limits<T>::epsilon();

    // Fill values and widths with 13 numbers with exponents from 2**-6 to 2**6
    std::array<T, 13> values;
    std::array<double, 13> widths;
    for (int i = 0; i < 13; ++i) {
        values[i] = std::ldexp(valueBase, i - 6);
        widths[i] = std::ldexp(widthBase, i - 6);
    }

    for (double width: widths) {
        for (T value: values) {
            ExerciseConversions(width, value);
        }
    }
}

int main()
{
    TestNearbyValues<double>();
    TestNearbyValues<float>();
    TestNearbyValues<GfHalf>();

    TestLargeValues<double>();
    TestLargeValues<float>();
    TestLargeValues<GfHalf>();

    return 0;
}
