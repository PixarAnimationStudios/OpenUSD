//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/error.h"

PXR_NAMESPACE_OPEN_SCOPE

class DummyClassInNamespace {};
class OtherDummyClassInNamespace {
    public: class SubClass { };
};

template <class T>
class TemplatedDummyClassInNamespace { };

PXR_NAMESPACE_CLOSE_SCOPE


PXR_NAMESPACE_USING_DIRECTIVE

template <class T>
class TemplatedDummyClass { };

struct Mangled {};

struct FooSsSsSsBar {};

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

    printf("ArchDemangle('%s') => '%s', expected '%s'\n",
        mangledName.c_str(), toBeDemangledName.c_str(), typeName.c_str());

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
    // We have special case code for std::string.
    TestDemangle<std::string>("string");
    TestDemangle<TemplatedDummyClass<std::string>>(
        "TemplatedDummyClass<string>");
    // This one is a regression test for a demangle bug on Linux.
    TestDemangle<FooSsSsSsBar>("FooSsSsSsBar");

    TestDemangle<DummyClassInNamespace>("DummyClassInNamespace"); 
    TestDemangle<OtherDummyClassInNamespace::SubClass>("OtherDummyClassInNamespace::SubClass");
    TestDemangle<TemplatedDummyClassInNamespace<DummyClassInNamespace>>(
        "TemplatedDummyClassInNamespace<DummyClassInNamespace>");
    TestDemangle<TemplatedDummyClassInNamespace<OtherDummyClassInNamespace::SubClass>>(
        "TemplatedDummyClassInNamespace<OtherDummyClassInNamespace::SubClass>");

    TestDemangle<unsigned long>("unsigned long");
    TestDemangle<MangledAlso<int> >("MangledAlso<int>");

    // Since C++11, the parser specification has been improved to be able
    // to interpret multiple right angle brackets in nested template
    // declarations. The implementation of the C++ ABI has been updated
    // accordingly starting with Clang 14 on macOS 13.3
#if defined(MAC_OS_VERSION_13_3)
    const bool improvedAngleBracketDemangling = true;
#else
    const bool improvedAngleBracketDemangling = false;
#endif
    const char* const nestedTemplateTypeName =
        improvedAngleBracketDemangling
            ? "MangledAlso<MangledAlso<int>>"
            : "MangledAlso<MangledAlso<int> >";
    TestDemangle<MangledAlso<MangledAlso<int> > >(nestedTemplateTypeName);

    const char* const badType = "type_that_doesnt_exist";
#if defined(ARCH_OS_WINDOWS)
    ARCH_AXIOM(ArchGetDemangled(badType) == badType);
#else
    ARCH_AXIOM(ArchGetDemangled(badType) == "");
#endif
   
    return 0;
}
