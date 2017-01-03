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
#include "pxr/base/tf/scoped.h"
#include <iostream>
#include <boost/lambda/lambda.hpp>

using namespace std;

static bool x = false;

static void
Func()
{
    x = false;
}

static void
BoundFunc(bool* y, const bool& value)
{
    x = value;
}

static void
ResetFunc(bool* y)
{
    x = false;
}

class Resetter {
public:
    Resetter(bool* x) : _x(x) { }

    void Reset()
    {
        *_x = false;
    }

private:
    bool* _x;
};

static bool
Test_TfScoped()
{
    int errors = 0;

    if (x) {
        cout << "Function: unexpected state before scope" << endl;
        ++errors;
    }
    {
        x = true;
        TfScoped<> scope(&Func);
        if (!x) {
            cout << "Function: unexpected state in scope" << endl;
            ++errors;
        }
    }
    if (x) {
        cout << "Function: unexpected state after scope" << endl;
        ++errors;
    }

    if (x) {
        cout << "boost::bind: unexpected state before scope" << endl;
        ++errors;
    }
    {
        x = true;
        TfScoped<> scope(boost::bind(&BoundFunc, &x, false));
        if (!x) {
            cout << "boost::bind: unexpected state in scope" << endl;
            ++errors;
        }
    }
    if (x) {
        cout << "boost::bind: unexpected state after scope" << endl;
        ++errors;
    }

    if (x) {
        cout << "boost lambda: unexpected state before scope" << endl;
        ++errors;
    }
    {
        x = true;
        TfScoped<> scope(boost::lambda::var(x) = false);
        if (!x) {
            cout << "boost lambda: unexpected state in scope" << endl;
            ++errors;
        }
    }
    if (x) {
        cout << "boost lambda: unexpected state after scope" << endl;
        ++errors;
    }

    if (x) {
        cout << "Function with arg: unexpected state before scope" << endl;
        ++errors;
    }
    {
        x = true;
        TfScoped<void (*)(bool*)> scope(&ResetFunc, &x);
        if (!x) {
            cout << "Function with arg: unexpected state in scope" << endl;
            ++errors;
        }
    }
    if (x) {
        cout << "Function with arg: unexpected state after scope" << endl;
        ++errors;
    }

    if (x) {
        cout << "Method: unexpected state before scope" << endl;
        ++errors;
    }
    {
        Resetter r(&x);
        x = true;
        TfScoped<void (Resetter::*)()> scope(&r, &Resetter::Reset);
        if (!x) {
            cout << "Method: unexpected state in scope" << endl;
            ++errors;
        }
    }
    if (x) {
        cout << "Method: unexpected state after scope" << endl;
        ++errors;
    }

    return !errors;
}

static bool
Test_TfScopedVar()
{
    int errors = 0;

    bool x = false;
    {
        TfScopedVar<bool> scope(x, true);
        if (!x) {
            cout << "bool: unexpected state in scope" << endl;
            ++errors;
        }
    }
    if (x) {
        cout << "bool: unexpected state after scope" << endl;
        ++errors;
    }

    int y = 5;
    {
        TfScopedAutoVar scope(y, 8);
        if (y != 8) {
            cout << "int: unexpected state in scope" << endl;
            ++errors;
        }
    }
    if (y != 5) {
        cout << "int: unexpected state after scope" << endl;
        ++errors;
    }

    return !errors;
}

TF_ADD_REGTEST(TfScoped);
TF_ADD_REGTEST(TfScopedVar);
