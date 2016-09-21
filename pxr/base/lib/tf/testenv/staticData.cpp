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
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/diagnostic.h"

#include <map>
#include <string>

#if !defined(ARCH_OS_WINDOWS)

using std::map;
using std::string;

TfStaticData<string> _str1;
TfStaticData<string> _str2;
TfStaticData<string> _str3;
TfStaticData<string> _str4;

TF_MAKE_STATIC_DATA(string, _initStr) {
    *_initStr = "initialized";
}

TF_MAKE_STATIC_DATA(map<int, int>, _initMap) {
    (*_initMap)[1] = 11;
    (*_initMap)[2] = 22;
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
    TF_AXIOM(not _str1.IsInitialized() and
             not _str2.IsInitialized() and
             not _str3.IsInitialized());
    TF_AXIOM(not (_str1.IsInitialized() or
                  _str2.IsInitialized() or
                  _str3.IsInitialized()));
    TF_AXIOM(Count::count == 0);

    (void) *_str1;  // force creation
    TF_AXIOM(_str1.IsInitialized() and
             not _str2.IsInitialized() and
             not _str3.IsInitialized());
    TF_AXIOM(_str1->empty());

    TF_AXIOM(_str2->empty() and
             _str1->empty() and
             not _str3.IsInitialized());

    // arrow dereference, should default construct.
    TF_AXIOM(_str3->empty() and
             _str2.IsInitialized() and
             _str1.IsInitialized());

    // Dereference, should default construct.  NOTE: please don't replace
    // this with '->'!  It's testing the '*' operator explicitly.
    TF_AXIOM((*_str4).empty());

    // Test a static data obj with an initializer.
    TF_AXIOM(not _initStr.IsInitialized());
    TF_AXIOM(*_initStr == "initialized");

    // test a static data obj for a templated type with an initializer.
    TF_AXIOM(not _initMap.IsInitialized());
    TF_AXIOM(_initMap->size() == 2);
    TF_AXIOM((*_initMap)[1] == 11);
    TF_AXIOM((*_initMap)[2] == 22);

    return true;
}

TF_ADD_REGTEST(TfStaticData);

#endif // #if !defined(ARCH_OS_WINDOWS)