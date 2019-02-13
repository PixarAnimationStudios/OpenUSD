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
#include "pxr/base/tf/span.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"

#include <algorithm>


using namespace std;
PXR_NAMESPACE_USING_DIRECTIVE


template <typename Span, typename Container>
void
Tf_TestSpanMatchesContainer(const Span& span, const Container& cont)
{
    TF_AXIOM(span.data() == cont.data());
    TF_AXIOM(span.size() == cont.size());
    TF_AXIOM(std::equal(span.begin(), span.end(), cont.begin()));
    TF_AXIOM(std::equal(span.cbegin(), span.cend(), cont.cbegin()));
    TF_AXIOM(std::equal(span.rbegin(), span.rend(), cont.rbegin()));
    TF_AXIOM(std::equal(span.crbegin(), span.crend(), cont.crbegin()));
}


void Tf_TestImplicitConversionInOverloads(TfSpan<int> span) {}
void Tf_TestImplicitConversionInOverloads(TfSpan<float> span) {}

void Tf_TestConstImplicitConversionInOverloads(TfSpan<const int> span) {}
void Tf_TestConstImplicitConversionInOverloads(TfSpan<const float> span) {}
                  

int main(int argc, char** argv)
{
    // Test empty spans.
    {
        TfSpan<int> span;
        TF_AXIOM(span.empty());
        TF_AXIOM(span.size() == 0);
        TF_AXIOM(span.data() == nullptr);
    }

    const std::vector<int> constData({1,2,3,4,5});

    std::vector<int> data(constData);

    // Test construction of a const span from a non-const container.
    {
        TfSpan<const int> span = data;
        Tf_TestSpanMatchesContainer(span, data);

        // Copy constructor.
        TfSpan<const int> copy(span);
        Tf_TestSpanMatchesContainer(copy, span);
    }
    {
        auto span = TfMakeConstSpan(data);
        Tf_TestSpanMatchesContainer(span, data);
    }

    // Test construction of non-const span from a non-const container.
    {
        TfSpan<int> span = data;
        Tf_TestSpanMatchesContainer(span, data);
        TF_AXIOM(!span.empty());

        // Should be allowed to implicitly convert a non-const span to const.
        TfSpan<const int> cspan(span);
        Tf_TestSpanMatchesContainer(cspan, span);
    }
    {
        auto span = TfMakeSpan(data);
        Tf_TestSpanMatchesContainer(span, data);
    }

    // Test construction of const span from a const container.
    {
        // XXX: Should not be allowed to construct a non-const span
        // from a const container.
        // TfSpan<int> span = data; // discards cv-qualifier

        TfSpan<const int> span = constData;
        Tf_TestSpanMatchesContainer(span, constData);
    }
    {
        // XXX: Should not be allowed to construct a non-const span
        // from a const container.
        // auto span = TfMakeSpan(data); // discards cv-qualifier

        auto span = TfMakeConstSpan(constData);
        Tf_TestSpanMatchesContainer(span, constData);
    }

    // Test subspans.
    {
        // Should be able to construct subspans from a constant span.
        const TfSpan<const int> span(data);

        // Test with default count (std::dynamic_extent)
        TfSpan<const int> subspan = span.subspan(2);
        const std::vector<int> expectedSubspan({3,4,5});
        TF_AXIOM(std::equal(subspan.begin(), subspan.end(),
                            expectedSubspan.begin()));

        // Test with explicit count
        TfSpan<const int> subspan2 = span.subspan(2, 2);
        const std::vector<int> expectedSubspan2({3,4});
        TF_AXIOM(std::equal(subspan2.begin(), subspan2.end(),   
                            expectedSubspan2.begin()));
    }

    // Test span edits.
    {
        TfSpan<int> span(data);
        for (ptrdiff_t i = 0; i < span.size(); ++i) {
            span[i] = (i+1)*10;
        }
        
        const std::vector<int> expected({10,20,30,40,50});
        TF_AXIOM(std::equal(data.begin(), data.end(),
                            expected.begin()));
    }

    // Test implicit conversion in function calls with multiple overloads.
    Tf_TestImplicitConversionInOverloads(data);
    Tf_TestConstImplicitConversionInOverloads(constData);

    return 0;
}
