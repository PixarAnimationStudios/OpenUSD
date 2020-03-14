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

    return 0;
}
