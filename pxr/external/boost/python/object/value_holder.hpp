//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

# ifndef PXR_EXTERNAL_BOOST_PYTHON_OBJECT_VALUE_HOLDER_HPP
#  define PXR_EXTERNAL_BOOST_PYTHON_OBJECT_VALUE_HOLDER_HPP 

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/object/value_holder.hpp>
#else

#  include "pxr/external/boost/python/object/value_holder_fwd.hpp"

#  include "pxr/external/boost/python/instance_holder.hpp"
#  include "pxr/external/boost/python/type_id.hpp"
#  include "pxr/external/boost/python/wrapper.hpp"

#  include "pxr/external/boost/python/object/inheritance_query.hpp"
#  include "pxr/external/boost/python/object/forward.hpp"

#  include "pxr/external/boost/python/detail/force_instantiate.hpp"
#  include "pxr/external/boost/python/detail/preprocessor.hpp"

#  include <memory>

namespace PXR_BOOST_NAMESPACE { namespace python { namespace objects { 

template <class Value>
struct value_holder : instance_holder
{
    typedef Value held_type;
    typedef Value value_type;

    // Forward construction to the held object
    template <class... A>
    value_holder(
      PyObject* self, A... a)
        : m_held(objects::do_unforward(a, 0)...)
    {
        python::detail::initialize_wrapper(self, std::addressof(this->m_held));
    }

 private: // required holder implementation
    void* holds(type_info, bool null_ptr_only);
    
    template <class T>
    inline void* holds_wrapped(type_info dst_t, wrapper<T>*,T* p)
    {
        return python::type_id<T>() == dst_t ? p : 0;
    }
    
    inline void* holds_wrapped(type_info, ...)
    {
        return 0;
    }
 private: // data members
    Value m_held;
};

template <class Value, class Held>
struct value_holder_back_reference : instance_holder
{
    typedef Held held_type;
    typedef Value value_type;
    
    // Forward construction to the held object
    template <class... A>
    value_holder_back_reference(
        PyObject* p, A... a)
        : m_held(p, objects::do_unforward(a, 0)...)
    {
    }

private: // required holder implementation
    void* holds(type_info, bool null_ptr_only);

 private: // data members
    Held m_held;
};

template <class Value>
void* value_holder<Value>::holds(type_info dst_t, bool /*null_ptr_only*/)
{
    if (void* wrapped = holds_wrapped(dst_t, std::addressof(m_held), std::addressof(m_held)))
        return wrapped;
    
    type_info src_t = python::type_id<Value>();
    return src_t == dst_t ? std::addressof(m_held)
        : find_static_type(std::addressof(m_held), src_t, dst_t);
}

template <class Value, class Held>
void* value_holder_back_reference<Value,Held>::holds(
    type_info dst_t, bool /*null_ptr_only*/)
{
    type_info src_t = python::type_id<Value>();
    Value* x = &m_held;
    
    if (dst_t == src_t)
        return x;
    else if (dst_t == python::type_id<Held>())
        return &m_held;
    else
        return find_static_type(x, src_t, dst_t);
}

}}} // namespace PXR_BOOST_NAMESPACE::python::objects

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
# endif // PXR_EXTERNAL_BOOST_PYTHON_OBJECT_VALUE_HOLDER_HPP
