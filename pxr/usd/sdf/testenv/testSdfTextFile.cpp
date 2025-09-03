//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/fileIO.h"
#include "pxr/usd/sdf/fileVersion.h"
#include "pxr/usd/sdf/usdaFileFormat.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static std::string defVersionString;
static SdfFileVersion defVersion;

void InitDefVersion()
{
    // Just in case defVersion is formatted strangely, maybe with trailing
    // spaces or "1.00", or something, convert it to an SdfFileVersion and back
    // to a string.
    std::string str = TfGetEnvSetting(USD_WRITE_NEW_USDA_FILES_AS_VERSION);

    // Initialize the static values
    defVersion =  SdfFileVersion::FromString(str);
    defVersionString = defVersion.AsString();

    std::cout << "Testing with default version of "
              << defVersionString
              << std::endl;
}

bool TestHeader()
{
    bool ok = true;

    SdfFileFormatConstPtr usdaFormat = SdfFileFormat::FindByExtension("usda");
    std::string cookie = usdaFormat->GetFileCookie();

    std::string contents, expected;

    {
        // Test with no explicit version
        Sdf_StringOutput out;
        out.WriteHeader(cookie);

        contents = out.GetString();
        expected = cookie + " " + defVersionString + "\n";

        ok &= TF_VERIFY(contents == expected,
                        "WriteHeader failed.\n"
                        "    Contents: '%s'\n"
                        "    Expected: '%s'\n",
                        contents.c_str(), expected.c_str());
    }

    {
        // Test with explicit default version
        Sdf_StringOutput out;
        out.WriteHeader(cookie, defVersion);

        contents = out.GetString();
        expected = cookie + " " + defVersionString + "\n";

        ok &= TF_VERIFY(contents == expected,
                        "WriteHeader failed.\n"
                        "    Contents: '%s'\n"
                        "    Expected: '%s'\n",
                        contents.c_str(), expected.c_str());
    }


    {
        // Test with version 1.0
        SdfFileVersion ver{1, 0, 0};
        Sdf_StringOutput out;
        out.WriteHeader(cookie, ver);

        contents = out.GetString();
        expected = cookie + " " + ver.AsString() + "\n";

        ok &= TF_VERIFY(contents == expected,
                        "WriteHeader failed.\n"
                        "    Contents: '%s'\n"
                        "    Expected: '%s'\n",
                        contents.c_str(), expected.c_str());
    }

    {
        // Test with version 1.1
        SdfFileVersion ver{1, 1, 0};
        Sdf_StringOutput out;
        out.WriteHeader(cookie, ver);

        contents = out.GetString();
        expected = cookie + " " + ver.AsString() + "\n";

        ok &= TF_VERIFY(contents == expected,
                        "WriteHeader failed.\n"
                        "    Contents: '%s'\n"
                        "    Expected: '%s'\n",
                        contents.c_str(), expected.c_str());
    }

    return ok;
}

bool TestUpdate()
{
    bool ok = true;

    SdfFileFormatConstPtr usdaFormat = SdfFileFormat::FindByExtension("usda");
    std::string cookie = usdaFormat->GetFileCookie();
    std::string comment = " testSdfTextFile was here\n";

    std::string contents, expected;

    const SdfFileVersion ver110{1, 1, 0};
    const std::string ver110String = ver110.AsString();

    {
        // Test with no explicit version
        Sdf_StringOutput out;
        out.WriteHeader(cookie);
        out.Write(comment);
        out.RequestWriteVersionUpgrade(ver110,
                                       "Upgrading implicit default.");

        contents = out.GetString();
        expected = cookie + " " + ver110String + "\n" + comment;

        ok &= TF_VERIFY(contents == expected,
                        "WriteHeader failed.\n"
                        "    Contents: '%s'\n"
                        "    Expected: '%s'\n",
                        contents.c_str(), expected.c_str());
    }

    {
        // Test with explicit default version
        Sdf_StringOutput out;
        out.WriteHeader(cookie, defVersion);
        out.Write(comment);
        out.RequestWriteVersionUpgrade(ver110,
                                       "Upgrading explicit default.");

        contents = out.GetString();
        expected = cookie + " " + ver110String + "\n" + comment;

        ok &= TF_VERIFY(contents == expected,
                        "WriteHeader failed.\n"
                        "    Contents: '%s'\n"
                        "    Expected: '%s'\n",
                        contents.c_str(), expected.c_str());
    }


    {
        // Test with version 1.0
        SdfFileVersion ver{1, 0, 0};
        Sdf_StringOutput out;
        out.WriteHeader(cookie, ver);
        out.Write(comment);
        out.RequestWriteVersionUpgrade(ver110,
                                       "Upgrading explicit v{1, 0, 0}.");

        contents = out.GetString();
        expected = cookie + " " + ver110String + "\n" + comment;

        ok &= TF_VERIFY(contents == expected,
                        "WriteHeader failed.\n"
                        "    Contents: '%s'\n"
                        "    Expected: '%s'\n",
                        contents.c_str(), expected.c_str());
    }

    {
        // Test with version 1.1
        SdfFileVersion ver{1, 1, 0};
        Sdf_StringOutput out;
        out.WriteHeader(cookie, ver);
        out.Write(comment);
        out.RequestWriteVersionUpgrade(ver110,
                                       "Upgrading explicit v{1, 1, 0}.");

        contents = out.GetString();
        expected = cookie + " " + ver110String + "\n" + comment;

        ok &= TF_VERIFY(contents == expected,
                        "WriteHeader failed.\n"
                        "    Contents: '%s'\n"
                        "    Expected: '%s'\n",
                        contents.c_str(), expected.c_str());
    }


    return ok;
}


int
main(int argc, char** argv)
{
    bool ok = true;

    InitDefVersion();

    ok &= TF_VERIFY(TestHeader(), "TestHeader failed.");
    ok &= TF_VERIFY(TestUpdate(), "TestUpdate failed.");

    return (ok ? 0 : 1);
}
