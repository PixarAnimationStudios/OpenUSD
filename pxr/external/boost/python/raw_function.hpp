//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_RAW_FUNCTION_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_RAW_FUNCTION_HPP

# include "pxr/external/boost/python/detail/prefix.hpp"

# include "pxr/external/boost/python/tuple.hpp"
# include "pxr/external/boost/python/dict.hpp"
# include "pxr/external/boost/python/object/py_function.hpp"
# include <boost/mpl/vector/vector10.hpp>

# include <boost/limits.hpp>
# include <cstddef>

namespace boost { namespace python { 

namespace detail
{
  template <class F>
  struct raw_dispatcher
  {
      raw_dispatcher(F f) : f(f) {}
      
      PyObject* operator()(PyObject* args, PyObject* keywords)
      {
          return incref(
              object(
                  f(
                      tuple(borrowed_reference(args))
                    , keywords ? dict(borrowed_reference(keywords)) : dict()
                  )
              ).ptr()
          );
      }

   private:
      F f;
  };

  object BOOST_PYTHON_DECL make_raw_function(objects::py_function);
}

template <class F>
object raw_function(F f, std::size_t min_args = 0)
{
    return detail::make_raw_function(
        objects::py_function(
            detail::raw_dispatcher<F>(f)
          , mpl::vector1<PyObject*>()
          , min_args
          , (std::numeric_limits<unsigned>::max)()
        )
    );
}
    
}} // namespace boost::python

#endif // PXR_EXTERNAL_BOOST_PYTHON_RAW_FUNCTION_HPP
