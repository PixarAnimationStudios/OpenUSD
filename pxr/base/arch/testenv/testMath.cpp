//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/math.h"
#include "pxr/base/arch/error.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define AXIOM(cond) \
    if (!(cond)) { ARCH_ERROR("failed: " #cond); }

int main()
{
    /*
     * Verify that the exponent and significand of float and double are
     * IEEE-754 compliant.
     */
    if (ArchFloatToBitPattern(5.6904566e-28f) != 0x12345678 ||
        ArchBitPatternToFloat(0x12345678) != 5.6904566e-28f) {
        ARCH_ERROR("float is not IEEE-754 compliant");
    }
    if (ArchDoubleToBitPattern(
            5.6263470058989390e-221) != 0x1234567811223344ULL ||
        ArchBitPatternToDouble(
            0x1234567811223344ULL) != 5.6263470058989390e-221) {
        ARCH_ERROR("double is not IEEE-754 compliant");
    }

    AXIOM(ArchSign(-123) == -1);
    AXIOM(ArchSign(123) == 1);
    AXIOM(ArchSign(0) == 0);

    AXIOM(ArchCountTrailingZeros(1) == 0);
    AXIOM(ArchCountTrailingZeros(2) == 1);
    AXIOM(ArchCountTrailingZeros(3) == 0);
    AXIOM(ArchCountTrailingZeros(4) == 2);
    AXIOM(ArchCountTrailingZeros(5) == 0);
    AXIOM(ArchCountTrailingZeros(6) == 1);
    AXIOM(ArchCountTrailingZeros(7) == 0);
    AXIOM(ArchCountTrailingZeros(8) == 3);

    AXIOM(ArchCountTrailingZeros(65535) == 0);
    AXIOM(ArchCountTrailingZeros(65536) == 16);

    AXIOM(ArchCountTrailingZeros(~((1ull << 32ull)-1ull)) == 32);
    AXIOM(ArchCountTrailingZeros(1ull << 63ull) == 63);
    
    return 0;
}
