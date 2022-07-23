//
// Copyright 2021 Pixar
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
#include "pxr/imaging/hd/containerDataSourceEditor.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <sstream>
#include <unordered_set>

// ----------------------------------------------------------------------------

std::ostream &operator <<(std::ostream &stream,
        HdContainerDataSourceHandle const &container )
{
    HdDebugPrintDataSource(stream, container);
    return stream;
}

#define COMPAREVALUE(T, A, B) \
    if (A == B) { \
        std::cout << T << " matches." << std::endl; \
    } else { \
        std::cerr << T << " doesn't match. Expecting " << B << " got " << A \
            << std::endl; \
        return false; \
    }

#define COMPARECONTAINERS(T, A, B) \
    { \
        std::ostringstream aStream; \
        aStream << std::endl; \
        HdDebugPrintDataSource(aStream, A); \
        std::ostringstream bStream; \
        bStream << std::endl; \
        HdDebugPrintDataSource(bStream, B); \
        COMPAREVALUE(T, aStream.str(), bStream.str()) \
    }

// test brevity conveniences

#define I(v) (HdRetainedTypedSampledDataSource<int>::New(v))

HdDataSourceLocator L(const std::string &inputStr)
{
    std::vector<TfToken> tokens;
    for (const std::string &s : TfStringSplit(inputStr, "/")) {
        if (!s.empty()) {
            tokens.push_back(TfToken(s));
        }
    }
    return HdDataSourceLocator(tokens.size(), tokens.data());
}

// ----------------------------------------------------------------------------

bool TestSimpleOverlay()
{
    HdContainerDataSourceHandle containers[] = {
        HdRetainedContainerDataSource::New(
            TfToken("A"), I(1),
            TfToken("F"), I(7)
            ),

        HdRetainedContainerDataSource::New(
            TfToken("B"), I(2),
            TfToken("C"), I(3)),

        HdRetainedContainerDataSource::New(
            TfToken("D"), HdRetainedContainerDataSource::New(
                    TfToken("E"), I(4)
                ),
            TfToken("F"), I(6),
            TfToken("G"), I(8)
            ),
    };

    HdOverlayContainerDataSourceHandle test =
            HdOverlayContainerDataSource::New(3, containers);

    HdContainerDataSourceHandle baseline =
        HdRetainedContainerDataSource::New(
            TfToken("A"), I(1),
            TfToken("B"), I(2),
            TfToken("C"), I(3),
            TfToken("D"), HdRetainedContainerDataSource::New(
                    TfToken("E"), I(4)),
            TfToken("F"), I(7),
            TfToken("G"), I(8));

    COMPARECONTAINERS("three container overlay:", test, baseline);

    return true;
}


// ----------------------------------------------------------------------------



