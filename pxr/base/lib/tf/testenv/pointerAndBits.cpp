//
// Copyright 2016 Pixar
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
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/pointerAndBits.h"

#include <cstring>

// Ensure that we can make a TfPointerAndBits with an incomplete type.
class Incomplete;
struct Container {
    TfPointerAndBits<Incomplete> member;
};

static bool
Test_TfPointerAndBits()
{
    {
        // short and bits (we expect at least one bit available).
        TfPointerAndBits<short> pbs;
        TF_AXIOM(pbs.GetMaxValue() > 0);
        TF_AXIOM(pbs.GetNumBitsValues() > 1);

        short data(1234);
        pbs = &data;
        TF_AXIOM(pbs.Get() == &data);
        pbs.SetBits(1);
        TF_AXIOM(pbs.BitsAs<int>() == 1);
        TF_AXIOM(pbs.BitsAs<bool>() == true);
        TF_AXIOM(pbs.Get() == &data);

        short data2(4321);

        // Copy and swap.
        TfPointerAndBits<short>(&data2).Swap(pbs);
        TF_AXIOM(pbs.Get() == &data2);
        TF_AXIOM(pbs.BitsAs<bool>() == false);

        TF_AXIOM(TfPointerAndBits<short>(&data, 1).BitsAs<bool>() == true);
        TF_AXIOM(TfPointerAndBits<short>(&data, 1).Get() == &data);
    }

    return true;
}

TF_ADD_REGTEST(TfPointerAndBits);

