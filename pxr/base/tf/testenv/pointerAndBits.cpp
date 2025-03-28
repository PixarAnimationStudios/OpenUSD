//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/pointerAndBits.h"

#include <cstring>

PXR_NAMESPACE_USING_DIRECTIVE

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

        // GetLiteral.
        TF_AXIOM(TfPointerAndBits<short>(&data, 1).GetLiteral() !=
                 pbs.GetLiteral());
        TF_AXIOM(TfPointerAndBits<short>(&data, 0).GetLiteral() !=
                 pbs.GetLiteral());
        TF_AXIOM(TfPointerAndBits<short>(&data2, 1).GetLiteral() !=
                 pbs.GetLiteral());
        TF_AXIOM(TfPointerAndBits<short>(&data2, 0).GetLiteral() ==
                 pbs.GetLiteral());
    }

    return true;
}

TF_ADD_REGTEST(TfPointerAndBits);