bool TestContainerEditor()
{
    {
        HdContainerDataSourceHandle baseline =
            HdRetainedContainerDataSource::New(
                TfToken("A"), I(1),
                TfToken("B"), I(2)
            );

        HdContainerDataSourceHandle test = HdContainerDataSourceEditor()
            .Set(L("A"), I(1))
            .Set(L("B"), I(2))
            .Finish();

        COMPARECONTAINERS("one level:", test, baseline);
    }

    {
        HdContainerDataSourceHandle test = HdContainerDataSourceEditor()
            .Set(L("A"), I(1))
            .Set(L("B"), I(2))
            .Set(L("C/D"), I(3))
            .Set(L("C/E"), I(4))
            .Set(L("B"), I(5))
            .Finish();

        HdContainerDataSourceHandle baseline = HdRetainedContainerDataSource::New(
            TfToken("A"), I(1),
            TfToken("B"), I(5),
            TfToken("C"), HdRetainedContainerDataSource::New(
                    TfToken("D"), I(3),
                    TfToken("E"), I(4)));

        COMPARECONTAINERS("two levels with override:", test, baseline);
    }


    {
        HdContainerDataSourceHandle test = HdContainerDataSourceEditor()
            .Set(L("A"), HdRetainedContainerDataSource::New(
                    TfToken("B"), I(1)))
            .Set(L("A/C"), I(2))
            .Set(L("A/D/E"), I(3))
            .Finish();

        HdContainerDataSourceHandle baseline =
            HdRetainedContainerDataSource::New(
                TfToken("A"), HdRetainedContainerDataSource::New(
                    TfToken("B"), I(1),
                    TfToken("C"), I(2),
                    TfToken("D"), HdRetainedContainerDataSource::New(
                        TfToken("E"), I(3))));

        COMPARECONTAINERS("set with container and then override:",
                    test, baseline);
    }

    {
        HdContainerDataSourceHandle subcontainer =
            HdContainerDataSourceEditor()
                .Set(L("B/C/E"), I(2))
                .Set(L("Z/Y"), I(3))
                .Finish();

        HdContainerDataSourceHandle test = HdContainerDataSourceEditor()
            .Set(L("A"), subcontainer)
            .Set(L("A/B/Q"), I(5))
            .Set(L("A/B/C/F"), I(6))
            .Set(L("A/Z/Y"), nullptr)
            .Finish();


        HdContainerDataSourceHandle baseline =
            HdRetainedContainerDataSource::New(
                TfToken("A"), HdRetainedContainerDataSource::New(
                    TfToken("B"), HdRetainedContainerDataSource::New(
                        TfToken("C"), HdRetainedContainerDataSource::New(
                            TfToken("E"), I(2),
                            TfToken("F"), I(6)),
                        TfToken("Q"), I(5)),
                        TfToken("Z"), HdRetainedContainerDataSource::New()));

        COMPARECONTAINERS("set with container, override deeply + delete:",
                    test, baseline);
    }

    {
        HdContainerDataSourceHandle initialContainer =
            HdContainerDataSourceEditor()
                .Set(L("A/B"), I(1))
                .Finish();

        HdContainerDataSourceHandle test =
            HdContainerDataSourceEditor(initialContainer)
                .Set(L("A/C"), I(2))
                .Set(L("D"), I(3))
                .Finish();

        HdContainerDataSourceHandle baseline =
            HdRetainedContainerDataSource::New(
                TfToken("A"), HdRetainedContainerDataSource::New(
                    TfToken("B"), I(1),
                    TfToken("C"), I(2)),
                TfToken("D"), I(3));

        COMPARECONTAINERS("initial container + overrides:", test, baseline);
    }

    {
        // Setting with a container data source masks the children of an
        // existing container on the editors initialContainer.

        // Confirm that A/B and A/C are not present after setting A directly
        // from a container.

        HdContainerDataSourceHandle initialContainer =
            HdContainerDataSourceEditor()
                .Set(L("A"),
                    HdRetainedContainerDataSource::New(
                        TfToken("B"), I(1),
                        TfToken("C"), I(2))
                    )
                .Finish();

        HdContainerDataSourceHandle test =
            HdContainerDataSourceEditor(initialContainer)
                .Set(L("A"),
                    HdRetainedContainerDataSource::New(
                        TfToken("D"), I(3),
                        TfToken("E"), I(4))
                        )
                .Finish();

        HdContainerDataSourceHandle baseline =
            HdContainerDataSourceEditor()
                .Set(L("A/D"), I(3))
                .Set(L("A/E"), I(4))
                .Finish();


        COMPARECONTAINERS("sub-container replacement + masking:",
            test, baseline);
    }

    {
        // Setting with a container data source masks the children of an
        // existing container on the editors initialContainer.

        // Confirm that A/B and A/C are not present after setting A directly
        // from a container.

        HdContainerDataSourceHandle initialContainer =
            HdContainerDataSourceEditor()
                .Set(L("A"),
                    HdRetainedContainerDataSource::New(
                        TfToken("B"), I(1),
                        TfToken("C"), I(2))
                    )
                .Finish();

        HdContainerDataSourceHandle subcontainer =
            HdContainerDataSourceEditor()
                .Set(L("D"), I(3))
                .Finish();

        HdContainerDataSourceHandle test =
            HdContainerDataSourceEditor(initialContainer)
                .Overlay(L("A"), subcontainer)
                .Finish();

        HdContainerDataSourceHandle baseline =
            HdContainerDataSourceEditor()
                .Set(L("A/B"), I(1))
                .Set(L("A/C"), I(2))
                .Set(L("A/D"), I(3))
                .Finish();

        COMPARECONTAINERS("sub-container overlay:",
            test, baseline);
    }

    return true;
}

#define xstr(s) str(s)
#define str(s) #s
#define TEST(X) std::cout << (++i) << ") " <<  str(X) << "..." << std::endl; \
if (!X()) { std::cout << "FAILED" << std::endl; return -1; } \
else std::cout << "...SUCCEEDED" << std::endl;

int main(int argc, char**argv)
{
    std::cout << "STARTING testHdContainerDataSourceEditor" << std::endl;
    // ------------------------------------------------------------------------

    int i = 0;
    TEST(TestSimpleOverlay);
    TEST(TestContainerEditor);

    // ------------------------------------------------------------------------
    std::cout << "DONE testHdContainerDataSourceEditor: SUCCESS" << std::endl;
    return 0;
}
