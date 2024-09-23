//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_OBJECT_FUNCTION_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_OBJECT_FUNCTION_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/object/function.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"
# include "pxr/external/boost/python/args_fwd.hpp"
# include "pxr/external/boost/python/handle.hpp"
# include "pxr/external/boost/python/object_core.hpp"
# include "pxr/external/boost/python/object/py_function.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace objects { 


struct PXR_BOOST_PYTHON_DECL function : PyObject
{
    function(
        py_function const&
        , python::detail::keyword const* names_and_defaults
        , unsigned num_keywords);
      
    ~function();
    
    PyObject* call(PyObject*, PyObject*) const;

    // Add an attribute to the name_space with the given name. If it is
    // a function object (this class), and an existing function is
    // already there, add it as an overload.
    static void add_to_namespace(
        object const& name_space, char const* name, object const& attribute);

    static void add_to_namespace(
        object const& name_space, char const* name, object const& attribute, char const* doc);

    object const& doc() const;
    void doc(object const& x);
    
    object const& name() const;

    object const& get_namespace() const { return m_namespace; }
    
 private: // helper functions
    object signature(bool show_return_type=false) const;
    object signatures(bool show_return_type=false) const;
    void argument_error(PyObject* args, PyObject* keywords) const;
    void add_overload(handle<function> const&);
    
 private: // data members
    py_function m_fn;
    handle<function> m_overloads;
    object m_name;
    object m_namespace;
    object m_doc;
    object m_arg_names;
    unsigned m_nkeyword_values;
    friend class function_doc_signature_generator;
};

//
// implementations
//
inline object const& function::doc() const
{
    return this->m_doc;
}

inline void function::doc(object const& x)
{
    this->m_doc = x;
}

inline object const& function::name() const
{
    return this->m_name;
}
  
}}} // namespace PXR_BOOST_NAMESPACE::python::objects

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_OBJECT_FUNCTION_HPP
