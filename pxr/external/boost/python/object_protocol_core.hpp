//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_OBJECT_PROTOCOL_CORE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_OBJECT_PROTOCOL_CORE_HPP

# include "pxr/external/boost/python/detail/prefix.hpp"

# include "pxr/external/boost/python/handle_fwd.hpp"

namespace boost { namespace python { 

namespace api
{
  class object;

  BOOST_PYTHON_DECL object getattr(object const& target, object const& key);
  BOOST_PYTHON_DECL object getattr(object const& target, object const& key, object const& default_);
  BOOST_PYTHON_DECL void setattr(object const& target, object const& key, object const& value);
  BOOST_PYTHON_DECL void delattr(object const& target, object const& key);

  // These are defined for efficiency, since attributes are commonly
  // accessed through literal strings.
  BOOST_PYTHON_DECL object getattr(object const& target, char const* key);
  BOOST_PYTHON_DECL object getattr(object const& target, char const* key, object const& default_);
  BOOST_PYTHON_DECL void setattr(object const& target, char const* key, object const& value);
  BOOST_PYTHON_DECL void delattr(object const& target, char const* key);
  
  BOOST_PYTHON_DECL object getitem(object const& target, object const& key);
  BOOST_PYTHON_DECL void setitem(object const& target, object const& key, object const& value);
  BOOST_PYTHON_DECL void delitem(object const& target, object const& key);

  BOOST_PYTHON_DECL object getslice(object const& target, handle<> const& begin, handle<> const& end);
  BOOST_PYTHON_DECL void setslice(object const& target, handle<> const& begin, handle<> const& end, object const& value);
  BOOST_PYTHON_DECL void delslice(object const& target, handle<> const& begin, handle<> const& end);
}

using api::getattr;
using api::setattr;
using api::delattr;

using api::getitem;
using api::setitem;
using api::delitem;

using api::getslice;
using api::setslice;
using api::delslice;

}} // namespace boost::python

#endif // PXR_EXTERNAL_BOOST_PYTHON_OBJECT_PROTOCOL_CORE_HPP
