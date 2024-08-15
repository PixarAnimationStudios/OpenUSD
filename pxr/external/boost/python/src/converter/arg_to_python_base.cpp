//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "pxr/external/boost/python/converter/arg_to_python_base.hpp"
#include "pxr/external/boost/python/errors.hpp"
#include "pxr/external/boost/python/converter/registrations.hpp"
#include "pxr/external/boost/python/handle.hpp"
#include "pxr/external/boost/python/refcount.hpp"

namespace boost { namespace python { namespace converter { 

namespace detail
{
  arg_to_python_base::arg_to_python_base(
      void const volatile* source, registration const& converters)
# if !defined(BOOST_MSVC) || BOOST_MSVC <= 1300 || _MSC_FULL_VER > 13102179
      : handle<>
# else
      : m_ptr
# endif
         (converters.to_python(source))
  {
  }
}

}}} // namespace boost::python::converter
