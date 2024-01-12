//
// Copyright 2023 Pixar
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
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/unicodeUtils.h"

#include <algorithm>
#include <array>
#include <limits>
#include <string_view>

PXR_NAMESPACE_USING_DIRECTIVE

static bool
TestUtf8CodePoint()
{
    {
        // Test default behavior
        TF_AXIOM(TfUtf8CodePoint{} == TfUtf8InvalidCodePoint);
    }
    {
        // Test boundary conditions
        TF_AXIOM(TfUtf8CodePoint{0}.AsUInt32() == 0);
        TF_AXIOM(TfUtf8CodePoint{TfUtf8CodePoint::MaximumValue}.AsUInt32() ==
                 TfUtf8CodePoint::MaximumValue);
        TF_AXIOM(TfUtf8CodePoint{TfUtf8CodePoint::MaximumValue + 1} ==
                 TfUtf8InvalidCodePoint);
        TF_AXIOM(
            TfUtf8CodePoint{std::numeric_limits<uint32_t>::max()} ==
            TfUtf8InvalidCodePoint);
        TF_AXIOM(TfUtf8CodePoint{
                    TfUtf8CodePoint::SurrogateRange.first - 1}.AsUInt32() ==
                 TfUtf8CodePoint::SurrogateRange.first - 1);
        TF_AXIOM(TfUtf8CodePoint{
                    TfUtf8CodePoint::SurrogateRange.second + 1}.AsUInt32() ==
                 TfUtf8CodePoint::SurrogateRange.second + 1);
        TF_AXIOM(TfUtf8CodePoint{TfUtf8CodePoint::SurrogateRange.first} ==
                 TfUtf8InvalidCodePoint);
        TF_AXIOM(TfUtf8CodePoint{TfUtf8CodePoint::SurrogateRange.second} ==
                 TfUtf8InvalidCodePoint);
        TF_AXIOM(TfUtf8CodePoint{
                    (TfUtf8CodePoint::SurrogateRange.second +
                     TfUtf8CodePoint::SurrogateRange.first) / 2} ==
                 TfUtf8InvalidCodePoint);
    }
    {
        // Test TfStringify
        TF_AXIOM(TfStringify(TfUtf8CodePoint(97)) == "a");
        TF_AXIOM(TfStringify(TfUtf8CodePoint(8747)) == "∫");
        TF_AXIOM(TfStringify(TfUtf8InvalidCodePoint) == "�");
        TF_AXIOM(TfStringify(TfUtf8CodePoint{}) ==
                 TfStringify(TfUtf8InvalidCodePoint));

    }
    return true;
}

