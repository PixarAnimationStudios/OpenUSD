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
#include "pxr/base/tf/cxxCast.h"
#include <stdio.h>

PXR_NAMESPACE_USING_DIRECTIVE

#define CHECK(cond)                                                      \
    if (!(cond)) { status = false;                                       \
        fprintf(stderr, "testTfCxxCast: %s failed (line %d, %s)\n",   \
                #cond, __LINE__, BUILD_COMPONENT_SRC_PREFIX __FILE__);                              \
    }

struct PolyBase0 {
    virtual ~PolyBase0() { }
    char data0[1024];
};

struct PolyBase1 {
    virtual ~PolyBase1() { }
    char data1[128];
};

struct PolyBase2 {
    virtual ~PolyBase2() { }
    char data1[12];
};

struct PolyDerived1 : public PolyBase0, public PolyBase1 { };
struct PolyDerived2 : public PolyDerived1, public PolyBase2 { };

struct NonPolyBase0 {
    char data[128];
};

struct NonPolyBase1 {
    char data[12];
};

struct NonPolyDerived : public NonPolyBase0, public NonPolyBase1 { };

int main()
{
    bool status = true;

    PolyDerived1* pd1 = new PolyDerived1;

    CHECK(dynamic_cast<void*>(pd1) == TfCastToMostDerivedType(pd1));


    PolyDerived1* pd2 = new PolyDerived2;

    CHECK(dynamic_cast<void*>(pd2) == TfCastToMostDerivedType(pd2));

    PolyBase0* pb0 = pd1;

    CHECK(dynamic_cast<void*>(pb0) == TfCastToMostDerivedType(pb0));

    PolyBase1* pb1 = pd1;

    CHECK(dynamic_cast<void*>(pb1) == TfCastToMostDerivedType(pb1));

    pb1 = pd2;

    CHECK(dynamic_cast<void*>(pb1) == TfCastToMostDerivedType(pb1));

    NonPolyDerived* npd = new NonPolyDerived;
    NonPolyBase0* npb0 = npd;
    NonPolyBase1* npb1 = npd;
    
    CHECK(static_cast<void*>(npb0) == TfCastToMostDerivedType(npb0));    
    CHECK(static_cast<void*>(npb1) == TfCastToMostDerivedType(npb1));    
    CHECK(static_cast<void*>(npd) == TfCastToMostDerivedType(npd));    

    return status ? 0 : -1;
}
