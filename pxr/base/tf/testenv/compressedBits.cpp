//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/bits.h"
#include "pxr/base/tf/compressedBits.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/stopwatch.h"
#include "pxr/base/tf/iterator.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

// This verifies that the TfBits and TfCompressedBits APIs remain compatible
static void
Test_TfCompressedBits_VerifyEquality(
    const TfBits &a,
    const TfCompressedBits &b)
{
    // Verify that the TfBits and TfCompressedBits APIs return equal values
    TF_AXIOM(a.GetSize() == b.GetSize());
    TF_AXIOM(a.GetFirstSet() == b.GetFirstSet());
    TF_AXIOM(a.GetLastSet() == b.GetLastSet());
    TF_AXIOM(a.GetNumSet() == b.GetNumSet());
    TF_AXIOM(a.AreAllSet() == b.AreAllSet());
    TF_AXIOM(a.AreAllUnset() == b.AreAllUnset());
    TF_AXIOM(a.IsAnySet() == b.IsAnySet());
    TF_AXIOM(a.IsAnyUnset() == b.IsAnyUnset());
    TF_AXIOM(a.AreContiguouslySet() == b.AreContiguouslySet());
    TF_AXIOM(a.GetAsStringLeftToRight() == b.GetAsStringLeftToRight());

    // Value equality
    for (size_t i = 0; i < a.GetSize(); ++i) {
        TF_AXIOM(a.IsSet(i) == b.IsSet(i));
    }
}

// This verifies that the TfBits and TfCompressedBits APIs remain compatible,
// and that converting between the two representations results in equal values
static void
Test_TfCompressedBits_VerifyEqualityWithConversion(
    const TfBits &a,
    const TfCompressedBits &b)
{
    Test_TfCompressedBits_VerifyEquality(a, b);

    TfCompressedBits c = TfCompressedBits(a);
    Test_TfCompressedBits_VerifyEquality(a, c);

    TfBits d;
    b.Decompress(&d);
    Test_TfCompressedBits_VerifyEquality(d, b);
}