static bool
TestUtf8CodePointView()
{

    {
        TF_AXIOM(TfUtf8CodePointView{}.empty());
        TF_AXIOM(TfUtf8CodePointView{}.cbegin() == TfUtf8CodePointView{}.EndAsIterator());
        TF_AXIOM(std::cbegin(TfUtf8CodePointView{}) == std::cend(TfUtf8CodePointView{}));
    }

    // Exercise the iterator converting from UTF-8 char to code point
    {
        const std::string_view s1{"ⅈ75_hgòð㤻"};
        TfUtf8CodePointView u1{s1};
        auto i1 = std::cbegin(u1);
        TF_AXIOM(i1.GetBase() == s1.begin());
        TF_AXIOM(*i1 == 8520);
        std::advance(i1, 9);
        TF_AXIOM(i1 == std::cend(u1));

        for (const uint32_t codePoint : u1) {
            TF_AXIOM(codePoint != TfUtf8InvalidCodePoint.AsUInt32());
        }
    }

    {
        const std::string_view s2{"㤼01৪∫"};
        TfUtf8CodePointView u2{s2};
        auto i2 = std::cbegin(u2);
        TF_AXIOM(i2.GetBase() == s2.begin());
        TF_AXIOM(*i2 == 14652);
        std::advance(i2, 5);
        TF_AXIOM(i2 == std::cend(u2));

        for (const uint32_t codePoint : u2) {
            TF_AXIOM(codePoint != TfUtf8InvalidCodePoint.AsUInt32());
        }
    }

    {
        const std::string_view s3{"㤻üaf-∫⁇…🔗"};
        TfUtf8CodePointView u3{s3};
        auto i3a = std::cbegin(u3);
        auto i3b = std::cbegin(u3);

        // The C++20 ranges version of find_if can be used with sentinels in
        // C++20
        for (; i3b != std::cend(u3); ++i3b) {
            if (*(i3b.GetBase()) == '-') {
                break;
            }
        }
        TF_AXIOM(i3b != std::cend(u3));

        // i3a should contain all characters before the "-"
        TF_AXIOM(*i3a == 14651);
        std::advance(i3a, 4);
        TF_AXIOM(i3a == i3b);
        TF_AXIOM(i3a.GetBase() == i3b.GetBase());

        // i3b should include the "-" character
        TF_AXIOM(*i3b == 45);
        std::advance(i3b, 5);
        TF_AXIOM(i3b == std::cend(u3));

        for (const uint32_t codePoint : u3) {
            TF_AXIOM(codePoint != TfUtf8InvalidCodePoint.AsUInt32());
        }

    }
    {
        // Unexpected continuations (\x80 and \x81) should be decoded as
        // invalid code points.
        const std::string_view sv{"\x80\x61\x62\x81\x63"};
        const TfUtf8CodePointView uv{sv};
        TF_AXIOM(std::next(std::begin(uv), 5) == std::end(uv));
        TF_AXIOM(std::next(std::begin(uv), 5).GetBase() == std::end(sv));

        std::array<uint32_t, 5> codePoints{0};
        const std::array<uint32_t, 5> expectedCodePoints{{
            TfUtf8InvalidCodePoint.AsUInt32(), 0x61, 0x62,
            TfUtf8InvalidCodePoint.AsUInt32(), 0x63}};
        std::copy(std::cbegin(uv), uv.EndAsIterator(), std::begin(codePoints));
        TF_AXIOM(codePoints == expectedCodePoints);
    }
    {
        // Incomplete UTF-8 sequences should not consume valid character
        // sequences
        const std::string_view sv{"\xc0\x61\xe0\x85\x62\xf0\x83\x84\x63\xf1"};
        const TfUtf8CodePointView uv{sv};
        TF_AXIOM(std::next(std::begin(uv), 7) == std::end(uv));
        TF_AXIOM(std::next(std::begin(uv), 7).GetBase() == std::end(sv));

        std::array<uint32_t, 7> codePoints{0};
        const std::array<uint32_t, 7> expectedCodePoints{{
            TfUtf8InvalidCodePoint.AsUInt32(), 0x61,
            TfUtf8InvalidCodePoint.AsUInt32(), 0x62,
            TfUtf8InvalidCodePoint.AsUInt32(), 0x63,
            TfUtf8InvalidCodePoint.AsUInt32()}};
        std::copy(std::cbegin(uv), uv.EndAsIterator(), std::begin(codePoints));
        TF_AXIOM(codePoints == expectedCodePoints);
    }
    return true;
}

/// Ensure that every code point can be serialized into a string and converted
/// back to a code point.
static bool
TestUtf8CodePointReflection()
{
    for (uint32_t value = 0; value <= TfUtf8CodePoint::MaximumValue; value++) {
        if ((value < TfUtf8CodePoint::SurrogateRange.first) ||
            (value > TfUtf8CodePoint::SurrogateRange.second)) {
            const TfUtf8CodePoint codePoint{value};
            TF_AXIOM(codePoint.AsUInt32() == value);
            const std::string text{TfStringify(codePoint)};
            const auto view = TfUtf8CodePointView{text};
            TF_AXIOM(std::cbegin(view) != std::cend(view));
            TF_AXIOM(*std::cbegin(view) == codePoint.AsUInt32());
            TF_AXIOM(++std::cbegin(view) == std::cend(view));
        }
    }
    return true;
}

/// Ensure that the surrogate range is replaced with the invalid character
static bool
TestUtf8CodePointSurrogateRange()
{
    for (uint32_t value = TfUtf8CodePoint::SurrogateRange.first;
         value <= TfUtf8CodePoint::SurrogateRange.second; value++) {
        const TfUtf8CodePoint surrogateCodePoint{value};
        TF_AXIOM(surrogateCodePoint == TfUtf8InvalidCodePoint);
        TF_AXIOM(TfStringify(surrogateCodePoint) ==
                 TfStringify(TfUtf8InvalidCodePoint));
    }
    return true;
}

