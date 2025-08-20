//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/mask.h"

#include <iostream>
#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE

static bool
TestEmptyMask()
{
    VdfMask mask;
    if (mask.GetSize() != 0) {
        std::cerr << "Could not create an empty mask." << std::endl;
        return false;
    }
    return true;
}


static bool
TestSetAllAndIsAllOnes() 
{
    VdfMask mask(20);
    mask.SetAll();
    if (!mask.IsAllOnes()) {
        std::cerr << "SetAll() and IsAllOnes() are inconsistent." 
                  << std::endl;
        return false;
    }
    return true;
}

static bool
TestIsAllZerosAndIterator() 
{
    VdfMask mask(10);

    if (!mask.IsAllZeros()) {
        std::cerr << "IsAllZeros is reporting false when true was expected." 
                  << std::endl;
        return false;
    }

    mask.SetIndex(4);

    if (mask.IsAllZeros()) {
        std::cerr << "IsAllZeros is reporting true when false was expected." 
                  << std::endl;
        return false;
    }


    VdfMask::iterator iter = mask.begin();
    if (*iter != 4) {
        std::cerr << "The iterator is not starting at the first set bit."
                  << std::endl;
        return false;
    }

    return true;
}

static bool
TestOverlaps() 
{
    VdfMask noneSet(100);
    VdfMask allSet = VdfMask::AllOnes(100);
    VdfMask someSet(100);
    someSet.SetIndex(10);
    someSet.SetIndex(20);

    if (noneSet.Overlaps(allSet)) {
        std::cerr << "Mask overlap reported but not expected." << std::endl;
        return false;
    }

    if (noneSet.Overlaps(someSet)) {
        std::cerr << "Mask overlap reported but not expected." << std::endl;
        return false;
    }

    if (!someSet.Overlaps(allSet)) {
        std::cerr << "Mask overlap not reported but was expected." 
                  << std::endl;
        return false;
    }
    
    VdfMask empty;
    if (empty.Overlaps(empty)) {
        std::cerr << "Mask overlap reported but not expected." << std::endl;
        return false;
    }

    VdfMask largeNonSet(1000);
    TF_AXIOM(!largeNonSet.IsAnySet());
    if (largeNonSet.Overlaps(largeNonSet)) {
        std::cerr << "Mask overlap reported but not expected." << std::endl;
        return false;
    }

    return true;
}


static bool
TestBooleanOperations() 
{
    VdfMask mask1(5); // 01010
    VdfMask mask2(5); // 10101
    VdfMask result(0);

    mask1.SetIndex(1); mask1.SetIndex(3);
    mask2.SetIndex(0); mask2.SetIndex(2); mask2.SetIndex(4);

    // AND
    result = mask1 & mask2;
    if (!result.IsAllZeros()) {
        std::cerr << "Execpted AND operation to produce all zeros." 
                  << std::endl;
        return false;
    }

    // OR
    result = mask1 | mask2;
    if (!result.IsAllOnes()) {
        std::cerr << "Execpted OR operation to produce all ones." << std::endl;
        return false;
    }

    // SET DIFFERENCE
    result = mask1 - mask2;
    if (result != mask1) {
        std::cerr << "Expected set difference to have no effect on mask1." 
                  << std::endl;
        return false;
    }
    result = mask2 - mask1;
    if (result != mask2) {
        std::cerr << "Expected set difference to have no effect on mask2." 
                  << std::endl;
        return false;
    }

    // For XOR we need to add one more bit to mask1 so that it actually
    // does something.
    mask1.SetIndex(2);
    VdfMask expected(5); // 11011
    expected.SetIndex(0); expected.SetIndex(1);
    expected.SetIndex(3); expected.SetIndex(4);
    result = mask1 ^ mask2;
    if (result != expected) {
        std::cerr << "Unexpected result from XOR" << std::endl;
        return false;
    }
    return true;
}

static bool
TestEqualityComparison()
{
    VdfMask allOnes = VdfMask::AllOnes(4);

    // 1100
    VdfMask maskA(4);
    maskA.SetIndex(0);
    maskA.SetIndex(1);

    // 0011
    VdfMask maskB(4);
    maskB.SetIndex(2);
    maskB.SetIndex(3);

    VdfMask mask(4);
    mask |= maskA;
    mask |= maskB;
    
    if (allOnes != mask) {
        std::cerr << "Unexpected result for equality comparison." << std::endl;
        return false;
    }

    return true;
}

static bool
TestPrintRLE()
{

    // Tests the PrinRLE() method.
    {
        std::ostringstream out;
        VdfMask mask(5);
        mask.SetIndex(1);
        mask.SetIndex(2);
        mask.SetIndex(4);
        out << mask.GetRLEString();
        std::string expected = "0x1-1x2-0x1-1x1";
        if (out.str() != expected) {
            std::cerr << "PrintRLE: expected: " << expected 
                      << " got: " << out.str() << std::endl;
            return false;
        }
    }

    // Test the degerate case of printing an empty mask.
    {
        std::ostringstream out;
        VdfMask emptyMask;
        out << emptyMask.GetRLEString();
        if (out.str() != "") {
            std::cerr << "PrintRLE: empty mask, got " << out.str() << std::endl;
            return false;
        }
    }


    return true;

}

static bool
TestGetNumSet()
{

    // Tests the GetNumSet() method.
    {
        VdfMask mask(5);
        mask.SetIndex(1);
        mask.SetIndex(2);
        mask.SetIndex(4);
        if (mask.GetNumSet() != 3) {
            std::cerr << "GetNumSet: expected 3 got " 
                      << mask.GetNumSet() << std::endl;
            return false;
        }
    }

    {
        VdfMask mask;
        if (mask.GetNumSet() != 0) {
            std::cerr << "GetNumSet: expected 0 got " 
                      << mask.GetNumSet() << std::endl;
            return false;
        }
    }

    {
        VdfMask mask = VdfMask::AllOnes(10);
        if (mask.GetNumSet() != 10) {
            std::cerr << "GetNumSet: expected 10 got " 
                      << mask.GetNumSet() << std::endl;
            return false;
        }
    }

    return true;

}

static bool
TestGetMemoryUsage()
{
    // Tests the GetMemoryUsage() method.
    {
        VdfMask mask(5);
        size_t mem = mask.GetMemoryUsage();
        if (mem != 48) {
            std::cerr << "GetMemoryUsage: expected 48 got "
                      << mem << std::endl;
            return false;
        }
    }

    return true;

}

// The list of tests to run.
typedef bool(* TestFunction)(void);
static const TestFunction tests[] = {

    TestEmptyMask,
    TestSetAllAndIsAllOnes,
    TestIsAllZerosAndIterator,
    TestOverlaps,
    TestBooleanOperations,
    TestEqualityComparison,
    TestPrintRLE,
    TestGetNumSet,
    TestGetMemoryUsage,
    NULL

};


int 
main(int argc, char **argv) 
{

    // This test tests very basic functionality of VdfMask.

    // Run through all the registered tests, and if any of them fail
    // fail the whole test.
    for (int i = 0; tests[i] != NULL; ++i) {
        if (!tests[i]()) {
            return -1;
        }
    }

    return 0;

}
