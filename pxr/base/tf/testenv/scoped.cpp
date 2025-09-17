//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/scoped.h"
#include <iostream>

using namespace std;
PXR_NAMESPACE_USING_DIRECTIVE

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
        cout << "std::bind: unexpected state before scope" << endl;
        ++errors;
    }
    {
        x = true;
        TfScoped<> scope(std::bind(&BoundFunc, &x, false));
        if (!x) {
            cout << "std::bind: unexpected state in scope" << endl;
            ++errors;
        }
    }
    if (x) {
        cout << "std::bind: unexpected state after scope" << endl;
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
