//
// Copyright 2020 Pixar
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
#include "pxr/base/tf/functionRef.h"
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

    return true;
}

TF_ADD_REGTEST(TfFunctionRef);
