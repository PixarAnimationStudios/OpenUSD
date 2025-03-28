//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_ARCH_ATTRIBUTES_H
#define PXR_BASE_ARCH_ATTRIBUTES_H

/// \file arch/attributes.h
/// Define function attributes.
///
/// This file allows you to define architecture-specific or compiler-specific
/// options to be used outside lib/arch.

#include "pxr/pxr.h"
#include "pxr/base/arch/export.h"

PXR_NAMESPACE_OPEN_SCOPE

#if defined(doxygen)

/// Macro used to indicate a function takes a printf-like specification.
///
/// This attribute is used as follows:
/// \code
///    void PrintFunc(T1 arg1, T2 arg2, const char* fmt, ...)
///        ARCH_PRINTF_FUNCTION(3, 4)
/// \endcode
/// This indicates that the third argument is the format string, and that the
/// fourth argument is where the var-args corresponding to the format string begin.
///
/// \hideinitializer
#   define ARCH_PRINTF_FUNCTION(_fmt, _firstArg)

/// Macro used to indicate a function takes a scanf-like specification.
///
/// This attribute is used as follows:
/// \code
///    void ScanFunc(T1 arg1, T2 arg2, const char* fmt, ...)
///        ARCH_PRINTF_FUNCTION(3, 4)
/// \endcode
/// This indicates that the third argument is the format string, and
/// that the fourth argument is where the var-args corresponding to the
/// format string begin.
///
/// \hideinitializer
#   define ARCH_SCANF_FUNCTION(_fmt, _firstArg)

/// Macro used to indicate that a function should never be inlined.
///
/// This attribute is used as follows:
/// \code
///    void Func(T1 arg1, T2 arg2) ARCH_NOINLINE;
/// \endcode
///
/// \hideinitializer
#   define ARCH_NOINLINE

/// Macro used to indicate a function parameter may be unused.
///
/// In general, avoid this attribute if possible.  Mostly this attribute
/// should be used when the set of arguments to a function is described
/// as part of a macro.  The usage is:
/// \code
///    void Func(T1 arg1, ARCH_UNUSED_ARG T2 arg2, ARCH_UNUSED_ARG T3 arg3, T4 arg4) {
///        ...
///    }
/// \endcode
///
/// \hideinitializer
#   define ARCH_UNUSED_ARG

/// Macro used to indicate a function may be unused.
///
/// In general, avoid this attribute if possible.  Mostly this attribute
/// should be used when you need to keep a function around (for some
/// good reason), but it is not used in the rest of the code. The usage
/// is:
/// \code
///    ARCH_UNUSED_FUNCTION void Func() {
///        ...
///    }
/// \endcode
///
/// \hideinitializer
#   define ARCH_UNUSED_FUNCTION

/// Macro used to indicate that a function's code must always be emitted even
/// if not required.
///
/// This attribute is especially useful with templated registration functions,
/// which might not be present in the linked binary if they are not used (or
/// the compiler optimizes away their use.)
///
/// The usage is:
/// \code
/// template <typename T>
/// struct TraitsClass {
///    static void RegistryFunction() ARCH_USED_FUNCTION {
///        ...
///    }
/// };
/// \endcode
///
/// \hideinitializer
#   define ARCH_USED_FUNCTION

/// Macro to begin the definition of a function that should be executed by
/// the dynamic loader when the dynamic object (library or program) is
/// loaded.
///
/// \p _priority is used to order the execution of constructors.  Valid
/// values are integers in the range [0,255].  Constructors with lower
/// numbers are run first.  It is unspecified if these functions are run
/// before or after dynamic initialization of non-local variables.
///
/// \p _name is the name of the function and must be unique across all
/// invocations of ARCH_CONSTRUCTOR in the same translation unit.
/// The remaining arguments should be types for the signature of the
/// function.  The types are only to make the name unique (when mangled);
/// the function will be called with no arguments so the arguments must
/// not be used.  If you don't need any arguments you must use void.
///
/// \hideinitializer
#   define ARCH_CONSTRUCTOR(_name, _priority, ...)

/// Macro to begin the definition of a function that should be executed by
/// the dynamic loader when the dynamic object (library or program) is
/// unloaded.
///
/// \p _priority is used to order the execution of destructors.  Valid
/// values are integers in the range [0,255].  Destructors with higher
/// numbers are run first.  It is unspecified if these functions are run
/// before or after dynamically initialized non-local variables.
///
/// \p _name is the name of the function and must be unique across all
/// invocations of ARCH_CONSTRUCTOR in the same translation unit.
/// The remaining arguments should be types for the signature of the
/// function.  The types are only to make the name unique (when mangled);
/// the function will be called with no arguments so the arguments must
/// not be used.  If you don't need any arguments you must use void.
///
/// \hideinitializer
#   define ARCH_DESTRUCTOR(_name, _priority, ...)

/// Macro to begin the definition of a class that is using private inheritance
/// to take advantage of the empty base optimization. Some compilers require
/// an explicit tag.
///
/// In C++20, usage of private inheritance may be able to be retired with the
/// [[no_unique_address]] tag.
#   define ARCH_EMPTY_BASES

#elif defined(ARCH_COMPILER_GCC) || defined(ARCH_COMPILER_CLANG)

#   define ARCH_PRINTF_FUNCTION(_fmt, _firstArg) \
        __attribute__((format(printf, _fmt, _firstArg)))
#   define ARCH_SCANF_FUNCTION(_fmt, _firstArg)	\
        __attribute__((format(scanf, _fmt, _firstArg)))
#   define ARCH_NOINLINE __attribute__((noinline))
#   define ARCH_ALWAYS_INLINE __attribute__((always_inline))
#   define ARCH_UNUSED_ARG   __attribute__ ((unused))
#   define ARCH_UNUSED_FUNCTION __attribute__((unused))
#   define ARCH_USED_FUNCTION __attribute__((used))
#   define ARCH_EMPTY_BASES

#elif defined(ARCH_COMPILER_MSVC)

#   define ARCH_PRINTF_FUNCTION(_fmt, _firstArg)
#   define ARCH_SCANF_FUNCTION(_fmt, _firstArg)
#   define ARCH_NOINLINE // __declspec(noinline)
#   define ARCH_ALWAYS_INLINE
#   define ARCH_UNUSED_ARG
#   define ARCH_UNUSED_FUNCTION
#   define ARCH_USED_FUNCTION
#   define ARCH_EMPTY_BASES __declspec(empty_bases)

#else

// Leave macros undefined so we'll fail to build on a new system/compiler
// rather than fail mysteriously at runtime.

#endif

// Helper to do on-demand static initialziation.  We need to insert per-library
// static initializers if the ARCH_CONSTRUCTOR macros are used, etc, but we
// don't want that to happen otherwise.  This mechanism makes that possible.  It
// works by creating a class template (Arch_PerLibInit) that has hidden
// visibility and a static member of its template parameter (StaticInit).  In
// its constructor, it "uses" its static member 'init', in order to ensure it
// gets instantiated.  Since it's a static member, it gets initialized only if
// it's instantiated.  This lets us have macros like ARCH_CONSTRUCTOR() that
// require a static initializer, but to only emit that static initializer in
// translation units that actually invoke the macro.  Clients typically do this
// by way of the _ARCH_ENSURE_PER_LIB_INIT macro.  This is tested on all current
// supported compilers (clang, gcc, msvc).  The hidden visibility is required to
// ensure that each library gets its own initialization.  Without it, on Linux,
// there would be exactly *one* initialization no matter how many libraries are
// loaded.
template <class StaticInit>
struct ARCH_HIDDEN Arch_PerLibInit {
    Arch_PerLibInit() { /* "use" of init here forces instantiation */
        (void)init; }
private:
    static StaticInit init;
};
template <class StaticInit>
StaticInit Arch_PerLibInit<StaticInit>::init;

#define _ARCH_CAT_NOEXPAND(a, b) a ## b
#define _ARCH_CAT(a, b) _ARCH_CAT_NOEXPAND(a, b)
#define _ARCH_ENSURE_PER_LIB_INIT(T, prefix) \
    static Arch_PerLibInit<T> _ARCH_CAT(prefix, __COUNTER__)

#if defined(doxygen)

// The macros are already defined above in doxygen.

#elif defined(ARCH_OS_DARWIN)

// Entry for a constructor/destructor in the custom section.
struct Arch_ConstructorEntry {
    typedef void (*Type)(void);
    Type function;
    unsigned int version:24;    // USD version
    unsigned int priority:8;    // Priority of function
};

// Emit a Arch_ConstructorEntry in the __Data,pxrctor section.
#   define ARCH_CONSTRUCTOR(_name, _priority, ...)                             \
    static void _name(__VA_ARGS__);                                            \
    static const Arch_ConstructorEntry _ARCH_CAT_NOEXPAND(arch_ctor_, _name)   \
        __attribute__((used, section("__DATA,pxrctor"))) = {                   \
        reinterpret_cast<Arch_ConstructorEntry::Type>(&_name),                 \
        static_cast<unsigned>(PXR_VERSION),                                    \
        _priority                                                              \
    };                                                                         \
    static void _name(__VA_ARGS__)
    
// Emit a Arch_ConstructorEntry in the __Data,pxrdtor section.
#   define ARCH_DESTRUCTOR(_name, _priority, ...)                              \
    static void _name(__VA_ARGS__);                                            \
    static const Arch_ConstructorEntry _ARCH_CAT_NOEXPAND(arch_dtor_, _name)   \
        __attribute__((used, section("__DATA,pxrdtor"))) = {                   \
        reinterpret_cast<Arch_ConstructorEntry::Type>(&_name),                 \
        static_cast<unsigned>(PXR_VERSION),                                    \
        _priority                                                              \
    };                                                                         \
    static void _name(__VA_ARGS__)

#elif defined(ARCH_COMPILER_GCC) || defined(ARCH_COMPILER_CLANG)

// The used attribute is required to prevent these apparently unused functions
// from being removed by the linker.
#   define ARCH_CONSTRUCTOR(_name, _priority, ...) \
        __attribute__((used, section(".pxrctor"), constructor((_priority) + 100))) \
        static void _name(__VA_ARGS__)
#   define ARCH_DESTRUCTOR(_name, _priority, ...) \
        __attribute__((used, section(".pxrdtor"), destructor((_priority) + 100))) \
        static void _name(__VA_ARGS__)

#elif defined(ARCH_OS_WINDOWS)
    
#    include "pxr/base/arch/api.h"
    
// Entry for a constructor/destructor in the custom section.
    __declspec(align(16))
    struct Arch_ConstructorEntry {
        typedef void (__cdecl *Type)(void);
        Type function;
        unsigned int version:24;    // USD version
        unsigned int priority:8;    // Priority of function
    };

// Declare the special sections.
#   pragma section(".pxrctor", read)
#   pragma section(".pxrdtor", read)

// Objects of this type run the ARCH_CONSTRUCTOR and ARCH_DESTRUCTOR functions
// for the library containing the object in the c'tor and d'tor, respectively.
// Each HMODULE is handled at most once.
struct Arch_ConstructorInit {
    ARCH_API Arch_ConstructorInit();
    ARCH_API ~Arch_ConstructorInit();
};

// Emit a Arch_ConstructorEntry in the .pxrctor section.  The namespace and
// extern are to convince the compiler and linker to leave the object in the
// final library/executable instead of stripping it out.  In clang/gcc we use
// __attribute__((used)) to do that.
#   define ARCH_CONSTRUCTOR(_name, _priority, ...)                             \
    static void _name(__VA_ARGS__);                                            \
    namespace {                                                                \
    __declspec(allocate(".pxrctor"))                                           \
    extern const Arch_ConstructorEntry                                         \
    _ARCH_CAT_NOEXPAND(arch_ctor_, _name) = {                                  \
        reinterpret_cast<Arch_ConstructorEntry::Type>(&_name),                 \
        static_cast<unsigned>(PXR_VERSION),                                    \
        _priority                                                              \
    };                                                                         \
    }                                                                          \
    _ARCH_ENSURE_PER_LIB_INIT(Arch_ConstructorInit, _archCtorInit);            \
    static void _name(__VA_ARGS__)

    // Emit a Arch_ConstructorEntry in the .pxrdtor section.
#   define ARCH_DESTRUCTOR(_name, _priority, ...)                              \
    static void _name(__VA_ARGS__);                                            \
    namespace {                                                                \
    __declspec(allocate(".pxrdtor"))                                           \
    extern const Arch_ConstructorEntry                                         \
    _ARCH_CAT_NOEXPAND(arch_dtor_, _name) = {                                  \
        reinterpret_cast<Arch_ConstructorEntry::Type>(&_name),                 \
        static_cast<unsigned>(PXR_VERSION),                                    \
        _priority                                                              \
    };                                                                         \
    }                                                                          \
    _ARCH_ENSURE_PER_LIB_INIT(Arch_ConstructorInit, _archCtorInit);            \
    static void _name(__VA_ARGS__)

#else

// Leave macros undefined so we'll fail to build on a new system/compiler
// rather than fail mysteriously at runtime.

#endif

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_ARCH_ATTRIBUTES_H