static bool
Test_TfCompressedBits()
{
    std::cout
        << "Testing TfCompressedBits...\n"
        << "sizeof(TfCompressedBits) = " << sizeof(TfCompressedBits) << "\n";

    // Basic API tests
    {
        TfCompressedBits b(4);

        TF_AXIOM(b.GetSize() == 4);
        TF_AXIOM(b.GetNumSet() == 0);
        TF_AXIOM(!b.AreAllSet());
        TF_AXIOM(b.AreAllUnset());
        TF_AXIOM(!b.IsAnySet());
        TF_AXIOM(b.IsAnyUnset());
        TF_AXIOM(!b.AreContiguouslySet());
        TF_AXIOM(b.GetFirstSet() == b.GetSize());
        TF_AXIOM(b.GetLastSet() == b.GetSize());

        // Test setting a single bit.
        b.Set(0);
        TF_AXIOM(b.IsSet(0));
        TF_AXIOM(b.GetSize() == 4);
        TF_AXIOM(b.GetNumSet() == 1);
        TF_AXIOM(!b.AreAllSet());
        TF_AXIOM(!b.AreAllUnset());
        TF_AXIOM(b.IsAnySet());
        TF_AXIOM(b.IsAnyUnset());
        TF_AXIOM(b.AreContiguouslySet());
        TF_AXIOM(b.GetFirstSet() == 0);
        TF_AXIOM(b.GetLastSet() == 0);
        TF_AXIOM(b.GetAsStringLeftToRight() == "1000");
        TF_AXIOM(b.GetAsStringRightToLeft() == "0001");

        // Test setting a second bit
        b.Set(2);
        TF_AXIOM(b.IsSet(0));
        TF_AXIOM(b.IsSet(2));
        TF_AXIOM(b.GetSize() == 4);
        TF_AXIOM(b.GetNumSet() == 2);
        TF_AXIOM(!b.AreAllSet());
        TF_AXIOM(!b.AreAllUnset());
        TF_AXIOM(b.IsAnySet());
        TF_AXIOM(b.IsAnyUnset());
        TF_AXIOM(!b.AreContiguouslySet());
        TF_AXIOM(b.GetFirstSet() == 0);
        TF_AXIOM(b.GetLastSet() == 2);
        TF_AXIOM(b.GetAsStringLeftToRight() == "1010");
        TF_AXIOM(b.GetAsStringRightToLeft() == "0101");

        // Test setting a third bit
        b.Assign(1, true);
        TF_AXIOM(b.IsSet(0));
        TF_AXIOM(b.IsSet(1));
        TF_AXIOM(b.IsSet(2));
        TF_AXIOM(b.GetSize() == 4);
        TF_AXIOM(b.GetNumSet() == 3);
        TF_AXIOM(!b.AreAllSet());
        TF_AXIOM(!b.AreAllUnset());
        TF_AXIOM(b.IsAnySet());
        TF_AXIOM(b.IsAnyUnset());
        TF_AXIOM(b.AreContiguouslySet());
        TF_AXIOM(b.GetFirstSet() == 0);
        TF_AXIOM(b.GetLastSet() == 2);
        TF_AXIOM(b.GetAsStringLeftToRight() == "1110");
        TF_AXIOM(b.GetAsStringRightToLeft() == "0111");

        // Test setting all bits
        b.SetAll();
        TF_AXIOM(b.IsSet(0));
        TF_AXIOM(b.IsSet(1));
        TF_AXIOM(b.IsSet(2));
        TF_AXIOM(b.IsSet(3));
        TF_AXIOM(b.GetSize() == 4);
        TF_AXIOM(b.GetNumSet() == 4);
        TF_AXIOM(b.AreAllSet());
        TF_AXIOM(!b.AreAllUnset());
        TF_AXIOM(b.IsAnySet());
        TF_AXIOM(!b.IsAnyUnset());
        TF_AXIOM(b.AreContiguouslySet());
        TF_AXIOM(b.GetFirstSet() == 0);
        TF_AXIOM(b.GetLastSet() == 3);
        TF_AXIOM(b.GetAsStringLeftToRight() == "1111");
        TF_AXIOM(b.GetAsStringRightToLeft() == "1111");

        // Test unsetting a bit
        b.Assign(0, false);
        TF_AXIOM(!b.IsSet(0));
        TF_AXIOM(b.IsSet(1));
        TF_AXIOM(b.IsSet(2));
        TF_AXIOM(b.IsSet(3));
        TF_AXIOM(b.GetSize() == 4);
        TF_AXIOM(b.GetNumSet() == 3);
        TF_AXIOM(!b.AreAllSet());
        TF_AXIOM(!b.AreAllUnset());
        TF_AXIOM(b.IsAnySet());
        TF_AXIOM(b.IsAnyUnset());
        TF_AXIOM(b.AreContiguouslySet());
        TF_AXIOM(b.GetFirstSet() == 1);
        TF_AXIOM(b.GetLastSet() == 3);
        TF_AXIOM(b.GetAsStringLeftToRight() == "0111");
        TF_AXIOM(b.GetAsStringRightToLeft() == "1110");

        // Test unsetting another bit
        b.Clear(2);
        TF_AXIOM(!b.IsSet(0));
        TF_AXIOM(b.IsSet(1));
        TF_AXIOM(!b.IsSet(2));
        TF_AXIOM(b.IsSet(3));
        TF_AXIOM(b.GetSize() == 4);
        TF_AXIOM(b.GetNumSet() == 2);
        TF_AXIOM(!b.AreAllSet());
        TF_AXIOM(!b.AreAllUnset());
        TF_AXIOM(b.IsAnySet());
        TF_AXIOM(b.IsAnyUnset());
        TF_AXIOM(!b.AreContiguouslySet());
        TF_AXIOM(b.GetFirstSet() == 1);
        TF_AXIOM(b.GetLastSet() == 3);
        TF_AXIOM(b.GetAsStringLeftToRight() == "0101");
        TF_AXIOM(b.GetAsStringRightToLeft() == "1010");

        // Test unsetting all bits
        b.ClearAll();
        TF_AXIOM(b.GetSize() == 4);
        TF_AXIOM(b.GetNumSet() == 0);
        TF_AXIOM(!b.AreAllSet());
        TF_AXIOM(b.AreAllUnset());
        TF_AXIOM(!b.IsAnySet());
        TF_AXIOM(b.IsAnyUnset());
        TF_AXIOM(!b.AreContiguouslySet());
        TF_AXIOM(b.GetFirstSet() == b.GetSize());
        TF_AXIOM(b.GetLastSet() == b.GetSize());
        TF_AXIOM(b.GetAsStringLeftToRight() == "0000");
        TF_AXIOM(b.GetAsStringRightToLeft() == "0000");

        // Test setting a range of bits
        b.SetRange(1, 3);
        TF_AXIOM(!b.IsSet(0));
        TF_AXIOM(b.IsSet(1));
        TF_AXIOM(b.IsSet(2));
        TF_AXIOM(b.IsSet(3));
        TF_AXIOM(b.GetSize() == 4);
        TF_AXIOM(b.GetNumSet() == 3);
        TF_AXIOM(!b.AreAllSet());
        TF_AXIOM(!b.AreAllUnset());
        TF_AXIOM(b.IsAnySet());
        TF_AXIOM(b.IsAnyUnset());
        TF_AXIOM(b.AreContiguouslySet());
        TF_AXIOM(b.GetFirstSet() == 1);
        TF_AXIOM(b.GetLastSet() == 3);
        TF_AXIOM(b.GetAsStringLeftToRight() == "0111");
        TF_AXIOM(b.GetAsStringRightToLeft() == "1110");

        // Set a bit that's already set
        b.Set(1);
        TF_AXIOM(!b.IsSet(0));
        TF_AXIOM(b.IsSet(1));
        TF_AXIOM(b.IsSet(2));
        TF_AXIOM(b.IsSet(3));
        TF_AXIOM(b.GetSize() == 4);
        TF_AXIOM(b.GetNumSet() == 3);
        TF_AXIOM(!b.AreAllSet());
        TF_AXIOM(!b.AreAllUnset());
        TF_AXIOM(b.IsAnySet());
        TF_AXIOM(b.IsAnyUnset());
        TF_AXIOM(b.AreContiguouslySet());
        TF_AXIOM(b.GetFirstSet() == 1);
        TF_AXIOM(b.GetLastSet() == 3);
        TF_AXIOM(b.GetAsStringLeftToRight() == "0111");
        TF_AXIOM(b.GetAsStringRightToLeft() == "1110");

        // Clear a bit that's already cleared
        b.Clear(0);
        TF_AXIOM(!b.IsSet(0));
        TF_AXIOM(b.IsSet(1));
        TF_AXIOM(b.IsSet(2));
        TF_AXIOM(b.IsSet(3));
        TF_AXIOM(b.GetSize() == 4);
        TF_AXIOM(b.GetNumSet() == 3);
        TF_AXIOM(!b.AreAllSet());
        TF_AXIOM(!b.AreAllUnset());
        TF_AXIOM(b.IsAnySet());
        TF_AXIOM(b.IsAnyUnset());
        TF_AXIOM(b.AreContiguouslySet());
        TF_AXIOM(b.GetFirstSet() == 1);
        TF_AXIOM(b.GetLastSet() == 3);
        TF_AXIOM(b.GetAsStringLeftToRight() == "0111");
        TF_AXIOM(b.GetAsStringRightToLeft() == "1110");

        // Append bits
        TfCompressedBits c;
        TF_AXIOM(c.GetSize() == 0);
        TF_AXIOM(c.GetNumSet() == 0);
        TF_AXIOM(c.GetAsStringLeftToRight() == "");

        c.Append(2, false);
        TF_AXIOM(c.GetSize() == 2);
        TF_AXIOM(c.GetNumSet() == 0);
        TF_AXIOM(c.GetAsStringLeftToRight() == "00");

        c.Append(1, false);
        TF_AXIOM(c.GetSize() == 3);
        TF_AXIOM(c.GetNumSet() == 0);
        TF_AXIOM(c.GetAsStringLeftToRight() == "000");

        c.Append(2, true);
        TF_AXIOM(c.GetSize() == 5);
        TF_AXIOM(c.GetNumSet() == 2);
        TF_AXIOM(c.GetAsStringLeftToRight() == "00011");

        c.Append(1, true);
        TF_AXIOM(c.GetSize() == 6);
        TF_AXIOM(c.GetNumSet() == 3);
        TF_AXIOM(c.GetAsStringLeftToRight() == "000111");

        c.Append(3, false);
        TF_AXIOM(c.GetSize() == 9);
        TF_AXIOM(c.GetNumSet() == 3);
        TF_AXIOM(c.GetAsStringLeftToRight() == "000111000");

        c = TfCompressedBits();
        TF_AXIOM(c.GetSize() == 0);
        TF_AXIOM(c.GetNumSet() == 0);
        TF_AXIOM(c.GetAsStringLeftToRight() == "");

        c.Append(3, true);
        TF_AXIOM(c.GetSize() == 3);
        TF_AXIOM(c.GetNumSet() == 3);
        TF_AXIOM(c.GetAsStringLeftToRight() == "111");

        TfCompressedBits d(3);
        d.SetAll();
        TF_AXIOM(c == d);
    }

    // Basic logic operations
    {
        TfCompressedBits a(4);
        a.SetAll();

        TfCompressedBits b(4);

        // AND
        {
            TfCompressedBits c(4);
            c = a & b;
            TF_AXIOM(c.AreAllUnset());
            TF_AXIOM(c.GetNumSet() == 0);
            TF_AXIOM(c.GetAsStringLeftToRight() == "0000");

            c.Set(0);
            c.Set(1);
            TF_AXIOM(c.GetNumSet() == 2);
            TF_AXIOM(c.GetAsStringLeftToRight() == "1100");

            c &= a;
            TF_AXIOM(c.GetNumSet() == 2);
            TF_AXIOM(c.GetAsStringLeftToRight() == "1100");

            TfCompressedBits d(a);
            d.Clear(0);
            d.Clear(2);
            TF_AXIOM(d.GetNumSet() == 2);
            TF_AXIOM(d.GetAsStringLeftToRight() == "0101");

            c.Set(3);
            TF_AXIOM(c.GetNumSet() == 3);
            TF_AXIOM(c.GetAsStringLeftToRight() == "1101");

            d &= c;
            TF_AXIOM(d.GetNumSet() == 2);
            TF_AXIOM(d.GetAsStringLeftToRight() == "0101");
        }

        // OR
        {
            TfCompressedBits c(4);
            c = a | b;
            TF_AXIOM(c.AreAllSet());
            TF_AXIOM(c.GetNumSet() == 4);
            TF_AXIOM(c.GetAsStringLeftToRight() == "1111");

            c.Clear(0);
            c.Clear(1);
            TF_AXIOM(c.GetNumSet() == 2);
            TF_AXIOM(c.GetAsStringLeftToRight() == "0011");

            c |= a;
            TF_AXIOM(c.GetNumSet() == 4);
            TF_AXIOM(c.GetAsStringLeftToRight() == "1111");

            TfCompressedBits d(a);
            d.Clear(0);
            d.Clear(2);
            TF_AXIOM(d.GetNumSet() == 2);
            TF_AXIOM(d.GetAsStringLeftToRight() == "0101");

            c.Clear(0);
            c.Clear(3);
            TF_AXIOM(c.GetNumSet() == 2);
            TF_AXIOM(c.GetAsStringLeftToRight() == "0110");

            d |= c;
            TF_AXIOM(d.GetNumSet() == 3);
            TF_AXIOM(d.GetAsStringLeftToRight() == "0111");
        }

        // XOR
        {
            TfCompressedBits c(4);
            c = a ^ b;
            TF_AXIOM(c.AreAllSet());
            TF_AXIOM(c.GetNumSet() == 4);
            TF_AXIOM(c.GetAsStringLeftToRight() == "1111");

            c.Clear(0);
            c.Clear(1);
            TF_AXIOM(c.GetNumSet() == 2);
            TF_AXIOM(c.GetAsStringLeftToRight() == "0011");

            c ^= a;
            TF_AXIOM(c.GetNumSet() == 2);
            TF_AXIOM(c.GetAsStringLeftToRight() == "1100");
        }

        // Complement
        {
            TfCompressedBits c(4);
            a.Complement();
            c = a;
            TF_AXIOM(c.AreAllUnset());
            TF_AXIOM(c.GetNumSet() == 0);
            TF_AXIOM(c.GetAsStringLeftToRight() == "0000");

            b.Complement();
            c = b;
            TF_AXIOM(c.AreAllSet());
            TF_AXIOM(c.GetNumSet() == 4);
            TF_AXIOM(c.GetAsStringLeftToRight() == "1111");

            c.Clear(0);
            c.Clear(2);
            TF_AXIOM(c.GetNumSet() == 2);
            TF_AXIOM(c.GetAsStringLeftToRight() == "0101");

            c.Complement();
            TF_AXIOM(c.GetNumSet() == 2);
            TF_AXIOM(c.GetAsStringLeftToRight() == "1010");
        }

        // Subtraction
        {
            TfCompressedBits c(4);
            c.SetAll();
            TF_AXIOM(c.GetNumSet() == 4);
            TF_AXIOM(c.GetAsStringLeftToRight() == "1111");

            TfCompressedBits d(4);
            d.ClearAll();
            TF_AXIOM(d.GetNumSet() == 0);
            TF_AXIOM(d.GetAsStringLeftToRight() == "0000");

            c -= d;
            TF_AXIOM(c.GetNumSet() == 4);
            TF_AXIOM(c.GetAsStringLeftToRight() == "1111");

            d -= c;
            TF_AXIOM(d.GetNumSet() == 0);
            TF_AXIOM(d.GetAsStringLeftToRight() == "0000");

            d.Set(0);
            TF_AXIOM(d.GetNumSet() == 1);
            TF_AXIOM(d.GetAsStringLeftToRight() == "1000");

            d -= c;
            TF_AXIOM(d.GetNumSet() == 0);
            TF_AXIOM(d.GetAsStringLeftToRight() == "0000");

            d.Set(0);
            d.Set(2);
            TF_AXIOM(d.GetNumSet() == 2);
            TF_AXIOM(d.GetAsStringLeftToRight() == "1010");

            d -= c;
            TF_AXIOM(d.GetNumSet() == 0);
            TF_AXIOM(d.GetAsStringLeftToRight() == "0000");

            d.Set(0);
            d.Set(3);
            TF_AXIOM(d.GetNumSet() == 2);
            TF_AXIOM(d.GetAsStringLeftToRight() == "1001");

            c -= d;
            TF_AXIOM(c.GetNumSet() == 2);
            TF_AXIOM(c.GetAsStringLeftToRight() == "0110");

            d.SetAll();
            c -= d;
            TF_AXIOM(c.GetNumSet() == 0);
            TF_AXIOM(c.GetAsStringLeftToRight() == "0000");
        }


        // Extra logic operations, compared against TfBits.
        {
            auto cc = TF_CALL_CONTEXT;
            unsigned seed = std::time(0);
            srand(seed);

            printf("Random seed is %u -- to debug, hard code this value around "
                   "line %zu in %s\n", seed, cc.GetLine(), cc.GetFile());

            auto verifyEqual = [](char const *expr,
                                  TfBits const &bits,
                                  TfCompressedBits const &cbits) {
                TF_VERIFY(bits.GetAsStringLeftToRight() ==
                          cbits.GetAsStringLeftToRight(),
                          "%s -- bits: %s != compressed bits: %s",
                          expr,
                          bits.GetAsStringLeftToRight().c_str(),
                          cbits.GetAsStringLeftToRight().c_str());
            };

            auto checkBits = [&verifyEqual]() {
                TfBits a, b;
                TfCompressedBits ca, cb;
                size_t sz = rand() % 128;
                if (sz == 0) {
                    sz = 1;
                }
                int nSets = rand() % sz;
                a = TfBits(sz);
                b = TfBits(sz);
                ca = TfCompressedBits(sz);
                cb = TfCompressedBits(sz);

                verifyEqual("clear1", a, ca);
                verifyEqual("clear2", b, cb);

                for (int i = 0; i != nSets; ++i) {
                    int index = rand() % sz;
                    a.Set(index);
                    ca.Set(index);
                    verifyEqual("set1", a, ca);
                    index = rand() % sz;
                    b.Set(index);
                    cb.Set(index);
                    verifyEqual("set2", b, cb);

                    verifyEqual("complement1", a.Complement(), ca.Complement());
                    verifyEqual("complement2", a.Complement(), ca.Complement());
                    verifyEqual("complement3", b.Complement(), cb.Complement());
                    verifyEqual("complement4", b.Complement(), cb.Complement());

                    verifyEqual("bitor1", a | b, ca | cb);
                    verifyEqual("bitand1", a & b, ca & cb);
                    verifyEqual("bitxor1", a ^ b, ca ^ cb);
                    verifyEqual("bitsub1", a & TfBits(b).Complement(), ca - cb);

                    verifyEqual("bitor2", a |= b, ca |= cb);
                    verifyEqual("bitand2", a &= b, ca &= cb);
                    verifyEqual("bitxor2", a ^= b, ca ^= cb);
                    verifyEqual("bitsub2", a -= b, ca -= cb);

                    TF_VERIFY(a.Contains(b) == ca.Contains(cb));
                    TF_VERIFY(a.HasNonEmptyDifference(b) ==
                              ca.HasNonEmptyDifference(cb));
                    TF_VERIFY(a.HasNonEmptyIntersection(b) ==
                              ca.HasNonEmptyIntersection(cb));

                    TF_VERIFY(a.GetFirstSet() == ca.GetFirstSet(),
                              "a = %s, ca = %s",
                              a.GetAsStringLeftToRight().c_str(),
                              ca.GetAsStringLeftToRight().c_str());
                    TF_VERIFY(b.GetFirstSet() == cb.GetFirstSet(),
                              "b = %s, cb = %s",
                              b.GetAsStringLeftToRight().c_str(),
                              cb.GetAsStringLeftToRight().c_str());
                   
                    TF_VERIFY(a.GetLastSet() == ca.GetLastSet(),
                              "a = %s, ca = %s",
                              a.GetAsStringLeftToRight().c_str(),
                              ca.GetAsStringLeftToRight().c_str());
                    TF_VERIFY(b.GetLastSet() == cb.GetLastSet(),
                              "b = %s, cb = %s",
                              b.GetAsStringLeftToRight().c_str(),
                              cb.GetAsStringLeftToRight().c_str());

                    TF_VERIFY(a.GetNumSet() == ca.GetNumSet(),
                              "a = %s, ca = %s",
                              a.GetAsStringLeftToRight().c_str(),
                              ca.GetAsStringLeftToRight().c_str());
                    TF_VERIFY(b.GetNumSet() == cb.GetNumSet(),
                              "b = %s, cb = %s",
                              b.GetAsStringLeftToRight().c_str(),
                              cb.GetAsStringLeftToRight().c_str());

                    verifyEqual("equal1", a, ca);
                    verifyEqual("equal2", b, cb);
                }
            };

            // Do randomized testing for 2 seconds.
            TfStopwatch runTimer;
            while (runTimer.GetSeconds() < 2.0) {
                runTimer.Start();
                checkBits();
                runTimer.Stop();
            }
        }

    }

    // Contains and Overlaps
    {
        TfCompressedBits a(4);
        a.SetRange(1, 3);
        TF_AXIOM(a.GetAsStringLeftToRight() == "0111");

        // Contains
        TfCompressedBits b(4);
        b.Set(0);
        TF_AXIOM(b.GetAsStringLeftToRight() == "1000");
        TF_AXIOM(b.HasNonEmptyDifference(a));

        b.Set(1);
        TF_AXIOM(b.GetAsStringLeftToRight() == "1100");
        TF_AXIOM(b.HasNonEmptyDifference(a));

        b.Set(2);
        TF_AXIOM(b.GetAsStringLeftToRight() == "1110");
        TF_AXIOM(b.HasNonEmptyDifference(a));

        b.Clear(0);
        TF_AXIOM(b.GetAsStringLeftToRight() == "0110");
        TF_AXIOM(!b.HasNonEmptyDifference(a));

        b.Clear(1);
        TF_AXIOM(b.GetAsStringLeftToRight() == "0010");
        TF_AXIOM(!b.HasNonEmptyDifference(a));

        b.Clear(2);
        TF_AXIOM(b.GetAsStringLeftToRight() == "0000");
        TF_AXIOM(!b.HasNonEmptyDifference(a));

        a.Clear(3);
        b.Set(3);
        TF_AXIOM(a.GetAsStringLeftToRight() == "0110");
        TF_AXIOM(b.GetAsStringLeftToRight() == "0001");
        TF_AXIOM(b.HasNonEmptyDifference(a));

        // Overlaps
        TF_AXIOM(!b.HasNonEmptyIntersection(a));

        a.Set(3);
        TF_AXIOM(a.GetAsStringLeftToRight() == "0111");
        TF_AXIOM(b.HasNonEmptyIntersection(a));

        b.Clear(3);
        TF_AXIOM(b.GetAsStringLeftToRight() == "0000");
        TF_AXIOM(!b.HasNonEmptyIntersection(a));

        b.Set(2);
        TF_AXIOM(b.GetAsStringLeftToRight() == "0010");
        TF_AXIOM(b.HasNonEmptyIntersection(a));

        b.Set(1);
        TF_AXIOM(b.GetAsStringLeftToRight() == "0110");
        TF_AXIOM(b.HasNonEmptyIntersection(a));

        b.Set(3);
        TF_AXIOM(b.GetAsStringLeftToRight() == "0111");
        TF_AXIOM(b.HasNonEmptyIntersection(a));

        TfCompressedBits c(4);
        c.Set(0);
        TF_AXIOM(c.GetAsStringLeftToRight() == "1000");
        TF_AXIOM(!b.HasNonEmptyIntersection(c));
    }

    // Grow and shrink word counts. This should also grow/shrink across
    // the local storage optimization threshold.
    {
        TfCompressedBits a(10);
        TF_AXIOM(a.GetNumSet() == 0);
        TF_AXIOM(a.GetAsStringLeftToRight() == "0000000000");
        TF_AXIOM(a.GetNumPlatforms() == 1);
        TF_AXIOM(a.GetNumSetPlatforms() == 0);
        TF_AXIOM(a.GetNumUnsetPlatforms() == 1);

        // Set every other bit, to create a lot of words / platforms
        a.Set(0);
        TF_AXIOM(a.GetNumSet() == 1);
        TF_AXIOM(a.GetAsStringLeftToRight() == "1000000000");
        TF_AXIOM(a.GetNumPlatforms() == 2);
        TF_AXIOM(a.GetNumSetPlatforms() == 1);
        TF_AXIOM(a.GetNumUnsetPlatforms() == 1);

        a.Set(2);
        TF_AXIOM(a.GetNumSet() == 2);
        TF_AXIOM(a.GetAsStringLeftToRight() == "1010000000");
        TF_AXIOM(a.GetNumPlatforms() == 4);
        TF_AXIOM(a.GetNumSetPlatforms() == 2);
        TF_AXIOM(a.GetNumUnsetPlatforms() == 2);

        a.Set(4);
        TF_AXIOM(a.GetNumSet() == 3);
        TF_AXIOM(a.GetAsStringLeftToRight() == "1010100000");
        TF_AXIOM(a.GetNumPlatforms() == 6);
        TF_AXIOM(a.GetNumSetPlatforms() == 3);
        TF_AXIOM(a.GetNumUnsetPlatforms() == 3);

        a.Set(6);
        TF_AXIOM(a.GetNumSet() == 4);
        TF_AXIOM(a.GetAsStringLeftToRight() == "1010101000");
        TF_AXIOM(a.GetNumPlatforms() == 8);
        TF_AXIOM(a.GetNumSetPlatforms() == 4);
        TF_AXIOM(a.GetNumUnsetPlatforms() == 4);

        a.Set(8);
        TF_AXIOM(a.GetNumSet() == 5);
        TF_AXIOM(a.GetAsStringLeftToRight() == "1010101010");
        TF_AXIOM(a.GetNumPlatforms() == 10);
        TF_AXIOM(a.GetNumSetPlatforms() == 5);
        TF_AXIOM(a.GetNumUnsetPlatforms() == 5);

        // Set ever other bit to consolidate words / platforms
        a.Set(1);
        TF_AXIOM(a.GetNumSet() == 6);
        TF_AXIOM(a.GetAsStringLeftToRight() == "1110101010");
        TF_AXIOM(a.GetNumPlatforms() == 8);
        TF_AXIOM(a.GetNumSetPlatforms() == 4);
        TF_AXIOM(a.GetNumUnsetPlatforms() == 4);

        a.Set(3);
        TF_AXIOM(a.GetNumSet() == 7);
        TF_AXIOM(a.GetAsStringLeftToRight() == "1111101010");
        TF_AXIOM(a.GetNumPlatforms() == 6);
        TF_AXIOM(a.GetNumSetPlatforms() == 3);
        TF_AXIOM(a.GetNumUnsetPlatforms() == 3);

        a.Set(5);
        TF_AXIOM(a.GetNumSet() == 8);
        TF_AXIOM(a.GetAsStringLeftToRight() == "1111111010");
        TF_AXIOM(a.GetNumPlatforms() == 4);
        TF_AXIOM(a.GetNumSetPlatforms() == 2);
        TF_AXIOM(a.GetNumUnsetPlatforms() == 2);

        a.Set(7);
        TF_AXIOM(a.GetNumSet() == 9);
        TF_AXIOM(a.GetAsStringLeftToRight() == "1111111110");
        TF_AXIOM(a.GetNumPlatforms() == 2);
        TF_AXIOM(a.GetNumSetPlatforms() == 1);
        TF_AXIOM(a.GetNumUnsetPlatforms() == 1);

        a.Set(9);
        TF_AXIOM(a.GetNumSet() == 10);
        TF_AXIOM(a.GetAsStringLeftToRight() == "1111111111");
        TF_AXIOM(a.GetNumPlatforms() == 1);
        TF_AXIOM(a.GetNumSetPlatforms() == 1);
        TF_AXIOM(a.GetNumUnsetPlatforms() == 0);
    }

    // Iterators
    {
        TfCompressedBits c(8);
        c.Set(1);
        c.Set(2);
        c.Set(3);
        c.Set(6);
        c.Set(7);
        TF_AXIOM(c.GetNumSet() == 5);
        TF_AXIOM(c.GetAsStringLeftToRight() == "01110011");
        TF_AXIOM(c.GetFirstSet() == 1);
        TF_AXIOM(c.GetLastSet() == 7);
        TF_AXIOM(c.IsAnySet());
        TF_AXIOM(c.IsAnyUnset());
        TF_AXIOM(!c.AreAllSet());
        TF_AXIOM(!c.AreAllUnset());
        TF_AXIOM(!c.AreContiguouslySet());

        size_t numSet = c.GetNumSet();
        TF_AXIOM(numSet == 5);

        // Verify inidividual values
        TF_AXIOM(!c.IsSet(0));
        TF_AXIOM(c.IsSet(1));
        TF_AXIOM(c.IsSet(2));
        TF_AXIOM(c.IsSet(3));
        TF_AXIOM(!c.IsSet(4));
        TF_AXIOM(!c.IsSet(5));
        TF_AXIOM(c.IsSet(6));
        TF_AXIOM(c.IsSet(7));
    
        // All
        {
            size_t count = 0;
            size_t accumIndices = 0;
            size_t accumValues = 0;
            TfCompressedBits::AllView v = c.GetAllView();
            TF_FOR_ALL (it, v) {
                ++count;
                std::cout << " " << *it;
                accumIndices += *it;
                accumValues += it.base().IsSet();
            }
            TF_AXIOM(count == 8);
            TF_AXIOM(accumIndices == 28);
            TF_AXIOM(accumValues == 5);
            std::cout << "\n";
        }
    
        // All Set
        {
            size_t count = 0;
            size_t accumIndices = 0;
            size_t accumValues = 0;
            TfCompressedBits::AllSetView v = c.GetAllSetView();
            TF_FOR_ALL (it, v) {
                ++count;
                std::cout << " " << *it;
                accumIndices += *it;
                accumValues += it.base().IsSet();
            }
            TF_AXIOM(count == 5);
            TF_AXIOM(accumIndices == 19);
            TF_AXIOM(accumValues == 5);
            std::cout << "\n";
        }

        // All Unset
        {
            size_t count = 0;
            size_t accumIndices = 0;
            size_t accumValues = 0;
            TfCompressedBits::AllUnsetView v = c.GetAllUnsetView();
            TF_FOR_ALL (it, v) {
                ++count;
                std::cout << " " << *it;
                accumIndices += *it;
                accumValues += it.base().IsSet();
            }
            TF_AXIOM(count == 3);
            TF_AXIOM(accumIndices == 9);
            TF_AXIOM(accumValues == 0);
            std::cout << "\n";
        }

        // All Platforms
        {
            size_t count = 0;
            size_t accumIndices = 0;
            size_t accumValues = 0;
            size_t accumPlatformSize = 0;
            TfCompressedBits::PlatformsView v = c.GetPlatformsView();
            for (TfCompressedBits::PlatformsView::const_iterator
                     it=v.begin(), e=v.end(); it != e; ++it) {
                ++count;
                std::cout << " " << *it;
                accumIndices += *it;
                accumValues += it.IsSet();
                accumPlatformSize += it.GetPlatformSize();
            }
            TF_AXIOM(count == 4);
            TF_AXIOM(accumIndices == 11);
            TF_AXIOM(accumValues == 2);
            TF_AXIOM(accumPlatformSize == 8);
            std::cout << "\n";
        }

        // Empty mask
        {
            TfCompressedBits d(8);
            TfCompressedBits::AllSetView vd = d.GetAllSetView();
            TF_AXIOM(vd.begin().IsAtEnd());
            TF_AXIOM(vd.begin() == vd.end());

            TfCompressedBits e;
            TfCompressedBits::AllSetView ve = e.GetAllSetView();
            TF_AXIOM(ve.begin().IsAtEnd());
            TF_AXIOM(ve.begin() == ve.end());
        }

        // All ones mask
        {
            TfCompressedBits d(8);
            d.SetAll();
            TfCompressedBits::AllSetView vd = d.GetAllSetView();
            TF_AXIOM(!vd.begin().IsAtEnd());
            TF_AXIOM(vd.begin() != vd.end());
            size_t count = 0;
            TF_FOR_ALL (it, vd) {
                ++count;
            }
            TF_AXIOM(count == 8);

            TfCompressedBits e(1);
            e.SetAll();
            TfCompressedBits::AllSetView ve = e.GetAllSetView();
            TF_AXIOM(!ve.begin().IsAtEnd());
            TF_AXIOM(ve.begin() != ve.end());
            count = 0;
            TF_FOR_ALL (it, ve) {
                ++count;
            }
            TF_AXIOM(count == 1);
        }

        // Default-constructed iterators are (sadly) required to report
        // IsAtEnd().
        {
            TfCompressedBits::AllView::const_iterator i1;
            TfCompressedBits::AllSetView::const_iterator i2;
            TfCompressedBits::AllUnsetView::const_iterator i3;
            TF_AXIOM(i1.IsAtEnd());
            TF_AXIOM(i2.IsAtEnd());
            TF_AXIOM(i3.IsAtEnd());
        }

        // FindNextSet and Friends
        // XXX: Deprecated API - would like to remove
        {
            size_t accumIndices = 0;
            size_t i = c.GetFirstSet();
            while (i < c.GetSize()) {
                accumIndices += i;
                i = c.FindNextSet(i + 1);
            }
            TF_AXIOM(accumIndices == 19);

            accumIndices = 0;
            i = c.GetLastSet();
            while (i < c.GetSize()) {
                accumIndices += i;
                if (i < 1) {
                    break;
                }
                i = c.FindPrevSet(i - 1);
            }
            TF_AXIOM(accumIndices == 19);

            accumIndices = 0;
            i = 0;
            while (i < c.GetSize()) {
                accumIndices += i;
                i = c.FindNextUnset(i + 1);
            }
            TF_AXIOM(accumIndices == 9);
        }

        // Find n-th set
        {
            // 01110011
            TF_AXIOM(c.FindNthSet(0) == 1);
            TF_AXIOM(c.FindNthSet(1) == 2);
            TF_AXIOM(c.FindNthSet(2) == 3);
            TF_AXIOM(c.FindNthSet(3) == 6);
            TF_AXIOM(c.FindNthSet(4) == 7);
            TF_AXIOM(c.FindNthSet(5) == c.GetSize());
            TF_AXIOM(c.FindNthSet(6) == c.GetSize());
            TF_AXIOM(c.FindNthSet(100) == c.GetSize());

            // 10001100
            TfCompressedBits ic(c);
            ic.Complement();
            TF_AXIOM(ic.FindNthSet(0) == 0);
            TF_AXIOM(ic.FindNthSet(1) == 4);
            TF_AXIOM(ic.FindNthSet(2) == 5);
            TF_AXIOM(ic.FindNthSet(3) == ic.GetSize());
            TF_AXIOM(ic.FindNthSet(4) == ic.GetSize());
            TF_AXIOM(ic.FindNthSet(100) == ic.GetSize());

            // 1111
            TfCompressedBits ac(4);
            ac.SetAll();
            TF_AXIOM(ac.FindNthSet(0) == 0);
            TF_AXIOM(ac.FindNthSet(1) == 1);
            TF_AXIOM(ac.FindNthSet(2) == 2);
            TF_AXIOM(ac.FindNthSet(3) == 3);
            TF_AXIOM(ac.FindNthSet(4) == ac.GetSize());
            TF_AXIOM(ac.FindNthSet(100) == ac.GetSize());

            // 0000
            TfCompressedBits nc(4);
            TF_AXIOM(nc.FindNthSet(0) == nc.GetSize());
            TF_AXIOM(nc.FindNthSet(1) == nc.GetSize());
            TF_AXIOM(nc.FindNthSet(2) == nc.GetSize());
            TF_AXIOM(nc.FindNthSet(3) == nc.GetSize());
            TF_AXIOM(nc.FindNthSet(4) == nc.GetSize());
            TF_AXIOM(nc.FindNthSet(100) == nc.GetSize());
        }
    }

    // Compress / Decompress
    {
        TfBits c(10);
        c.Set(1);
        c.Set(2);
        c.Set(6);
        c.Set(7);
        c.Set(8);
        TF_AXIOM(c.GetAsStringLeftToRight() == "0110001110");

        TfCompressedBits cc = TfCompressedBits(c);
        TF_AXIOM(cc.GetAsStringLeftToRight() == "0110001110");

        size_t numSet = cc.GetNumSet();
        TF_AXIOM(numSet == 5);

        c.Complement();
        TF_AXIOM(c.GetAsStringLeftToRight() == "1001110001");

        cc = TfCompressedBits(c);
        TF_AXIOM(cc.GetAsStringLeftToRight() == "1001110001");

        TfBits d;
        cc.Decompress(&d);
        TF_AXIOM(d.GetAsStringLeftToRight() == "1001110001");

        // 1x1 and 1x0 masks
        TfBits e(1);
        c.ClearAll();
        TF_AXIOM(e.GetAsStringLeftToRight() == "0");

        cc = TfCompressedBits(e);
        TF_AXIOM(cc.GetSize() == 1);
        TF_AXIOM(cc.GetNumSet() == 0);
        TF_AXIOM(!cc.IsSet(0));
        TF_AXIOM(cc.GetAsStringLeftToRight() == "0");

        e.SetAll();
        TF_AXIOM(e.GetAsStringLeftToRight() == "1");

        cc = TfCompressedBits(e);
        TF_AXIOM(cc.GetSize() == 1);
        TF_AXIOM(cc.GetNumSet() == 1);
        TF_AXIOM(cc.IsSet(0));
        TF_AXIOM(cc.GetAsStringLeftToRight() == "1");
    }

    // Shift
    {
        size_t numSet;

        // Right
        TfCompressedBits c(8);
        c.Set(2);
        c.Set(3);
        c.Set(4);
        c.Set(6);
        TF_AXIOM(c.GetAsStringLeftToRight() == "00111010");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 4);

        c.ShiftRight(0);
        TF_AXIOM(c.GetAsStringLeftToRight() == "00111010");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 4);

        c.ShiftRight(1);
        TF_AXIOM(c.GetAsStringLeftToRight() == "00011101");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 4);

        c.ShiftRight(1);
        TF_AXIOM(c.GetAsStringLeftToRight() == "00001110");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 3);

        c.ShiftRight(2);
        TF_AXIOM(c.GetAsStringLeftToRight() == "00000011");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 2);

        c.ShiftRight(5);
        TF_AXIOM(c.GetAsStringLeftToRight() == "00000000");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 0);

        c.Set(0);
        c.Set(1);
        c.Set(2);
        c.Set(3);
        c.Set(6);
        c.Set(7);        
        TF_AXIOM(c.GetAsStringLeftToRight() == "11110011");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 6);

        c.ShiftRight(3);
        TF_AXIOM(c.GetAsStringLeftToRight() == "00011110");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 4);

        c.ShiftRight(3);
        TF_AXIOM(c.GetAsStringLeftToRight() == "00000011");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 2);

        c.ShiftRight(2);
        TF_AXIOM(c.GetAsStringLeftToRight() == "00000000");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 0);

        c.Complement();
        TF_AXIOM(c.GetAsStringLeftToRight() == "11111111");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 8);

        c.ShiftRight(4);
        TF_AXIOM(c.GetAsStringLeftToRight() == "00001111");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 4);

        c.ShiftRight(100);
        TF_AXIOM(c.GetAsStringLeftToRight() == "00000000");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 0);

        c.ShiftRight(100);
        TF_AXIOM(c.GetAsStringLeftToRight() == "00000000");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 0);

        // Left
        c.ClearAll();
        c.Set(2);
        c.Set(3);
        c.Set(4);
        c.Set(6);
        TF_AXIOM(c.GetAsStringLeftToRight() == "00111010");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 4);

        c.ShiftLeft(0);
        TF_AXIOM(c.GetAsStringLeftToRight() == "00111010");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 4);

        c.ShiftLeft(1);
        TF_AXIOM(c.GetAsStringLeftToRight() == "01110100");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 4);

        c.ShiftLeft(1);
        TF_AXIOM(c.GetAsStringLeftToRight() == "11101000");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 4);

        c.ShiftLeft(2);
        TF_AXIOM(c.GetAsStringLeftToRight() == "10100000");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 2);

        c.ShiftLeft(5);
        TF_AXIOM(c.GetAsStringLeftToRight() == "00000000");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 0);

        c.Set(0);
        c.Set(1);
        c.Set(2);
        c.Set(3);
        c.Set(6);
        c.Set(7);        
        TF_AXIOM(c.GetAsStringLeftToRight() == "11110011");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 6);

        c.ShiftLeft(3);
        TF_AXIOM(c.GetAsStringLeftToRight() == "10011000");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 3);

        c.ShiftLeft(3);
        TF_AXIOM(c.GetAsStringLeftToRight() == "11000000");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 2);

        c.ShiftLeft(2);
        TF_AXIOM(c.GetAsStringLeftToRight() == "00000000");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 0);

        c.Complement();
        TF_AXIOM(c.GetAsStringLeftToRight() == "11111111");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 8);

        c.ShiftLeft(4);
        TF_AXIOM(c.GetAsStringLeftToRight() == "11110000");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 4);

        c.ShiftLeft(100);
        TF_AXIOM(c.GetAsStringLeftToRight() == "00000000");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 0);

        c.ShiftLeft(100);
        TF_AXIOM(c.GetAsStringLeftToRight() == "00000000");
        numSet = c.GetNumSet();
        TF_AXIOM(numSet == 0);
    }

    // Resizing
    {
        TfCompressedBits b(6);
        b.Set(0);
        b.Set(1);
        b.Set(4);
        TF_AXIOM(b.GetAsStringLeftToRight() == "110010");

        b.ResizeKeepContents(6);
        TF_AXIOM(b.GetAsStringLeftToRight() == "110010");

        b.ResizeKeepContents(10);
        TF_AXIOM(b.GetAsStringLeftToRight() == "1100100000");

        b.ResizeKeepContents(6);
        TF_AXIOM(b.GetAsStringLeftToRight() == "110010");

        b.ResizeKeepContents(2);
        TF_AXIOM(b.GetAsStringLeftToRight() == "11");

        b.ResizeKeepContents(1);
        TF_AXIOM(b.GetAsStringLeftToRight() == "1");

        b.ResizeKeepContents(0);
        TF_AXIOM(b.GetSize() == 0);
        TF_AXIOM(b.GetNumSet() == 0);
    }

    // TfBits API compatibility
    {
        TfBits a(0);
        TfCompressedBits b(0);
        Test_TfCompressedBits_VerifyEqualityWithConversion(a, b);

        a.SetAll();
        b.SetAll();
        Test_TfCompressedBits_VerifyEqualityWithConversion(a, b);

        a = TfBits(1);
        b = TfCompressedBits(1);
        Test_TfCompressedBits_VerifyEqualityWithConversion(a, b);

        a.SetAll();
        b.SetAll();
        Test_TfCompressedBits_VerifyEqualityWithConversion(a, b);

        a = TfBits(4);
        b = TfCompressedBits(4);
        Test_TfCompressedBits_VerifyEqualityWithConversion(a, b);

        a.SetAll();
        b.SetAll();
        Test_TfCompressedBits_VerifyEqualityWithConversion(a, b);

        a.Clear(0);
        a.Clear(3);
        b.Clear(0);
        b.Clear(3);
        a.SetAll();
        b.SetAll();
        Test_TfCompressedBits_VerifyEqualityWithConversion(a, b);

        a.Complement();
        b.Complement();
        Test_TfCompressedBits_VerifyEqualityWithConversion(a, b);
    }

    // Testing against a bug where TfCompressedBits was left in an internally
    // inconsistent state: The platforms array would contain zeroes, when _num
    // was unequal zero.
    {
        TfCompressedBits a(4);
        a.SetRange(0, 3);

        TfCompressedBits b(4);
        b.SetAll();

        TF_AXIOM(a == b);

        a.ClearAll();
        a.SetRange(2, 3);
        b.Clear(0);
        b.Clear(1);
        TF_AXIOM(a == b);
    }

    // Test building a TfCompressedBits from a string representation.
    {
        TfCompressedBits c = TfCompressedBits::FromString("0x5-1x5-0x5");
        TF_AXIOM(c.GetAsStringLeftToRight() == "000001111100000");
        TF_AXIOM(c.GetAsRLEString() == "0x5-1x5-0x5");

        c = TfCompressedBits::FromString("  0x5 - 1x5 - 0 x 5  ");
        TF_AXIOM(c.GetAsStringLeftToRight() == "000001111100000");
        TF_AXIOM(c.GetAsRLEString() == "0x5-1x5-0x5");

        c = TfCompressedBits::FromString("000001111100000");
        TF_AXIOM(c.GetAsStringLeftToRight() == "000001111100000");
        TF_AXIOM(c.GetAsRLEString() == "0x5-1x5-0x5");

        c = TfCompressedBits::FromString("00000 11111 000 00");
        TF_AXIOM(c.GetAsStringLeftToRight() == "000001111100000");
        TF_AXIOM(c.GetAsRLEString() == "0x5-1x5-0x5");

        c = TfCompressedBits::FromString("0x15");
        TF_AXIOM(c.GetAsStringLeftToRight() == "000000000000000");
        TF_AXIOM(c.GetAsRLEString() == "0x15");

        c = TfCompressedBits::FromString("1x15");
        TF_AXIOM(c.GetAsStringLeftToRight() == "111111111111111");
        TF_AXIOM(c.GetAsRLEString() == "1x15");

        // Invalid string formulations
        c = TfCompressedBits::FromString("3x15");
        TF_AXIOM(c.GetSize() == 0);

        c = TfCompressedBits::FromString("1x0");
        TF_AXIOM(c.GetSize() == 0);

        c = TfCompressedBits::FromString("0x5x1");
        TF_AXIOM(c.GetSize() == 0);

        c = TfCompressedBits::FromString("0x5-1");
        TF_AXIOM(c.GetSize() == 0);

        c = TfCompressedBits::FromString("0-5x1");
        TF_AXIOM(c.GetSize() == 0);

        c = TfCompressedBits::FromString("foo bar");
        TF_AXIOM(c.GetSize() == 0);

        c = TfCompressedBits::FromString("1x15 foo");
        TF_AXIOM(c.GetSize() == 0);

        c = TfCompressedBits::FromString("000001111122222");
        TF_AXIOM(c.GetSize() == 0);
    }

    std::cout << "... success!\n\n";

    return true;
}

TF_ADD_REGTEST(TfCompressedBits);
