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
#include "pxr/base/tf/unicodeUtils.h"

#include <string_view>

PXR_NAMESPACE_USING_DIRECTIVE

static bool
TestUtf8CodePointView()
{

    {
        TF_AXIOM(TfUtf8CodePointView{}.empty());
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
            TF_AXIOM(codePoint != TfUtf8CodePointIterator::INVALID_CODE_POINT);
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
            TF_AXIOM(codePoint != TfUtf8CodePointIterator::INVALID_CODE_POINT);
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
            TF_AXIOM(codePoint != TfUtf8CodePointIterator::INVALID_CODE_POINT);
        }

    }
    return true;
}

static bool
Test_TfUnicodeUtils()
{
    return TestUtf8CodePointView();
}

TF_ADD_REGTEST(TfUnicodeUtils);
