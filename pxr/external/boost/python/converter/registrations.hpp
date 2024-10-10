//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_REGISTRATIONS_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_REGISTRATIONS_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/converter/registrations.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"

# include "pxr/external/boost/python/type_id.hpp"

# include "pxr/external/boost/python/converter/convertible_function.hpp"
# include "pxr/external/boost/python/converter/constructor_function.hpp"
# include "pxr/external/boost/python/converter/to_python_function_type.hpp"


namespace PXR_BOOST_NAMESPACE { namespace python { namespace converter { 

struct lvalue_from_python_chain
{
    convertible_function convert;
    lvalue_from_python_chain* next;
};

struct rvalue_from_python_chain
{
    convertible_function convertible;
    constructor_function construct;
    PyTypeObject const* (*expected_pytype)();
    rvalue_from_python_chain* next;
};

struct PXR_BOOST_PYTHON_DECL registration
{
 public: // member functions
    explicit registration(type_info target, bool is_shared_ptr = false);
   ~registration();
    
    // Convert the appropriately-typed data to Python
    PyObject* to_python(void const volatile*) const;

    // Return the class object, or raise an appropriate Python
    // exception if no class has been registered.
    PyTypeObject* get_class_object() const;

    // Return common denominator of the python class objects, 
    // convertable to target. Inspects the m_class_object and the value_chains.
    PyTypeObject const* expected_from_python_type() const;
    PyTypeObject const* to_python_target_type() const;

 public: // data members. So sue me.
    const python::type_info target_type;

    // The chain of eligible from_python converters when an lvalue is required
    lvalue_from_python_chain* lvalue_chain;

    // The chain of eligible from_python converters when an rvalue is acceptable
    rvalue_from_python_chain* rvalue_chain;
    
    // The class object associated with this type
    PyTypeObject* m_class_object;

    // The unique to_python converter for the associated C++ type.
    to_python_function_t m_to_python;
    PyTypeObject const* (*m_to_python_target_type)();


    // True iff this type is a shared_ptr.  Needed for special rvalue
    // from_python handling.
    const bool is_shared_ptr;
};

//
// implementations
//
inline registration::registration(type_info target_type, bool is_shared_ptr)
    : target_type(target_type)
      , lvalue_chain(0)
      , rvalue_chain(0)
      , m_class_object(0)
      , m_to_python(0)
      , m_to_python_target_type(0)
      , is_shared_ptr(is_shared_ptr)
{}

inline bool operator<(registration const& lhs, registration const& rhs)
{
    return lhs.target_type < rhs.target_type;
}

}}} // namespace PXR_BOOST_NAMESPACE::python::converter

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_REGISTRATIONS_HPP
