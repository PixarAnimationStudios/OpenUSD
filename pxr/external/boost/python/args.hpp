//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_ARGS_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_ARGS_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/args.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"

# include "pxr/external/boost/python/args_fwd.hpp"
# include "pxr/external/boost/python/detail/preprocessor.hpp"
# include "pxr/external/boost/python/detail/type_list.hpp"
# include "pxr/external/boost/python/detail/type_traits.hpp"

# include "pxr/external/boost/python/object_core.hpp"

# include "pxr/external/boost/python/detail/mpl2/bool.hpp"

# include <cstddef>
# include <algorithm>

namespace PXR_BOOST_NAMESPACE { namespace python {

typedef detail::keywords<1> arg;
typedef arg arg_; // gcc 2.96 workaround

namespace detail
{
  template <std::size_t nkeywords>
  struct keywords_base
  {
      static constexpr std::size_t size = nkeywords;
      
      keyword_range range() const
      {
          return keyword_range(elements, elements + nkeywords);
      }

      keyword elements[nkeywords];

      keywords<nkeywords+1>
      operator,(python::arg const &k) const;

      keywords<nkeywords + 1>
      operator,(char const *name) const;

      template <class... Name>
      void set_elements(Name... name)
      {
          static_assert(sizeof...(Name) == nkeywords, "Must supply all keywords");

          size_t i = 0;
          ([this, &i](char const* n) { elements[i++].name = n; }(name), ...);
      }
  };
  
  template <std::size_t nkeywords>
  struct keywords : keywords_base<nkeywords>
  {
  };

  template <>
  struct keywords<1> : keywords_base<1>
  {
      explicit keywords(char const *name)
      {
          elements[0].name = name;
      }
    
      template <class T>
      python::arg& operator=(T const& value)
      {
          object z(value);
          elements[0].default_value = handle<>(python::borrowed(object(value).ptr()));
          return *this;
      }
    
      operator detail::keyword const&() const
      {
          return elements[0];
      }
  };

  template <std::size_t nkeywords>
  inline
  keywords<nkeywords+1>
  keywords_base<nkeywords>::operator,(python::arg const &k) const
  {
      keywords<nkeywords> const& l = *static_cast<keywords<nkeywords> const*>(this);
      python::detail::keywords<nkeywords+1> res;
      std::copy(l.elements, l.elements+nkeywords, res.elements);
      res.elements[nkeywords] = k.elements[0];
      return res;
  }

  template <std::size_t nkeywords>
  inline
  keywords<nkeywords + 1>
  keywords_base<nkeywords>::operator,(char const *name) const
  {
      return this->operator,(python::arg(name));
  }

  template<typename T>
  struct is_keywords
  {
      static constexpr bool value = false; 
  };

  template<std::size_t nkeywords>
  struct is_keywords<keywords<nkeywords> >
  {
      static constexpr bool value = true;
  };
  template <class T>
  struct is_reference_to_keywords
  {
      static constexpr bool is_ref = detail::is_reference<T>::value;
      typedef typename detail::remove_reference<T>::type deref;
      typedef typename detail::remove_cv<deref>::type key_t;
      static constexpr bool is_key = is_keywords<key_t>::value;
      static constexpr bool value = (is_ref & is_key);
      
      typedef detail::mpl2::bool_<value> type;
  };
}

inline detail::keywords<1> args(char const* name)
{ 
    return detail::keywords<1>(name);
}

template <class... Name>
inline detail::keywords<sizeof...(Name)> args(Name... name)
{
    static_assert(
        (std::is_convertible_v<Name, char const*> && ...),
        "Arguments must be char const*");

    detail::keywords<sizeof...(Name)> result;
    result.set_elements(name...);
    return result;
}

}} // namespace PXR_BOOST_NAMESPACE::python


#endif // PXR_USE_INTERNAL_BOOST_PYTHON
# endif // PXR_EXTERNAL_BOOST_PYTHON_ARGS_HPP
