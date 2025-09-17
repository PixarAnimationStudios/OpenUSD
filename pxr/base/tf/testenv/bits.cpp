//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/bits.h"
#include "pxr/base/tf/regTest.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static void
TestSwap(
    const TfBits & a, TfBits & b)
{
    // Make copies of a & b so we can test swapping in both directions.
    TfBits a1(a), a2(a), b1(b), b2(b);

    b1.Swap(a1);
    TF_AXIOM(a1 == b);
    TF_AXIOM(b1 == a);

    a2.Swap(b2);
    TF_AXIOM(a2 == b);
    TF_AXIOM(b2 == a);

    // Swap back
    b1.Swap(a1);
    TF_AXIOM(a1 == a);
    TF_AXIOM(b1 == b);

    a2.Swap(b2);
    TF_AXIOM(a2 == a);
    TF_AXIOM(b2 == b);
}

static bool
Test_TfBits()
{
    std::cout
        << "Testing TfBits...\n"
        << "sizeof(TfBits) = " << sizeof(TfBits) << "\n";

    TfBits b(4);

    TF_AXIOM(b.GetSize() == 4);
    TF_AXIOM(b.GetNumSet() == 0);
    TF_AXIOM(!b.AreAllSet());
    TF_AXIOM(b.AreAllUnset());
    TF_AXIOM(!b.AreContiguouslySet());

    // Test setting a single bit.
    b.Set(0);
    TF_AXIOM(b.GetSize() == 4);
    TF_AXIOM(b.GetNumSet() == 1);
    TF_AXIOM(!b.AreAllSet());
    TF_AXIOM(!b.AreAllUnset());
    TF_AXIOM(b.AreContiguouslySet());

    TF_AXIOM(b.GetAsStringLeftToRight() == "1000");
    TF_AXIOM(b.GetAsStringRightToLeft() == "0001");

    // Test resize /w keeping content.
    b.ResizeKeepContent(8);
    TF_AXIOM(b.GetSize() == 8);
    TF_AXIOM(b.GetNumSet() == 1);
    TF_AXIOM(!b.AreAllSet());
    TF_AXIOM(!b.AreAllUnset());
    TF_AXIOM(b.AreContiguouslySet());

    TF_AXIOM(b.GetAsStringLeftToRight() == "10000000");
    TF_AXIOM(b.GetAsStringRightToLeft() == "00000001");

    // Test resize /w keeping content.
    b.ResizeKeepContent(2);
    TF_AXIOM(b.GetSize() == 2);
    TF_AXIOM(b.GetNumSet() == 1);
    TF_AXIOM(!b.AreAllSet());
    TF_AXIOM(!b.AreAllUnset());
    TF_AXIOM(b.AreContiguouslySet());

    TF_AXIOM(b.GetAsStringLeftToRight() == "10");
    TF_AXIOM(b.GetAsStringRightToLeft() == "01");

    {
        // Testing Assign() API
        TfBits a(4);
        a.ClearAll();
        a.Set(1);
        TF_AXIOM(a.GetAsStringLeftToRight() == "0100");

        a.Assign(2, true);
        TF_AXIOM(a.GetAsStringLeftToRight() == "0110");
        TF_AXIOM(a.GetNumSet() == 2);
        TF_AXIOM(a.GetFirstSet() == 1);
        TF_AXIOM(a.GetLastSet() == 2);

        a.Assign(2, false);
        TF_AXIOM(a.GetAsStringLeftToRight() == "0100");
        TF_AXIOM(a.GetNumSet() == 1);
        TF_AXIOM(a.GetFirstSet() == 1);
        TF_AXIOM(a.GetLastSet() == 1);

        a.Assign(3, false);
        TF_AXIOM(a.GetAsStringLeftToRight() == "0100");
        TF_AXIOM(a.GetNumSet() == 1);
        TF_AXIOM(a.GetFirstSet() == 1);
        TF_AXIOM(a.GetLastSet() == 1);

        TfBits t;
        t.Resize(12);
        t.ClearAll();
        t.Assign(1, true);
        t.Assign(2, true);
        TF_AXIOM(t.GetAsStringLeftToRight() == "011000000000");
        t.Assign(4, false);
        t.Assign(5, true);
        TF_AXIOM(t.GetAsStringLeftToRight() == "011001000000");
        TF_AXIOM(t.GetNumSet() == 3);
        TF_AXIOM(t.GetFirstSet() == 1);
        TF_AXIOM(t.GetLastSet() == 5);
    }

    {
        // Test resizing bug: GetFirstSet() won't work after ResizeKeepContent().
        b.Resize(0);
        TF_AXIOM(b.GetFirstSet() == 0);
        b.ResizeKeepContent(4);
        b.Assign(3, true);
        TF_AXIOM(b.GetFirstSet() == 3);
    }

    {
        // Test basic iterator views.
        b.Assign(1, true);
        TF_AXIOM(b.GetAsStringLeftToRight() == "0101");
    
        size_t c=0;
        TF_FOR_ALL(i, b.GetAllView())
            c += *i;
        TF_AXIOM(c == 6);
    
        c=0;
        TF_FOR_ALL(i, b.GetAllSetView())
            c += *i;
        TF_AXIOM(c == 4);
    
        c=0;
        TF_FOR_ALL(i, b.GetAllUnsetView())
            c += *i;
        TF_AXIOM(c == 2);
    }

    // Testing Swap
    {
        // 'a' & 'b' are both small enough to both use inline bits storage.
        TfBits a(4), b(2);
        a.Set(0);
        b.Set(1);

        TestSwap(a, b);
    }
    {
        // 'a' & 'b' are both large enough to both use heap allocated bits
        // storage.
        TfBits a(2048), b(1024);
        a.Set(0);
        b.Set(512);

        TestSwap(a, b);
    }
    {
        // 'a' uses inline bits storage, 'b' uses heap allocated storage.
        TfBits a(4), b(1024);
        a.Set(0);
        b.Set(512);

        TestSwap(a, b);
    }

    //XXX: Needs more testing.

    std::cout << "... success!\n\n";

    return true;
}
TF_ADD_REGTEST(TfBits);

