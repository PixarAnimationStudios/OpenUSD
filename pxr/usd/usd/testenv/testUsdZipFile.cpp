//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

        TF_AXIOM(*i == "a.test"); // Test operator*
        TF_AXIOM(strcmp(i->c_str(), "a.test") == 0); // Test operator->
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
