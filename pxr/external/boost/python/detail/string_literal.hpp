//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_STRING_LITERAL_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_STRING_LITERAL_HPP

# include <cstddef>
# include <boost/type.hpp>
# include "pxr/external/boost/python/detail/type_traits.hpp"
# include <boost/mpl/bool.hpp>
# include <boost/detail/workaround.hpp>

namespace boost { namespace python { namespace detail { 

template <class T>
struct is_string_literal : mpl::false_
{
};

#  if !defined(__MWERKS__) || __MWERKS__ > 0x2407
template <std::size_t n>
struct is_string_literal<char const[n]> : mpl::true_
{
};

#   if BOOST_WORKAROUND(__DECCXX_VER, BOOST_TESTED_AT(60590040)) \
  || (defined(__sgi) && defined(_COMPILER_VERSION) && _COMPILER_VERSION <= 730)
// This compiler mistakenly gets the type of string literals as char*
// instead of char[NN].
template <>
struct is_string_literal<char* const> : mpl::true_
{
};
#   endif

#  else

// CWPro7 has trouble with the array type deduction above
template <class T, std::size_t n>
struct is_string_literal<T[n]>
    : is_same<T, char const>
{
};
#  endif 

}}} // namespace boost::python::detail

#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_STRING_LITERAL_HPP
