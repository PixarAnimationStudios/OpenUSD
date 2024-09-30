//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_OBJECT_MAKE_INSTANCE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_OBJECT_MAKE_INSTANCE_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/object/make_instance.hpp>
#else

# include "pxr/external/boost/python/ref.hpp"
# include "pxr/external/boost/python/detail/prefix.hpp"
# include "pxr/external/boost/python/object/instance.hpp"
# include "pxr/external/boost/python/converter/registered.hpp"
# include "pxr/external/boost/python/detail/decref_guard.hpp"
# include "pxr/external/boost/python/detail/type_traits.hpp"
# include "pxr/external/boost/python/detail/none.hpp"
# include "pxr/external/boost/python/detail/mpl2/or.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace objects { 

template <class T, class Holder, class Derived>
struct make_instance_impl
{
    typedef objects::instance<Holder> instance_t;
        
    template <class Arg>
    static inline PyObject* execute(Arg& x)
    {
        static_assert((python::detail::mpl2::or_<PXR_BOOST_NAMESPACE::python::detail::is_class<T>,
                PXR_BOOST_NAMESPACE::python::detail::is_union<T> >::value));

        PyTypeObject* type = Derived::get_class_object(x);

        if (type == 0)
            return python::detail::none();

        PyObject* raw_result = type->tp_alloc(
            type, objects::additional_instance_size<Holder>::value);
          
        if (raw_result != 0)
        {
            python::detail::decref_guard protect(raw_result);
            
            instance_t* instance = (instance_t*)raw_result;
            
            // construct the new C++ object and install the pointer
            // in the Python object.
            Holder *holder =Derived::construct(instance->storage.bytes, (PyObject*)instance, x);
            holder->install(raw_result);
              
            // Note the position of the internally-stored Holder,
            // for the sake of destruction
            const size_t offset = reinterpret_cast<size_t>(holder) -
              reinterpret_cast<size_t>(instance->storage.bytes) + offsetof(instance_t, storage);
            Py_SET_SIZE(instance, offset);

            // Release ownership of the python object
            protect.cancel();
        }
        return raw_result;
    }
};
    

template <class T, class Holder>
struct make_instance
    : make_instance_impl<T, Holder, make_instance<T,Holder> >
{
    template <class U>
    static inline PyTypeObject* get_class_object(U&)
    {
        return converter::registered<T>::converters.get_class_object();
    }
    
    static inline Holder* construct(void* storage, PyObject* instance, std::reference_wrapper<T const> x)
    {
        size_t allocated = objects::additional_instance_size<Holder>::value;
        void* aligned_storage = std::align(PXR_BOOST_NAMESPACE::python::detail::alignment_of<Holder>::value,
                                           sizeof(Holder), storage, allocated);
        return new (aligned_storage) Holder(instance, x);
    }
};
  

}}} // namespace PXR_BOOST_NAMESPACE::python::object

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_OBJECT_MAKE_INSTANCE_HPP