/// Ensure that code points outside of the ASCII range are ordered by
/// code point value.
static bool
TestUtf8DictionaryLessThanOrdering()
{
    // All ASCII code points should be less than the first non-ASCII
    // code point.
    for (uint32_t value = 0; value <= 127; value++) {
        const TfUtf8CodePoint asciiCodePoint(value);
        TF_AXIOM(TfDictionaryLessThan{}(
            TfStringify(asciiCodePoint),
            TfStringify(TfUtf8CodePoint{128})));
    }
    // All non-ASCII code points should be numerically ordered.
    for (uint32_t value = 129;
         value <= TfUtf8CodePoint::MaximumValue; value++) {
        if ((value < TfUtf8CodePoint::SurrogateRange.first) ||
            (value > TfUtf8CodePoint::SurrogateRange.second + 1)) {
           const TfUtf8CodePoint codePoint(value);
           const TfUtf8CodePoint previousCodePoint(value-1);
           TF_AXIOM(TfDictionaryLessThan{}(
                TfStringify(previousCodePoint),
                TfStringify(codePoint))
            );
        }
    }
    // Test that the first value after the surrogate range is greater than
    // the last value before the surrogate range.
    TF_AXIOM(TfDictionaryLessThan{}(
        TfStringify(TfUtf8CodePoint::SurrogateRange.first - 1),
        TfStringify(TfUtf8CodePoint::SurrogateRange.second + 1))
    );
    return true;
}

