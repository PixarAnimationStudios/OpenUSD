//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/functionRef.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/regTest.h"

PXR_NAMESPACE_USING_DIRECTIVE

static bool
Test_TfFunctionRef()
{
    auto lambda1 = [](int arg) { return arg + 1; };
    auto lambda2 = [](int arg) { return arg + 2; };

    TfFunctionRef<int (int)> f1(lambda1);
    TfFunctionRef<int (int)> f2(lambda2);

    TF_AXIOM(lambda1(1) == f1(1));
    TF_AXIOM(lambda2(1) == f2(1));
    TF_AXIOM(lambda1(1) != f2(1));
    TF_AXIOM(lambda2(1) != f1(1));

    f1.swap(f2);

    TF_AXIOM(lambda1(1) == f2(1));
    TF_AXIOM(lambda2(1) == f1(1));
    TF_AXIOM(lambda1(1) != f1(1));
    TF_AXIOM(lambda2(1) != f2(1));

    swap(f1, f2);

    TF_AXIOM(lambda1(1) == f1(1));
    TF_AXIOM(lambda2(1) == f2(1));
    TF_AXIOM(lambda1(1) != f2(1));
    TF_AXIOM(lambda2(1) != f1(1));

    f2 = f1;

    TF_AXIOM(f2(1) == f1(1));

    auto lambda3 = [](int arg) { return arg + 3; };
    
    f2 = lambda3;

    TF_AXIOM(lambda3(1) == f2(1));

    TfFunctionRef<int (int)> f3(f2);
    
    TF_AXIOM(f3(1) == f2(1));

    // Copy constructed objects should refer to the original function rather
    // than forming a reference to a reference.
    {
        const auto ok = [] { };
        const auto error = [] {
            TF_FATAL_ERROR("Constructed new reference to callable instead of "
                           "copying");
        };
        TfFunctionRef<void()> ref(ok);
        TfFunctionRef<void()> refCopy(ref);
        ref = error;
        refCopy();
    }

    // Copy assigned objects should refer to the original function rather
    // than forming a reference to a reference.
    {
        const auto ok = [] { };
        const auto error1 = [] {
            TF_FATAL_ERROR("Failed to assign reference");
        };
        const auto error2 = [] {
            TF_FATAL_ERROR("Assigned new reference to callable instead of "
                           "copying");
        };
        TfFunctionRef<void()> ref(ok);
        TfFunctionRef<void()> refCopy(error1);
        refCopy = ref;
        ref = error2;
        refCopy();
    }

    return true;
}

TF_ADD_REGTEST(TfFunctionRef);
