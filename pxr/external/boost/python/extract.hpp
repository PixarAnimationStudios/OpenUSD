//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_EXTRACT_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_EXTRACT_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/extract.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"

# include "pxr/external/boost/python/converter/object_manager.hpp"
# include "pxr/external/boost/python/converter/from_python.hpp"
# include "pxr/external/boost/python/converter/rvalue_from_python_data.hpp"
# include "pxr/external/boost/python/converter/registered.hpp"
# include "pxr/external/boost/python/converter/registered_pointee.hpp"

# include "pxr/external/boost/python/object_core.hpp"
# include "pxr/external/boost/python/refcount.hpp"

# include "pxr/external/boost/python/detail/copy_ctor_mutates_rhs.hpp"
# include "pxr/external/boost/python/detail/void_ptr.hpp"
# include "pxr/external/boost/python/detail/void_return.hpp"
# include "pxr/external/boost/python/detail/type_traits.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python {

namespace api
{
  class object;
}

namespace converter
{
  template <class Ptr>
  struct extract_pointer
  {
      typedef Ptr result_type;
      extract_pointer(PyObject*);
      
      bool check() const;
      Ptr operator()() const;
      
   private:
      PyObject* m_source;
      void* m_result;
  };
  
  template <class Ref>
  struct extract_reference
  {
      typedef Ref result_type;
      extract_reference(PyObject*);
      
      bool check() const;
      Ref operator()() const;
      
   private:
      PyObject* m_source;
      void* m_result;
  };
  
  template <class T>
  struct extract_rvalue
  {
      typedef typename python::detail::mpl2::if_<
          python::detail::copy_ctor_mutates_rhs<T>
        , T&
        , typename python::detail::param_type<T>::type
      >::type result_type;

      extract_rvalue(PyObject*);

      extract_rvalue(extract_rvalue const&) = delete;
      extract_rvalue& operator=(extract_rvalue const&) = delete;

      bool check() const;
      result_type operator()() const;
   private:
      PyObject* m_source;
      mutable rvalue_from_python_data<T> m_data;
  };
  
  template <class T>
  struct extract_object_manager
  {
      typedef T result_type;
      extract_object_manager(PyObject*);

      bool check() const;
      result_type operator()() const;
   private:
      PyObject* m_source;
  };
  
  template <class T>
  struct select_extract
  {
      static constexpr 
          bool obj_mgr = is_object_manager<T>::value;

      static constexpr 
          bool ptr = python::detail::is_pointer<T>::value;
    
      static constexpr 
          bool ref = python::detail::is_reference<T>::value;

      typedef typename python::detail::mpl2::if_c<
          obj_mgr
          , extract_object_manager<T>
          , typename python::detail::mpl2::if_c<
              ptr
              , extract_pointer<T>
              , typename python::detail::mpl2::if_c<
                  ref
                  , extract_reference<T>
                  , extract_rvalue<T>
                >::type
            >::type
         >::type type;
  };
}

template <class T>
struct extract
    : converter::select_extract<T>::type
{
 private:
    typedef typename converter::select_extract<T>::type base;
 public:
    typedef typename base::result_type result_type;
    
    operator result_type() const
    {
        return (*this)();
    }
    
    extract(PyObject*);
    extract(api::object const&);
};

//
// Implementations
//
template <class T>
inline extract<T>::extract(PyObject* o)
    : base(o)
{
}

template <class T>
inline extract<T>::extract(api::object const& o)
    : base(o.ptr())
{
}

namespace converter
{
  template <class T>
  inline extract_rvalue<T>::extract_rvalue(PyObject* x)
      : m_source(x)
      , m_data(
          (rvalue_from_python_stage1)(x, registered<T>::converters)
          )
  {
  }
  
  template <class T>
  inline bool
  extract_rvalue<T>::check() const
  {
      return m_data.stage1.convertible;
  }

  template <class T>
  inline typename extract_rvalue<T>::result_type
  extract_rvalue<T>::operator()() const
  {
      return *(T*)(
          // Only do the stage2 conversion once
          m_data.stage1.convertible ==  m_data.storage.bytes
             ? m_data.storage.bytes
             : (rvalue_from_python_stage2)(m_source, m_data.stage1, registered<T>::converters)
          );
  }

  template <class Ref>
  inline extract_reference<Ref>::extract_reference(PyObject* obj)
      : m_source(obj)
      , m_result(
          (get_lvalue_from_python)(obj, registered<Ref>::converters)
          )
  {
  }

  template <class Ref>
  inline bool extract_reference<Ref>::check() const
  {
      return m_result != 0;
  }

  template <class Ref>
  inline Ref extract_reference<Ref>::operator()() const
  {
      if (m_result == 0)
          (throw_no_reference_from_python)(m_source, registered<Ref>::converters);
      
      return python::detail::void_ptr_to_reference(m_result, (Ref(*)())0);
  }

  template <class Ptr>
  inline extract_pointer<Ptr>::extract_pointer(PyObject* obj)
      : m_source(obj)
      , m_result(
          obj == Py_None ? 0 : (get_lvalue_from_python)(obj, registered_pointee<Ptr>::converters)
          )
  {
  }

  template <class Ptr>
  inline bool extract_pointer<Ptr>::check() const
  {
      return m_source == Py_None || m_result != 0;
  }

  template <class Ptr>
  inline Ptr extract_pointer<Ptr>::operator()() const
  {
      if (m_result == 0 && m_source != Py_None)
          (throw_no_pointer_from_python)(m_source, registered_pointee<Ptr>::converters);
      
      return Ptr(m_result);
  }

  template <class T>
  inline extract_object_manager<T>::extract_object_manager(PyObject* obj)
      : m_source(obj)
  {
  }

  template <class T>
  inline bool extract_object_manager<T>::check() const
  {
      return object_manager_traits<T>::check(m_source);
  }

  template <class T>
  inline T extract_object_manager<T>::operator()() const
  {
      return T(
          object_manager_traits<T>::adopt(python::incref(m_source))
          );
  }
}
  
}} // namespace PXR_BOOST_NAMESPACE::python::converter

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_EXTRACT_HPP
