//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_OBJECT_MAKE_PTR_INSTANCE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_OBJECT_MAKE_PTR_INSTANCE_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/object/make_ptr_instance.hpp>
#else

# include "pxr/external/boost/python/object/make_instance.hpp"
# include "pxr/external/boost/python/converter/registry.hpp"
# include "pxr/external/boost/python/detail/type_traits.hpp"
# include <boost/get_pointer.hpp>
# include <typeinfo>

namespace PXR_BOOST_NAMESPACE {
    using boost::get_pointer; // Enable ADL for boost types
}

namespace PXR_BOOST_NAMESPACE { namespace python { namespace objects { 

template <class T, class Holder>
struct make_ptr_instance
    : make_instance_impl<T, Holder, make_ptr_instance<T,Holder> >
{
    template <class Arg>
    static inline Holder* construct(void* storage, PyObject*, Arg& x)
    {
      return new (storage) Holder(std::move(x));
    }
    
    template <class Ptr>
    static inline PyTypeObject* get_class_object(Ptr const& x)
    {
        return get_class_object_impl(get_pointer(x));
    }
#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
    static inline PyTypeObject const* get_pytype()
    {
        return converter::registered<T>::converters.get_class_object();
    }
#endif
 private:
    template <class U>
    static inline PyTypeObject* get_class_object_impl(U const volatile* p)
    {
        if (p == 0)
            return 0; // means "return None".

        PyTypeObject* derived = get_derived_class_object(
            BOOST_DEDUCED_TYPENAME PXR_BOOST_NAMESPACE::python::detail::is_polymorphic<U>::type(), p);
        
        if (derived)
            return derived;
        return converter::registered<T>::converters.get_class_object();
    }
    
    template <class U>
    static inline PyTypeObject* get_derived_class_object(PXR_BOOST_NAMESPACE::python::detail::true_, U const volatile* x)
    {
        converter::registration const* r = converter::registry::query(
            type_info(typeid(*x))
        );
        return r ? r->m_class_object : 0;
    }
    
    template <class U>
    static inline PyTypeObject* get_derived_class_object(PXR_BOOST_NAMESPACE::python::detail::false_, U*)
    {
        return 0;
    }
};
  

}}} // namespace PXR_BOOST_NAMESPACE::python::object

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_OBJECT_MAKE_PTR_INSTANCE_HPP
