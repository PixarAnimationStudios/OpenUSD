//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/fileVersion.h"

#include "pxr/base/tf/diagnostic.h"

#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

void TestRelops(const std::vector<SdfFileVersion>& versions)
{
    // Given a list of sorted versions, make sure all the relops are true
    // by comparing every item against every other item.
    for (size_t i = 0; i < versions.size(); ++i) {
        for (size_t j = 0; j < versions.size(); ++j) {
            TF_AXIOM((versions[i] <  versions[j]) == (i <  j));
            TF_AXIOM((versions[i] <= versions[j]) == (i <= j));
            TF_AXIOM((versions[i] == versions[j]) == (i == j));
            TF_AXIOM((versions[i] != versions[j]) == (i != j));
            TF_AXIOM((versions[i] >= versions[j]) == (i >= j));
            TF_AXIOM((versions[i] >  versions[j]) == (i >  j));
        }
    }
}

int 
main(int argc, char** argv)
{
    // Default and explicit constructors
    SdfFileVersion ver000;
    SdfFileVersion ver100(1, 0, 0);
    SdfFileVersion ver101(1, 0, 1);
    SdfFileVersion ver110(1, 1, 0);
    SdfFileVersion ver123(1, 2, 3);
    SdfFileVersion ver321(3, 2, 1);

    // CrateFile::_BootStrap class has a version that is uint8_t[8]. Test
    // constructing from that.
    using BootVersion = uint8_t[8];
    //typedef uint8_t BootVersion[8];
    BootVersion boot000{0, 0, 0, 0, 0, 0, 0, 0};
    BootVersion boot100{1, 0, 0, 4, 5, 6, 7, 8};
    BootVersion boot101{1, 0, 1, 4, 5, 6, 7, 8};
    BootVersion boot110{1, 1, 0, 4, 5, 6, 7, 8};
    BootVersion boot123{1, 2, 3, 4, 5, 6, 7, 8};
    BootVersion boot321{3, 2, 1, 4, 5, 6, 7, 8};
    
    // Verify the default constructor sets everything to 0
    TF_AXIOM(ver000.AsInt() == 0);
    TF_AXIOM(ver000 == SdfFileVersion(0, 0, 0));
    TF_AXIOM(ver000.majver == 0);
    TF_AXIOM(ver000.minver == 0);
    TF_AXIOM(ver000.patchver == 0);
    TF_AXIOM(!ver000.IsValid());
    TF_AXIOM(!ver000);

    // Verify the explicit constructor
    TF_AXIOM(ver123.AsInt() == 0x010203);
    TF_AXIOM(ver123 == SdfFileVersion(1, 2, 3));
    TF_AXIOM(ver123.majver == 1);
    TF_AXIOM(ver123.minver == 2);
    TF_AXIOM(ver123.patchver == 3);
    TF_AXIOM(ver123.IsValid());
    TF_AXIOM(ver123);

    // Verify the "BootVersion" constructor
    TF_AXIOM(SdfFileVersion(boot000) == ver000);
    TF_AXIOM(SdfFileVersion(boot100) == ver100);
    TF_AXIOM(SdfFileVersion(boot101) == ver101);
    TF_AXIOM(SdfFileVersion(boot110) == ver110);
    TF_AXIOM(SdfFileVersion(boot123) == ver123);
    TF_AXIOM(SdfFileVersion(boot321) == ver321);

    // Verify AsString()
    TF_AXIOM(ver000.AsString() == "0.0");
    TF_AXIOM(ver100.AsString() == "1.0");
    TF_AXIOM(ver101.AsString() == "1.0.1");
    TF_AXIOM(ver110.AsString() == "1.1");
    TF_AXIOM(ver123.AsString() == "1.2.3");

    // Verify AsFullString();
    TF_AXIOM(ver000.AsFullString() == "0.0.0");
    TF_AXIOM(ver100.AsFullString() == "1.0.0");
    TF_AXIOM(ver101.AsFullString() == "1.0.1");
    TF_AXIOM(ver110.AsFullString() == "1.1.0");
    TF_AXIOM(ver123.AsFullString() == "1.2.3");

    // Verify FromString()
    TF_AXIOM(SdfFileVersion::FromString("1.0")   == ver100);
    TF_AXIOM(SdfFileVersion::FromString("1.0.0") == ver100);
    TF_AXIOM(SdfFileVersion::FromString("1.0.1") == ver101);
    TF_AXIOM(SdfFileVersion::FromString("1.1")   == ver110);
    TF_AXIOM(SdfFileVersion::FromString("1.1.0") == ver110);
    TF_AXIOM(SdfFileVersion::FromString("1.2.3") == ver123);

    // Trailing whitespace is legal and ignored.
    TF_AXIOM(SdfFileVersion::FromString("1.0   ") == ver100);
    TF_AXIOM(SdfFileVersion::FromString("1.0.0 ") == ver100);
    TF_AXIOM(SdfFileVersion::FromString("1.0.1 ") == ver101);
    TF_AXIOM(SdfFileVersion::FromString("1.1   ") == ver110);
    TF_AXIOM(SdfFileVersion::FromString("1.1.0 ") == ver110);
    TF_AXIOM(SdfFileVersion::FromString("1.2.3 ") == ver123);

    // Verify CanRead()
    TF_AXIOM(ver100.CanRead(ver100));
    TF_AXIOM(ver100.CanRead(ver101));
    TF_AXIOM(ver101.CanRead(ver100));
    TF_AXIOM(ver110.CanRead(ver101));
    TF_AXIOM(!ver101.CanRead(ver110));

    // Verify CanWrite()
    TF_AXIOM(ver100.CanWrite(ver100));
    TF_AXIOM(!ver100.CanWrite(ver101));
    TF_AXIOM(ver101.CanWrite(ver100));
    TF_AXIOM(ver110.CanWrite(ver101));
    TF_AXIOM(!ver101.CanWrite(ver110));

    TestRelops({ver000, ver100, ver101, ver110, ver123, ver321});

    // Test failure cases. SdfFileVersion does not report any errors, it simply
    // returns an invalid version given invalid inputs.
    TF_AXIOM(!SdfFileVersion::FromString("Hello world"));
    TF_AXIOM(!SdfFileVersion::FromString("1.0a"));
    TF_AXIOM(!SdfFileVersion::FromString("1.0."));
    TF_AXIOM(!SdfFileVersion::FromString("1.0_"));
    TF_AXIOM(!SdfFileVersion::FromString("1.0_stuff"));
    TF_AXIOM(!SdfFileVersion::FromString("1.0_stuff"));
    TF_AXIOM(!SdfFileVersion::FromString("1.0.xyz"));
    TF_AXIOM(SdfFileVersion::FromString("3.14"));
    TF_AXIOM(SdfFileVersion::FromString("3.141"));
    TF_AXIOM(!SdfFileVersion::FromString("3.1416"));

    printf(">>> Test PASSED\n");
    return 0;
}
