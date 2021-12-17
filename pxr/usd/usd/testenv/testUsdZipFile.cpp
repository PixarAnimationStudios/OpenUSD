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

#include "pxr/pxr.h"

#include "pxr/usd/usd/zipFile.h"
#include "pxr/base/tf/diagnosticLite.h"

#include <string>
#include <cstring>

PXR_NAMESPACE_USING_DIRECTIVE

static void
TestIterators()
{
    UsdZipFile zipFile = UsdZipFile::Open("test_reader.usdz");
    TF_AXIOM(zipFile);

    // Test various operators.
    {
        UsdZipFile::Iterator i = zipFile.begin();
        TF_AXIOM(i != UsdZipFile::Iterator());

        UsdZipFile::Iterator j = i;
        TF_AXIOM(i == j);
        ++j;
        TF_AXIOM(i != j);
        i++;
        TF_AXIOM(i == j);

        UsdZipFile::Iterator k = std::move(i);
        TF_AXIOM(j == k);

        UsdZipFile::Iterator l(std::move(j));
        TF_AXIOM(k == l);
    }

    // Test iterating over files in zip archive.
    {
        UsdZipFile::Iterator i = zipFile.begin(), e = zipFile.end();
        TF_AXIOM(std::distance(i, e) == 4);

        TF_AXIOM(*i == "a.txt"); // Test operator*
        TF_AXIOM(strcmp(i->c_str(), "a.txt") == 0); // Test operator->
        TF_AXIOM(i == std::next(zipFile.begin(), 0));
        TF_AXIOM(i != std::next(zipFile.begin(), 1));
        TF_AXIOM(i != std::next(zipFile.begin(), 2));
        TF_AXIOM(i != std::next(zipFile.begin(), 3));
        TF_AXIOM(i != std::next(zipFile.begin(), 4));
        ++i;

        TF_AXIOM(*i == "b.png");
        TF_AXIOM(strcmp(i->c_str(), "b.png") == 0);
        TF_AXIOM(i != std::next(zipFile.begin(), 0));
        TF_AXIOM(i == std::next(zipFile.begin(), 1));
        TF_AXIOM(i != std::next(zipFile.begin(), 2));
        TF_AXIOM(i != std::next(zipFile.begin(), 3));
        TF_AXIOM(i != std::next(zipFile.begin(), 4));
        ++i;

        TF_AXIOM(*i == "sub/c.png");
        TF_AXIOM(strcmp(i->c_str(), "sub/c.png") == 0);
        TF_AXIOM(i != std::next(zipFile.begin(), 0));
        TF_AXIOM(i != std::next(zipFile.begin(), 1));
        TF_AXIOM(i == std::next(zipFile.begin(), 2));
        TF_AXIOM(i != std::next(zipFile.begin(), 3));
        TF_AXIOM(i != std::next(zipFile.begin(), 4));
        ++i;

        TF_AXIOM(*i == "sub/d.txt");
        TF_AXIOM(strcmp(i->c_str(), "sub/d.txt") == 0);
        TF_AXIOM(i != std::next(zipFile.begin(), 0));
        TF_AXIOM(i != std::next(zipFile.begin(), 1));
        TF_AXIOM(i != std::next(zipFile.begin(), 2));
        TF_AXIOM(i == std::next(zipFile.begin(), 3));
        TF_AXIOM(i != std::next(zipFile.begin(), 4));
        ++i;

        TF_AXIOM(i != std::next(zipFile.begin(), 0));
        TF_AXIOM(i != std::next(zipFile.begin(), 1));
        TF_AXIOM(i != std::next(zipFile.begin(), 2));
        TF_AXIOM(i != std::next(zipFile.begin(), 3));
        TF_AXIOM(i == std::next(zipFile.begin(), 4));
        TF_AXIOM(i == e);
    }
}

int main(int argc, char** argv)
{
    TestIterators();

    return 0;
}
