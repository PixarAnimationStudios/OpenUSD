//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file wrapTestPyStaticTokens.cpp

#include "pxr/pxr.h"

#include "pxr/base/tf/pyStaticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define TF_TEST_TOKENS                  \
    (orange)                            \
    ((pear, "d'Anjou"))                 

TF_DECLARE_PUBLIC_TOKENS(tfTestStaticTokens, TF_API, TF_TEST_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(tfTestStaticTokens, TF_TEST_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
struct _DummyScope {
};
}

void
wrapTf_TestPyStaticTokens(nanobind::module_& m)
{
    TF_NB_WRAP_PUBLIC_TOKENS(m, "_testStaticTokens",
                             tfTestStaticTokens, TF_TEST_TOKENS);

    nanobind::class_<_DummyScope> cls(m, "_TestStaticTokens");
    TF_NB_WRAP_PUBLIC_TOKENS_IN_SCOPE(
        cls, tfTestStaticTokens, TF_TEST_TOKENS);
}
