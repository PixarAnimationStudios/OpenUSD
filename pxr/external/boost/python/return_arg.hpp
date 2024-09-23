//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams and Nikolay Mladenov 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_RETURN_ARG_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_RETURN_ARG_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/return_arg.hpp>
#else
# include "pxr/external/boost/python/default_call_policies.hpp"
# include "pxr/external/boost/python/detail/none.hpp"
# include "pxr/external/boost/python/detail/value_arg.hpp"

#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
# include "pxr/external/boost/python/converter/pytype_function.hpp"
#endif

# include "pxr/external/boost/python/detail/type_traits.hpp"

# include <boost/mpl/int.hpp>
# include <boost/mpl/at.hpp>

# include "pxr/external/boost/python/refcount.hpp"

# include <cstddef>

namespace PXR_BOOST_NAMESPACE { namespace python { 

namespace detail
{
  template <std::size_t>
  struct return_arg_pos_argument_must_be_positive
# if defined(__GNUC__) || defined(__EDG__)
  {}
# endif
  ;

  struct return_none
  {
      template <class T> struct apply
      {
          struct type
          {
              static bool convertible()
              {
                  return true;
              }
              
              PyObject *operator()( typename value_arg<T>::type ) const
              {
                  return none();
              }
#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
              PyTypeObject const *get_pytype() const { return converter::expected_pytype_for_arg<T>::get_pytype() ; }
#endif
          };
      };
  };
}
    
template <
    std::size_t arg_pos=1
  , class Base = default_call_policies
> 
struct return_arg : Base
{
 private:
    BOOST_STATIC_CONSTANT(bool, legal = arg_pos > 0);

 public:
    typedef typename detail::mpl2::if_c<
        legal
        , detail::return_none
        , detail::return_arg_pos_argument_must_be_positive<arg_pos>
        // we could default to the base result_converter in case or
        // arg_pos==0 since return arg 0 means return result, but I
        // think it is better to issue an error instead, cause it can
        // lead to confusions
    >::type result_converter;

    template <class ArgumentPackage>
    static PyObject* postcall(ArgumentPackage const& args, PyObject* result)
    {
        // In case of arg_pos == 0 we could simply return Base::postcall,
        // but this is redundant
        static_assert(arg_pos > 0);

        result = Base::postcall(args,result);
        if (!result)
            return 0;
        Py_DECREF(result);
        return incref( detail::get(mpl::int_<arg_pos-1>(),args) );
    }

    template <class Sig> 
    struct extract_return_type : mpl::at_c<Sig, arg_pos>
    {
    };

};

template <
    class Base = default_call_policies
    >
struct return_self 
  : return_arg<1,Base>
{};

}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_RETURN_ARG_HPP
