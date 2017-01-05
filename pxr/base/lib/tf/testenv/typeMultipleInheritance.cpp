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
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/arch/error.h"

using namespace std;

class A {
public:
    virtual ~A() {}
};

class B {
public:
    virtual ~B() {}
};

class X : public A, public B {
public:
    virtual ~X() {}
};

// Note that A,B are inherited in the opposite order from X.
class Y : public B, public A {
public:
    virtual ~Y() {}
};

class Z : public X, public Y {
public:
    virtual ~Z() {}
};


TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<A>();
    TfType::Define<B>();
    TfType::Define<X, TfType::Bases< A, B > >();
    TfType::Define<Y, TfType::Bases< B, A > >();
    TfType::Define<Z, TfType::Bases< X, Y > >();
}

static bool
Test_TfType_MultipleInheritance()
{
    // Test TfType::GetAncestorTypes()'s error condition of
    // inconsistent multiple inheritance.  (We'd ideally test this from
    // Python, but Python prohibits you from even declaring hierarchies
    // like this to begin with.)

    TfErrorMark m;
    m.SetMark();

    vector<TfType> types;

    TF_AXIOM(m.IsClean());

    TfType::Find<Z>().GetAllAncestorTypes(&types);

    TF_AXIOM(!m.IsClean());
    m.Clear();

    return true;
}

TF_ADD_REGTEST(TfType_MultipleInheritance);

