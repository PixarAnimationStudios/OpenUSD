//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Copyright Stefan Seefeld 2016.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef PXR_EXTERNAL_BOOST_PYTHON_TO_PYTHON_VALUE_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_TO_PYTHON_VALUE_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/to_python_value.hpp>
#else

#include "pxr/external/boost/python/detail/prefix.hpp"

#include "pxr/external/boost/python/refcount.hpp"
#include "pxr/external/boost/python/tag.hpp"
#include "pxr/external/boost/python/handle.hpp"

#include "pxr/external/boost/python/converter/registry.hpp"
#include "pxr/external/boost/python/converter/registered.hpp"
#include "pxr/external/boost/python/converter/builtin_converters.hpp"
#include "pxr/external/boost/python/converter/object_manager.hpp"
#include "pxr/external/boost/python/converter/shared_ptr_to_python.hpp"

#include "pxr/external/boost/python/detail/type_traits.hpp"
#include "pxr/external/boost/python/detail/value_is_shared_ptr.hpp"
#include "pxr/external/boost/python/detail/value_arg.hpp"

#include "pxr/external/boost/python/detail/mpl2/if.hpp"
#include "pxr/external/boost/python/detail/mpl2/or.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { 

namespace detail
{
#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES

template <bool is_const_ref>
struct object_manager_get_pytype
{
   template <class U>
   static PyTypeObject const* get( U& (*)() =0)
   {
      return converter::object_manager_traits<U>::get_pytype();
   }
};

template <>
struct object_manager_get_pytype<true>
{
   template <class U>
   static PyTypeObject const* get( U const& (*)() =0)
   {
      return converter::object_manager_traits<U>::get_pytype();
   }
};

#endif

  template <class T>
  struct object_manager_to_python_value
  {
      typedef typename value_arg<T>::type argument_type;
    
      PyObject* operator()(argument_type) const;
#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
      typedef detail::mpl2::bool_<is_handle<T>::value> is_t_handle;
      typedef indirect_traits::is_reference_to_const<T> is_t_const;
      PyTypeObject const* get_pytype() const {
          return get_pytype_aux((is_t_handle*)0);
      }

      inline static PyTypeObject const* get_pytype_aux(detail::mpl2::true_*) {return converter::object_manager_traits<T>::get_pytype();}
      
      inline static PyTypeObject const* get_pytype_aux(detail::mpl2::false_* ) 
      {
          return object_manager_get_pytype<is_t_const::value>::get((T(*)())0);
      }
      
#endif 

      // This information helps make_getter() decide whether to try to
      // return an internal reference or not. I don't like it much,
      // but it will have to serve for now.
      static constexpr bool uses_registry = false;
  };

  
  template <class T>
  struct registry_to_python_value
  {
      typedef typename value_arg<T>::type argument_type;
    
      PyObject* operator()(argument_type) const;
#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
      PyTypeObject const* get_pytype() const {return converter::registered<T>::converters.to_python_target_type();}
#endif

      // This information helps make_getter() decide whether to try to
      // return an internal reference or not. I don't like it much,
      // but it will have to serve for now.
      static constexpr bool uses_registry = true;
  };

  template <class T>
  struct shared_ptr_to_python_value
  {
      typedef typename value_arg<T>::type argument_type;
    
      PyObject* operator()(argument_type) const;
#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
      PyTypeObject const* get_pytype() const {return get_pytype((type<argument_type>*)0);}
#endif 
      // This information helps make_getter() decide whether to try to
      // return an internal reference or not. I don't like it much,
      // but it will have to serve for now.
      static constexpr bool uses_registry = false;
  private:
#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
#ifdef PXR_BOOST_PYTHON_HAS_BOOST_SHARED_PTR
    template <class U>
    PyTypeObject const* get_pytype(type<boost::shared_ptr<U> &> *) const {return converter::registered<U>::converters.to_python_target_type();}
    template <class U>
    PyTypeObject const* get_pytype(type<const boost::shared_ptr<U> &> *) const {return converter::registered<U>::converters.to_python_target_type();}
#endif
    template <class U>
    PyTypeObject const* get_pytype(type<std::shared_ptr<U> &> *) const {return converter::registered<U>::converters.to_python_target_type();}
    template <class U>
    PyTypeObject const* get_pytype(type<const std::shared_ptr<U> &> *) const {return converter::registered<U>::converters.to_python_target_type();}
#endif
  };
}

template <class T>
struct to_python_value
    : detail::mpl2::if_<
          detail::value_is_shared_ptr<T>
        , detail::shared_ptr_to_python_value<T>
        , typename detail::mpl2::if_<
              detail::mpl2::or_<
                  converter::is_object_manager<T>
                , converter::is_reference_to_object_manager<T>
              >
            , detail::object_manager_to_python_value<T>
            , detail::registry_to_python_value<T>
          >::type
      >::type
{
};

//
// implementation 
//
namespace detail
{
  template <class T>
  inline PyObject* registry_to_python_value<T>::operator()(argument_type x) const
  {
      return converter::registered<argument_type>::converters.to_python(&x);
  }

  template <class T>
  inline PyObject* object_manager_to_python_value<T>::operator()(argument_type x) const
  {
      return python::upcast<PyObject>(
          python::xincref(
              get_managed_object(x, tag))
          );
  }

  template <class T>
  inline PyObject* shared_ptr_to_python_value<T>::operator()(argument_type x) const
  {
      return converter::shared_ptr_to_python(x);
  }
}

}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif
