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
#include "pxr/pxr.h"
#include "pxr/base/arch/export.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/staticTokens.h"
#include <assert.h>

#include <map>
#include <string>

#include <assert.h>

using std::map;
using std::string;
PXR_NAMESPACE_USING_DIRECTIVE

#define TFTEST_TOKENS \
    (foo) \
    ((bar, "bar_value")) \
    ((array, ((array_0) (array_1))))

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
    TF_AXIOM( holder->array[0] == TfToken("array_0") );               \
    TF_AXIOM( holder->array[1] == TfToken("array_1") );               \
    TF_AXIOM( holder->array_0 == TfToken("array_0") );                \
    TF_AXIOM( holder->array_1 == TfToken("array_1") );               \
    TF_AXIOM( holder->allTokens == expectedAllTokens );               \
    ;

static std::vector<TfToken> expectedAllTokens;

static bool
Test_TfStaticTokens()
{
    // Expected contents of allTokens
    expectedAllTokens.push_back( TfToken("foo") );
    expectedAllTokens.push_back( TfToken("bar_value") );
    expectedAllTokens.push_back( TfToken("array_0") );
    expectedAllTokens.push_back( TfToken("array_1") );

    TEST(TfTestPublicTokens);
    TEST(TfTestPrivateTokens);

    return true;
}

TF_ADD_REGTEST(TfStaticTokens);
