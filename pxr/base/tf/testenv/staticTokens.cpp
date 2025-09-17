//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/arch/export.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/staticTokens.h"

#include <map>
#include <string>


using std::map;
using std::string;
PXR_NAMESPACE_USING_DIRECTIVE

#define TFTEST_TOKENS \
    (foo) \
    ((bar, "bar_value")) 

// Public tokens.
// Normally only in .h file:
TF_DECLARE_PUBLIC_TOKENS( TfTestPublicTokens, ARCH_EXPORT, TFTEST_TOKENS );
// Normally only in .cpp file:
TF_DEFINE_PUBLIC_TOKENS( TfTestPublicTokens, TFTEST_TOKENS );

// Private tokens.
TF_DEFINE_PRIVATE_TOKENS( TfTestPrivateTokens, TFTEST_TOKENS );

// Helper for testing public/private tokens the same way
#define TEST(holder)                                                \
    TF_AXIOM( holder->foo == TfToken("foo") );                        \
    TF_AXIOM( holder->bar == TfToken("bar_value") );                  \
    TF_AXIOM( holder->allTokens == expectedAllTokens );               \
    ;

static std::vector<TfToken> expectedAllTokens;

static bool
Test_TfStaticTokens()
{
    // Expected contents of allTokens
    expectedAllTokens.push_back( TfToken("foo") );
    expectedAllTokens.push_back( TfToken("bar_value") );

    TEST(TfTestPublicTokens);
    TEST(TfTestPrivateTokens);

    return true;
}

TF_ADD_REGTEST(TfStaticTokens);
