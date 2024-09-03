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

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/module_init.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"
# include <boost/preprocessor/cat.hpp>
# include <boost/preprocessor/stringize.hpp>

# ifndef PXR_BOOST_PYTHON_MODULE_INIT

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail {

#  if PY_VERSION_HEX >= 0x03000000

PXR_BOOST_PYTHON_DECL PyObject* init_module(PyModuleDef&, void(*)());

#else

PXR_BOOST_PYTHON_DECL PyObject* init_module(char const* name, void(*)());

#endif

}}}

#  if PY_VERSION_HEX >= 0x03000000

#   define _PXR_BOOST_PYTHON_MODULE_INIT(name) \
  PyObject* BOOST_PP_CAT(PyInit_, name)()  \
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
        BOOST_PP_STRINGIZE(name), \
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
        moduledef, BOOST_PP_CAT(init_module_, name) ); \
  } \
  void BOOST_PP_CAT(init_module_, name)()

#  else

#   define _PXR_BOOST_PYTHON_MODULE_INIT(name)              \
  void BOOST_PP_CAT(init,name)()                        \
{                                                       \
    PXR_BOOST_NAMESPACE::python::detail::init_module(                 \
        BOOST_PP_STRINGIZE(name),&BOOST_PP_CAT(init_module_,name)); \
}                                                       \
  void BOOST_PP_CAT(init_module_,name)()

#  endif

#  define PXR_BOOST_PYTHON_MODULE_INIT(name)                       \
  void BOOST_PP_CAT(init_module_,name)();                      \
extern "C" BOOST_SYMBOL_EXPORT _PXR_BOOST_PYTHON_MODULE_INIT(name)

# endif

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_MODULE_INIT_HPP
