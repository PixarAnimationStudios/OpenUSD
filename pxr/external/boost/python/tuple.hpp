//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_TUPLE_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_TUPLE_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/tuple.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"

#include "pxr/external/boost/python/object.hpp"
#include "pxr/external/boost/python/converter/pytype_object_mgr_traits.hpp"
#include <boost/preprocessor/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>

// XXX: Workaround for distcc issues with BOOST_PP_ITERATE
# define PXR_BOOST_PYTHON_SYNOPSIS
# include "pxr/external/boost/python/detail/make_tuple.hpp"
# undef PXR_BOOST_PYTHON_SYNOPSIS

namespace PXR_BOOST_NAMESPACE { namespace python {

namespace detail
{
  struct PXR_BOOST_PYTHON_DECL tuple_base : object
  {
   protected:
      tuple_base();
      tuple_base(object_cref sequence);
      
      PXR_BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS(tuple_base, object)

   private:
      static detail::new_reference call(object const&);
  };
}

class tuple : public detail::tuple_base
{
    typedef detail::tuple_base base;
 public:
    tuple() {}

    template <class T>
    explicit tuple(T const& sequence)
        : base(object(sequence))
    {
    }

 public: // implementation detail -- for internal use only
    PXR_BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS(tuple, base)
};

//
// Converter Specializations    // $$$ JDG $$$ moved here to prevent
//                              // G++ bug complaining specialization
                                // provided after instantiation
namespace converter
{
  template <>
  struct object_manager_traits<tuple>
      : pytype_object_manager_traits<&PyTuple_Type,tuple>
  {
  };
}

// for completeness
inline tuple make_tuple() { return tuple(); }

# define BOOST_PP_ITERATION_PARAMS_1 (3, (1, PXR_BOOST_PYTHON_MAX_ARITY, "pxr/external/boost/python/detail/make_tuple.hpp"))
# include BOOST_PP_ITERATE()

}}  // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif

