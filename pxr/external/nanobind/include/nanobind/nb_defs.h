/*
    nanobind/nb_defs.h: Preprocessor definitions used by the project

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#define NB_STRINGIFY(x) #x
#define NB_TOSTRING(x) NB_STRINGIFY(x)
#define NB_CONCAT_IMPL(first, second) first##second
#define NB_CONCAT(first, second) NB_CONCAT_IMPL(first, second)
#define NB_NEXT_OVERLOAD ((PyObject *) 1) // special failure return code

#if !defined(NAMESPACE_BEGIN)
#  define NAMESPACE_BEGIN(name) namespace name {
#endif

#if !defined(NAMESPACE_END)
#  define NAMESPACE_END(name) }
#endif

#if defined(_WIN32)
#  define NB_EXPORT          __declspec(dllexport)
#  define NB_IMPORT          __declspec(dllimport)
#  define NB_INLINE          __forceinline
#  define NB_NOINLINE        __declspec(noinline)
#  define NB_INLINE_LAMBDA
#  define NB_NOUNROLL
#else
#  define NB_EXPORT          __attribute__ ((visibility("default")))
#  define NB_IMPORT          NB_EXPORT
#  define NB_INLINE          inline __attribute__((always_inline))
#  define NB_NOINLINE        __attribute__((noinline))
#  if defined(__clang__)
#    define NB_INLINE_LAMBDA __attribute__((always_inline))
#    define NB_NOUNROLL      _Pragma("nounroll")
#  else
#    define NB_INLINE_LAMBDA
#    if defined(__GNUC__)
#      define NB_NOUNROLL    _Pragma("GCC unroll 0")
#    else
#      define NB_NOUNROLL
#    endif
#  endif
#endif

#if defined(__GNUC__) && !defined(_WIN32)
#  define NB_NAMESPACE nanobind __attribute__((visibility("hidden")))
#else
#  define NB_NAMESPACE nanobind
#endif

#if defined(__GNUC__)
#  define NB_UNLIKELY(x) __builtin_expect(bool(x), 0)
#  define NB_LIKELY(x)   __builtin_expect(bool(x), 1)
#else
#  define NB_LIKELY(x) x
#  define NB_UNLIKELY(x) x
#endif

#if defined(NB_SHARED)
#  if defined(NB_BUILD)
#    define NB_CORE NB_EXPORT
#  else
#    define NB_CORE NB_IMPORT
#  endif
#else
#  define NB_CORE
#endif

#if !defined(NB_SHARED) && defined(__GNUC__) && !defined(_WIN32)
#  define NB_EXPORT_SHARED __attribute__ ((visibility("hidden")))
#else
#  define NB_EXPORT_SHARED
#endif

#if defined(__cpp_lib_char8_t) && __cpp_lib_char8_t >= 201811L
#  define NB_HAS_U8STRING
#endif

// The oldest Python version that the compiled binary must be able to run on
#if defined(Py_LIMITED_API)
#  define NB_PYTHON_VERSION Py_LIMITED_API
#else
#  define NB_PYTHON_VERSION PY_VERSION_HEX
#endif

// Singletons (True, False, None) are immortal on Python 3.12+
#if NB_PYTHON_VERSION >= 0x030C0000
#  define NB_IMMORTAL_SINGLETONS 1
#else
#  define NB_IMMORTAL_SINGLETONS 0
#endif

// Cache immortal singletons to avoid PLT calls to Py_GetConstantBorrowed in 3.13+.
#if defined(Py_LIMITED_API) && NB_PYTHON_VERSION >= 0x030D0000
#  define NB_CACHE_SINGLETONS 1
#else
#  define NB_CACHE_SINGLETONS 0
#endif

#if defined(Py_LIMITED_API)
#  if Py_LIMITED_API < 0x030A0000 || defined(PYPY_VERSION)
#    error "nanobind can target Python's limited API, but this requires CPython >= 3.10"
#  endif
// Prefer 'Py_SIZE' for tuples/lists since it stays inline in the stable ABI
#  define NB_TUPLE_GET_SIZE Py_SIZE
#  define NB_TUPLE_GET_ITEM PyTuple_GetItem
#  define NB_TUPLE_SET_ITEM PyTuple_SetItem
#  define NB_LIST_GET_SIZE Py_SIZE
#  define NB_LIST_GET_ITEM PyList_GetItem
#  define NB_LIST_SET_ITEM PyList_SetItem
#  define NB_DICT_GET_SIZE PyDict_Size
#  define NB_SET_GET_SIZE PySet_Size
#else
#  define NB_TUPLE_GET_SIZE PyTuple_GET_SIZE
#  define NB_TUPLE_GET_ITEM PyTuple_GET_ITEM
#  define NB_TUPLE_SET_ITEM PyTuple_SET_ITEM
#  define NB_LIST_GET_SIZE PyList_GET_SIZE
#  define NB_LIST_GET_ITEM PyList_GET_ITEM
#  define NB_LIST_SET_ITEM PyList_SET_ITEM
#  define NB_DICT_GET_SIZE PyDict_GET_SIZE
#  define NB_SET_GET_SIZE PySet_GET_SIZE
#endif

#if defined(PYPY_VERSION_NUM) && PYPY_VERSION_NUM < 0x07030c00
#    error "nanobind requires a newer PyPy version (>= 7.3.12)"
#endif

#if defined(NB_BACKEND_MODULE) && defined(PYPY_VERSION)
#    error "nanobind's split mode requires CPython"
#endif

/* python_error is unusable with libc++ on ELF platforms, where typeinfo is
   compared by pointer. Python imports extensions with RTLD_LOCAL, so the
   weak symbols of the extensions are not correctly merged. */
