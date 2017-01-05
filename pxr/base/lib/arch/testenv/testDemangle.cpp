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
#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/error.h"

struct Mangled {};

template <class T>
class MangledAlso {};

typedef Mangled Remangled;

enum MangleEnum { ONE, TWO, THREE };

template <typename T>
static bool
TestDemangle(const std::string& typeName)
{
    const std::type_info& typeInfo = typeid(T);
    std::string mangledName = typeInfo.name();
    std::string toBeDemangledName = typeInfo.name();

    ARCH_AXIOM(ArchDemangle(&toBeDemangledName));
    ARCH_AXIOM(toBeDemangledName == typeName);
    ARCH_AXIOM(ArchGetDemangled(mangledName) == typeName);
    ARCH_AXIOM(ArchGetDemangled(typeInfo) == typeName);
    ARCH_AXIOM(ArchGetDemangled<T>() == typeName);

    return true;
}

int main()
{
    TestDemangle<bool>("bool");
    TestDemangle<Mangled>("Mangled");
    TestDemangle<Remangled>("Mangled");
    TestDemangle<MangleEnum>("MangleEnum");

#if !defined(__ICC)
    // not currently tested on icc and sun 64 bit
    TestDemangle<unsigned long>("unsigned long");
    TestDemangle<MangledAlso<int> >("MangledAlso<int>");
    TestDemangle<MangledAlso<MangledAlso<int> > >("MangledAlso<MangledAlso<int> >");
#endif

    ARCH_AXIOM(ArchGetDemangled("type_that_doesnt_exist") == "");
        
    return 0;
}
