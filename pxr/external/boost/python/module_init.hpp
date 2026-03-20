//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_MODULE_INIT_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_MODULE_INIT_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

# include "pxr/external/boost/python/detail/prefix.hpp"

# ifndef PXR_BOOST_PYTHON_MODULE_INIT

namespace PXR_BOOST_NAMESPACE { namespace python {

#ifdef PXR_BOOST_PYTHON_HAS_CXX11
// Use to activate the Py_MOD_GIL_NOT_USED flag.
class mod_gil_not_used {
public:
    explicit mod_gil_not_used(bool flag = true) : flag_(flag) {}
    bool flag() const { return flag_; }

private:
    bool flag_;
};

namespace detail {

inline bool gil_not_used_option() { return false; }
template <typename F, typename... O>
bool gil_not_used_option(F &&, O &&...o);
template <typename... O>
inline bool gil_not_used_option(mod_gil_not_used f, O &&...o) {
    return f.flag() || gil_not_used_option(o...);
}
template <typename F, typename... O>
inline bool gil_not_used_option(F &&, O &&...o) {
    return gil_not_used_option(o...);
}

}
#endif // PXR_BOOST_PYTHON_HAS_CXX11

namespace detail {

#  if PY_VERSION_HEX >= 0x03000000

PXR_BOOST_PYTHON_DECL PyObject* init_module(PyModuleDef&, void(*)(), bool gil_not_used = false);

#else

PXR_BOOST_PYTHON_DECL PyObject* init_module(char const* name, void(*)());

#endif

}}}

#define PXR_BOOST_PYTHON_PP_CAT_IMPL(x, y) x ## y
#define PXR_BOOST_PYTHON_PP_CAT(x, y) PXR_BOOST_PYTHON_PP_CAT_IMPL(x, y)
#define PXR_BOOST_PYTHON_PP_STRINGIZE_IMPL(x) #x
#define PXR_BOOST_PYTHON_PP_STRINGIZE(x) PXR_BOOST_PYTHON_PP_STRINGIZE_IMPL(x)

#  if PY_VERSION_HEX >= 0x03000000

#   ifdef PXR_BOOST_PYTHON_HAS_CXX11
#    define _PXR_BOOST_PYTHON_MODULE_INIT(name, ...) \
  PyObject* PXR_BOOST_PYTHON_PP_CAT(PyInit_, name)()  \
  { \
    static PyModuleDef_Base initial_m_base = { \
        PyObject_HEAD_INIT(NULL) \
        0, /* m_init */ \
        0, /* m_index */ \
        0 /* m_copy */ };  \
    static PyMethodDef initial_methods[] = { { 0, 0, 0, 0 } }; \
 \
    static struct PyModuleDef moduledef = { \
        initial_m_base, \
        PXR_BOOST_PYTHON_PP_STRINGIZE(name), \
        0, /* m_doc */ \
        -1, /* m_size */ \
        initial_methods, \
        0,  /* m_reload */ \
        0, /* m_traverse */ \
        0, /* m_clear */ \
        0,  /* m_free */ \
    }; \
 \
    return PXR_BOOST_NAMESPACE::python::detail::init_module( \
        moduledef, PXR_BOOST_PYTHON_PP_CAT(init_module_, name), \
        PXR_BOOST_NAMESPACE::python::detail::gil_not_used_option(__VA_ARGS__) ); \
  } \
  void PXR_BOOST_PYTHON_PP_CAT(init_module_, name)()

#   else // !PXR_BOOST_PYTHON_HAS_CXX11
#    define _PXR_BOOST_PYTHON_MODULE_INIT(name) \
  PyObject* PXR_BOOST_PYTHON_PP_CAT(PyInit_, name)()  \
  { \
    static PyModuleDef_Base initial_m_base = { \
        PyObject_HEAD_INIT(NULL) \
        0, /* m_init */ \
        0, /* m_index */ \
        0 /* m_copy */ };  \
    static PyMethodDef initial_methods[] = { { 0, 0, 0, 0 } }; \
 \
    static struct PyModuleDef moduledef = { \
        initial_m_base, \
        PXR_BOOST_PYTHON_PP_STRINGIZE(name), \
        0, /* m_doc */ \
        -1, /* m_size */ \
        initial_methods, \
        0,  /* m_reload */ \
        0, /* m_traverse */ \
        0, /* m_clear */ \
        0,  /* m_free */ \
    }; \
 \
    return PXR_BOOST_NAMESPACE::python::detail::init_module( \
        moduledef, PXR_BOOST_PYTHON_PP_CAT(init_module_, name) ); \
  } \
  void PXR_BOOST_PYTHON_PP_CAT(init_module_, name)()
#   endif // PXR_BOOST_PYTHON_HAS_CXX11

#  else

#   define _PXR_BOOST_PYTHON_MODULE_INIT(name)              \
  void PXR_BOOST_PYTHON_PP_CAT(init,name)()                        \
{                                                       \
    PXR_BOOST_NAMESPACE::python::detail::init_module(                 \
        PXR_BOOST_PYTHON_PP_STRINGIZE(name),&PXR_BOOST_PYTHON_PP_CAT(init_module_,name)); \
}                                                       \
  void PXR_BOOST_PYTHON_PP_CAT(init_module_,name)()

#  endif

#  if defined(PXR_BOOST_PYTHON_HAS_CXX11) && (PY_VERSION_HEX >= 0x03000000)
#   define PXR_BOOST_PYTHON_MODULE_INIT(name, ...)                  \
  void PXR_BOOST_PYTHON_PP_CAT(init_module_,name)();                      \
extern "C" PXR_BOOST_PYTHON_SYMBOL_EXPORT _PXR_BOOST_PYTHON_MODULE_INIT(name, __VA_ARGS__)
#  else
#   define PXR_BOOST_PYTHON_MODULE_INIT(name)                       \
  void PXR_BOOST_PYTHON_PP_CAT(init_module_,name)();                      \
extern "C" PXR_BOOST_PYTHON_SYMBOL_EXPORT _PXR_BOOST_PYTHON_MODULE_INIT(name)
#  endif // PXR_BOOST_PYTHON_HAS_CXX11 && Python 3

# endif

#endif // PXR_EXTERNAL_BOOST_PYTHON_MODULE_INIT_HPP