static bool
TestCharacterClasses()
{
    // a mix of code points that should fall into the following
    // character classes that make up XID_Start:
    // Lu | Ll | Lt | Lm | Lo | Nl 
    std::vector<uint32_t> xidStartCodePoints = {
        0x0043u,    // Latin captial letter C (Lu)
        0x006Au,    // Latin small letter j (Ll)
        0x0254u,    // Latin small letter Open o (Ll)
        0x01C6u,    // Latin small letter DZ with Caron (Ll)
        0x01CBu,    // Latin capital letter N with small letter j (Lt)
        0x02B3u,    // Modifier letter small r (Lm)
        0x10464u,   // Shavian letter Loll (Lo)
        0x132B5u,   // Egyptian hieroglpyh R0004 (Lo)
        0x12421u,   // Cuneiform numeric sign four geshu (Nl)
        0xFDABu,    // Arabic Ligature seen with Khan 
                    //with Alef Maksura FInal Form (Lo)
        0x18966u,   // Tangut Component-359 (Lo)
        0x10144u,   // Greek acrophonic attick fifty (Nl)
        0x037Fu,    // Greek captial letter YOT (Lu) 
                    // [test singular code point range]
        0x2F800u,   // CJK Compatibility Ideograph-2F800 (Lo) 
                    // [test start range]
        0x3134Au,   // CJK Ideograph Extension G Last (Lo) 
                    // [test end range]
    };


    // a mix of code points that should fall into the following
    // character classes that make up XID_Continue
    // XID_Start | Nd | Mn | Mc | Pc
    std::vector<uint32_t> xidContinueCodePoints = {
        0x0032u,    // Digit two (Nd)
        0x0668u,    // Arabic-Indic Digit Eight (Nd)
        0x07C0u,    // NKO Digit Zero (Nd)
        0x1E145u,   // Nyiakeng Puachue Hmong Digit Five (Nd)
        0x0300u,    // Combining Grave Accent (Mn)
        0x2CEFu,    // Coptic Combining NI Above (Mn)
        0x10A02u,   // Kharoshthi Vowel Sign U (Mn)
        0x16F92u,   // Miao Tone Below (Mn)
        0x0903u,    // Devanagari Sign Visarga (Mc)
        0x16F55u,   // Miao Vowel Sign AA (Mc)
        0x1D172u,   // Musical Symbol Combining Flag-5 (Mc)
        0x203Fu,    // Undertie (Pc)
        0x005Fu,    // Low line (underscore) (Pc)
        0xFE4Fu,    // Wavy Low Line (Pc)
        0x05BFu,    // Hebrew Point Rafe (Mn) [test singular code point range]
        0x1E2ECu,   // Wancho Tone Tup (Mn) [test start range]
        0xE01EFu,   // Variation Selector-256 (Mn) [test end range]
    };

    // code points that shouldn't fall into either XID_Start
    // or XID_Continue
    std::vector<uint32_t> invalidCodePoints = {
        0x002Du,    // Hyphen-Minus (Pd)
        0x00ABu,    // Left-Pointing Double Angle Quotation Mark (Pi)
        0x2019u,    // Right Single Quotation Mark (Pf)
        0x2021u,    // Double Dagger (Po)
        0x1ECB0u,   // Indic Siyaq Rupee Mark (Sc)
        0x0020u,    // Space (Zs)
        0x3000u,    // Ideographic Space (Zs)
        0x000Bu,    // Line tabulation (Cc)
        0xF8FEu,    // Private Use (Co)
    };

    for (size_t i = 0; i < xidStartCodePoints.size(); i++)
    {
        TF_AXIOM(TfIsUtf8CodePointXidStart(xidStartCodePoints[i]));

        // XID_Continue sets contain XID_Start
        TF_AXIOM(TfIsUtf8CodePointXidContinue(xidStartCodePoints[i]));
    }

    for (size_t i = 0; i < xidContinueCodePoints.size(); i++)
    {
        TF_AXIOM(TfIsUtf8CodePointXidContinue(xidContinueCodePoints[i]));
    }

    for (size_t i = 0; i < invalidCodePoints.size(); i++)
    {
        TF_AXIOM(!TfIsUtf8CodePointXidStart(invalidCodePoints[i]));
        TF_AXIOM(!TfIsUtf8CodePointXidContinue(invalidCodePoints[i]));
    }

    // now test some strings with some characters from each of these sets
    // such that we can exercise the iterator converting from UTF-8 char
    // to code point
    std::string s1 = "ⅈ75_hgòð㤻";
    std::string s2 = "㤼01৪∫";
    std::string s3 = "㤻üaf-∫⁇…🔗";
    std::string s3_1 = s3.substr(0, s3.find("-"));
    std::string s3_2 = s3.substr(s3.find("-"));
    std::string_view sv1 {s1};
    std::string_view sv2 {s2};
    std::string_view sv3 {s3_1};
    std::string_view sv4 {s3_2};

    TfUtf8CodePointView view1 {sv1};
    TfUtf8CodePointView view2 {sv2};
    TfUtf8CodePointView view3 {sv3};
    TfUtf8CodePointView view4 {sv4};

    // s1 should start with XID_Start and then have XID_Continue
    bool first = true;
    for (const uint32_t codePoint : view1)
    {
        bool result = first ? TfIsUtf8CodePointXidStart(codePoint)
            : TfIsUtf8CodePointXidContinue(codePoint);
        TF_AXIOM(result);

        first = false;
    }

    // s2 should start with XID_Start, have three characters that are 
    // XID_Continue, then one that isn't in either 
    size_t count = 0;
    for (const uint32_t codePoint : view2)
    {
        if (count == 0)
        {
            TF_AXIOM(TfIsUtf8CodePointXidStart(codePoint));
        }
        else if (count == 4)
        {
            TF_AXIOM(!TfIsUtf8CodePointXidContinue(codePoint));
        }
        else
        {
            TF_AXIOM(TfIsUtf8CodePointXidContinue(codePoint));
        }

        count++;
    }

    // s3 should have all XID_Start characters in the first set
    // (before the "-") and all invalid characters after
    for (const uint32_t codePoint : view3)
    {
        TF_AXIOM(TfIsUtf8CodePointXidStart(codePoint));
    }
    for (const uint32_t codePoint : view4)
    {
        TF_AXIOM(!TfIsUtf8CodePointXidContinue(codePoint));
    }

    // test uint32_t max, which should overflow the number of code points
    // and make sure it returns invalid
    TF_AXIOM(!TfIsUtf8CodePointXidStart(
        std::numeric_limits<uint32_t>::max()));
    TF_AXIOM(!TfIsUtf8CodePointXidContinue(
        std::numeric_limits<uint32_t>::max()));

    // Test TfUtf8CodePoint::MaximumValue (the last valid) and
    // TfUtf8CodePoint::MaximumValue + 1 (the first invalid)
    TF_AXIOM(!TfIsUtf8CodePointXidStart(TfUtf8CodePoint::MaximumValue));
    TF_AXIOM(!TfIsUtf8CodePointXidContinue(TfUtf8CodePoint::MaximumValue));
    TF_AXIOM(!TfIsUtf8CodePointXidStart(TfUtf8CodePoint::MaximumValue + 1));
    TF_AXIOM(!TfIsUtf8CodePointXidContinue(TfUtf8CodePoint::MaximumValue + 1));

    return true;
}

static bool
Test_TfUnicodeUtils()
{
    return TestUtf8CodePoint() &&
           TestUtf8CodePointView() &&
           TestCharacterClasses() &&
           TestUtf8CodePointReflection() &&
           TestUtf8CodePointSurrogateRange() &&
           TestUtf8DictionaryLessThanOrdering();
}

TF_ADD_REGTEST(TfUnicodeUtils);
