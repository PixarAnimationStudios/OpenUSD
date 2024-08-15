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

namespace boost { namespace python {

class override;

namespace detail
{
  class wrapper_base;
  
  // The result of calling a method.
  class method_result
  {
   private:
      friend class boost::python::override;
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

#  if BOOST_WORKAROUND(_MSC_FULL_VER, BOOST_TESTED_AT(140050215))
      template <class T>
      operator T*()
      {
          converter::return_from_python<T*> converter;
          return converter(m_obj.release());
      }
#  endif 
      
#  if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1400)) || BOOST_WORKAROUND(BOOST_INTEL_WIN, >= 900)
      // No operator T&
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

# define BOOST_PYTHON_fast_arg_to_python_get(z, n, _)   \
    , converter::arg_to_python<A##n>(a##n).get()

# define BOOST_PP_ITERATION_PARAMS_1 (3, (1, BOOST_PYTHON_MAX_ARITY, "pxr/external/boost/python/override.hpp"))
# include BOOST_PP_ITERATE()

# undef BOOST_PYTHON_fast_arg_to_python_get
};

}} // namespace boost::python

#endif // PXR_EXTERNAL_BOOST_PYTHON_OVERRIDE_HPP

#else
# if !(BOOST_WORKAROUND(__MWERKS__, > 0x3100)                      \
        && BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3201)))
#  line BOOST_PP_LINE(__LINE__, override.hpp)
# endif 

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
          , const_cast<char*>("(" BOOST_PP_REPEAT_1ST(N, BOOST_PYTHON_FIXED, "O") ")")
            BOOST_PP_REPEAT_1ST(N, BOOST_PYTHON_fast_arg_to_python_get, nil)
        ));
    return x;
}

# undef N
#endif 
