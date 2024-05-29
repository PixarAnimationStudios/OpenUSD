//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/diagnostic.h"

#include <map>
#include <string>

using std::map;
using std::string;
PXR_NAMESPACE_USING_DIRECTIVE

TfStaticData<string> _str1;
TfStaticData<string> _str2;
TfStaticData<string> _str3;
TfStaticData<string> _str4;

TF_MAKE_STATIC_DATA(string, _initStr) {
    *_initStr = "initialized";
}

#if !defined(ARCH_OS_WINDOWS) // Problems with macro to eat parens.
TF_MAKE_STATIC_DATA((map<int, int>), _initMap) {
    (*_initMap)[1] = 11;
    (*_initMap)[2] = 22;
}
#endif

TF_MAKE_STATIC_DATA(const std::vector<int>, _constVector)
{
    *_constVector = { 1, 2, 3 };
}

class Count {
public:
    Count() { ++count; }
    ~Count() { --count; }

public:
    static size_t count;
};
size_t Count::count = 0;

struct Type1 {
    Type1() : line(0) { }
    string str;
    size_t line;
    string func;
};
struct Type2 {
    Type2() : line(0) { }
    size_t line;
    string func;
    string str;
};

static bool
Test_TfStaticData()
{
    // none should exist yet.
    TF_AXIOM(!_str1.IsInitialized() && 
             !_str2.IsInitialized() && 
             !_str3.IsInitialized());
    TF_AXIOM(!(_str1.IsInitialized() || 
               _str2.IsInitialized() || 
               _str3.IsInitialized()));
    TF_AXIOM(Count::count == 0);

    (void) *_str1;  // force creation
    TF_AXIOM(_str1.IsInitialized() &&
             !_str2.IsInitialized() &&
             !_str3.IsInitialized());
    TF_AXIOM(_str1->empty());

    TF_AXIOM(_str2->empty() &&
             _str1->empty() &&
             !_str3.IsInitialized());

    // arrow dereference, should default construct.
    TF_AXIOM(_str3->empty() &&
             _str2.IsInitialized() &&
             _str1.IsInitialized());

    // Dereference, should default construct.  NOTE: please don't replace
    // this with '->'!  It's testing the '*' operator explicitly.
    TF_AXIOM((*_str4).empty());

    // Test a static data obj with an initializer.
    TF_AXIOM(!_initStr.IsInitialized());
    TF_AXIOM(*_initStr == "initialized");

#if !defined(ARCH_OS_WINDOWS)
    // test a static data obj for a templated type with an initializer.
    TF_AXIOM(!_initMap.IsInitialized());
    TF_AXIOM(_initMap->size() == 2);
    TF_AXIOM((*_initMap)[1] == 11);
    TF_AXIOM((*_initMap)[2] == 22);
#endif

    // test accessing const static data
    TF_AXIOM(!_constVector.IsInitialized());
    TF_AXIOM(_constVector->size() == 3);
    TF_AXIOM((*_constVector)[0] == 1);
    TF_AXIOM((*_constVector)[1] == 2);
    TF_AXIOM((*_constVector)[2] == 3);

    return true;
}

TF_ADD_REGTEST(TfStaticData);
