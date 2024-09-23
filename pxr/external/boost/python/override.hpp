#if !defined(BOOST_PP_IS_ITERATING)

//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_OVERRIDE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_OVERRIDE_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/override.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"

# include "pxr/external/boost/python/converter/return_from_python.hpp"

# include "pxr/external/boost/python/extract.hpp"
# include "pxr/external/boost/python/handle.hpp"

#  include <boost/preprocessor/iterate.hpp>
#  include <boost/preprocessor/repeat.hpp>
#  include <boost/preprocessor/debug/line.hpp>
#  include <boost/preprocessor/repetition/enum_params.hpp>
#  include <boost/preprocessor/repetition/enum_binary_params.hpp>

#  include <boost/type.hpp>

namespace PXR_BOOST_NAMESPACE { namespace python {

class override;

namespace detail
{
  class wrapper_base;
  
  // The result of calling a method.
  class method_result
  {
   private:
      friend class PXR_BOOST_NAMESPACE::python::override;
      explicit method_result(PyObject* x)
        : m_obj(x)
      {}

   public:
      template <class T>
      operator T()
      {
          converter::return_from_python<T> converter;
          return converter(m_obj.release());
      }

#  if defined(BOOST_MSVC)
      // No operator T&
      // This workaround was noted as required for VC 8 (_MSC_VER == 1400).
      // It is still required as of Visual Studio 2019 (_MSC_VER == 1929).
      // Without it, the unit test "exec" fails.
#  else
      
      template <class T>
      operator T&() const
      {
          converter::return_from_python<T&> converter;
          return converter(const_cast<handle<>&>(m_obj).release());
      }
#  endif 

      template <class T>
      T as(type<T>* = 0)
      {
          converter::return_from_python<T> converter;
          return converter(m_obj.release());
      }

      template <class T>
      T unchecked(type<T>* = 0)
      {
          return extract<T>(m_obj.get())();
      }
   private:
      mutable handle<> m_obj;
  };
}

class override : public object
{
 private:
    friend class detail::wrapper_base;
    override(handle<> x)
      : object(x)
    {}
    
 public:
    detail::method_result
    operator()() const
    {
        detail::method_result x(
            PyObject_CallFunction(
                this->ptr()
              , const_cast<char*>("()")
            ));
        return x;
    }

# define PXR_BOOST_PYTHON_fast_arg_to_python_get(z, n, _)   \
    , converter::arg_to_python<A##n>(a##n).get()

# define BOOST_PP_ITERATION_PARAMS_1 (3, (1, PXR_BOOST_PYTHON_MAX_ARITY, "pxr/external/boost/python/override.hpp"))
# include BOOST_PP_ITERATE()

# undef PXR_BOOST_PYTHON_fast_arg_to_python_get
};

}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_OVERRIDE_HPP

#else
#  line BOOST_PP_LINE(__LINE__, override.hpp)

# define N BOOST_PP_ITERATION()

template <
    BOOST_PP_ENUM_PARAMS_Z(1, N, class A)
    >
detail::method_result
operator()( BOOST_PP_ENUM_BINARY_PARAMS_Z(1, N, A, const& a) ) const
{
    detail::method_result x(
        PyObject_CallFunction(
            this->ptr()
          , const_cast<char*>("(" BOOST_PP_REPEAT_1ST(N, PXR_BOOST_PYTHON_FIXED, "O") ")")
            BOOST_PP_REPEAT_1ST(N, PXR_BOOST_PYTHON_fast_arg_to_python_get, nil)
        ));
    return x;
}

# undef N
#endif 