#if defined(NB_BACKEND_MODULE) && defined(_LIBCPP_VERSION) && !defined(__APPLE__)
#    error "nanobind's split mode requires libstdc++ on ELF platforms"
#endif

#if defined(NB_BACKEND_MODULE) && !defined(Py_LIMITED_API)
#    error "nanobind's split mode requires targeting the Python stable ABI (Py_LIMITED_API)"
#endif

#if defined(NB_BACKEND_MODULE) && defined(Py_GIL_DISABLED) &&                  \
    defined(Py_LIMITED_API) && Py_LIMITED_API < 0x030F0000
#    error "nanobind's split mode requires the 'abi3t' stable ABI (Python >= 3.15) on free-threaded builds"
#endif

#if defined(NB_FREE_THREADED) && !defined(Py_GIL_DISABLED)
#    error "Free-threaded extensions require a free-threaded version of Python"
#endif

#if defined(NB_DOMAIN)
#  define NB_DOMAIN_STR NB_TOSTRING(NB_DOMAIN)
#else
#  define NB_DOMAIN_STR ""
#endif

#if !defined(PYPY_VERSION)
#  define NB_TYPE_GET_SLOT_IMPL 0
#  if PY_VERSION_HEX < 0x030C0000
#    define NB_TYPE_FROM_METACLASS_IMPL 1 // Custom implementation of PyType_FromMetaclass
#  else
#    define NB_TYPE_FROM_METACLASS_IMPL 0
#  endif
#else
#  define NB_TYPE_FROM_METACLASS_IMPL 1
#  define NB_TYPE_GET_SLOT_IMPL 1
#endif

#if defined(Py_LIMITED_API)
#  define NB_DYNAMIC_VERSION Py_Version
#else
#  define NB_DYNAMIC_VERSION PY_VERSION_HEX
#endif

#define NB_NONCOPYABLE(X)                                                      \
    X(const X &) = delete;                                                     \
    X &operator=(const X &) = delete;

#if defined(_MSC_VER) && !defined(__clang__)
#  define NB_UNREACHABLE() __assume(0)
#else
#  define NB_UNREACHABLE() __builtin_unreachable()
#endif

#if defined(_WIN32)
#  define NB_HIDDEN
#else
#  define NB_HIDDEN __attribute__((visibility("hidden")))
#endif

// PY_VECTORCALL_ARGUMENTS_OFFSET is hidden from the limited API before Python
// 3.12, but its value is frozen by the vector call protocol (PEP 590)
#if defined(PY_VECTORCALL_ARGUMENTS_OFFSET)
#  define NB_VECTORCALL_ARGUMENTS_OFFSET PY_VECTORCALL_ARGUMENTS_OFFSET
#else
#  define NB_VECTORCALL_ARGUMENTS_OFFSET ((size_t) 1 << (8 * sizeof(size_t) - 1))
#endif

// Decode the argument count of a vector call. Equivalent to
// PyVectorcall_NARGS(), which the Python 3.10 limited API does not expose and
// which costs an indirect PLT call in later versions.
#define NB_VECTORCALL_NARGS(n)                                                 \
    ((Py_ssize_t) ((n) & ~NB_VECTORCALL_ARGUMENTS_OFFSET))

#if defined(NB_BUILD) || !defined(NB_BACKEND_MODULE)
#  define NB_CALL(name) ::nanobind::detail::name
#else
#  define NB_CALL(name) ::nanobind::detail::nb_backend.name
#endif

// Helper macros to ensure macro arguments are expanded before token pasting/stringification
#define NB_MODULE_IMPL(name, variable) NB_MODULE_IMPL2(name, variable)
#define NB_MODULE_IMPL2(name, variable)                                        \
    static void nanobind_##name##_exec_impl(nanobind::module_);                \
    static int nanobind_##name##_exec(PyObject *m) {                           \
        nanobind::detail::nb_internals *p =                                    \
            NB_CALL(nb_module_init)(NB_DOMAIN_STR, m);                         \
        if (!p)                                                                \
            return -1;                                                         \
        if (nanobind::detail::internals != p)                                  \
            nanobind::detail::internals = p;                                   \
        try {                                                                  \
            nanobind_##name##_exec_impl(                                       \
                nanobind::borrow<nanobind::module_>(m));                       \
            return 0;                                                          \
        } catch (nanobind::python_error &e) {                                  \
            e.restore();                                                       \
            nanobind::chain_error(                                             \
                PyExc_ImportError,                                             \
                "Encountered an error while initializing the extension.");     \
        } catch (const std::exception &e) {                                    \
            PyErr_SetString(PyExc_ImportError, e.what());                      \
        }                                                                      \
        return -1;                                                             \
    }                                                                          \
    static PyObject *nanobind_##name##_def = nullptr;                          \
    extern "C" [[maybe_unused]] NB_EXPORT PyObject *PyInit_##name(void);       \
    extern "C" PyObject *PyInit_##name(void) {                                 \
        nanobind::detail::init_singletons();                                   \
        if (!nanobind::detail::nb_backend_init(#name))                         \
            return nullptr;                                                    \
        if (!nanobind_##name##_def)                                            \
            nanobind_##name##_def = NB_CALL(module_new)(                       \
                #name, nullptr, (void *) nanobind_##name##_exec,               \
                NB_ABI_MINOR_TAG);                                             \
        return nanobind_##name##_def;                                          \
    }                                                                          \
    void nanobind_##name##_exec_impl(nanobind::module_ variable)

#define NB_MODULE(name, variable) NB_MODULE_IMPL(name, variable)
