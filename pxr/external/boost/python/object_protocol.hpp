//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_OBJECT_PROTOCOL_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_OBJECT_PROTOCOL_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/object_protocol.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"

# include "pxr/external/boost/python/object_protocol_core.hpp"
# include "pxr/external/boost/python/object_core.hpp"

# include <boost/detail/workaround.hpp>

namespace PXR_BOOST_NAMESPACE { namespace python { namespace api {

# if BOOST_WORKAROUND(__SUNPRO_CC, BOOST_TESTED_AT(0x590))
// attempt to use SFINAE to prevent functions accepting T const& from
// coming up as ambiguous with the one taking a char const* when a
// string literal is passed
#  define PXR_BOOST_PYTHON_NO_ARRAY_ARG(T)             , T (*)() = 0
# else 
#  define PXR_BOOST_PYTHON_NO_ARRAY_ARG(T) 
# endif

template <class Target, class Key>
object getattr(Target const& target, Key const& key PXR_BOOST_PYTHON_NO_ARRAY_ARG(Key))
{
    return getattr(object(target), object(key));
}

template <class Target, class Key, class Default>
object getattr(Target const& target, Key const& key, Default const& default_ PXR_BOOST_PYTHON_NO_ARRAY_ARG(Key))
{
    return getattr(object(target), object(key), object(default_));
}


template <class Key, class Value>
void setattr(object const& target, Key const& key, Value const& value PXR_BOOST_PYTHON_NO_ARRAY_ARG(Key))
{
    setattr(target, object(key), object(value));
}

template <class Key>
void delattr(object const& target, Key const& key PXR_BOOST_PYTHON_NO_ARRAY_ARG(Key))
{
    delattr(target, object(key));
}

template <class Target, class Key>
object getitem(Target const& target, Key const& key PXR_BOOST_PYTHON_NO_ARRAY_ARG(Key))
{
    return getitem(object(target), object(key));
}


template <class Key, class Value>
void setitem(object const& target, Key const& key, Value const& value PXR_BOOST_PYTHON_NO_ARRAY_ARG(Key))
{
    setitem(target, object(key), object(value));
}

template <class Key>
void delitem(object const& target, Key const& key PXR_BOOST_PYTHON_NO_ARRAY_ARG(Key))
{
    delitem(target, object(key));
}

template <class Target, class Begin, class End>
object getslice(Target const& target, Begin const& begin, End const& end)
{
    return getslice(object(target), object(begin), object(end));
}

template <class Begin, class End, class Value>
void setslice(object const& target, Begin const& begin, End const& end, Value const& value)
{
    setslice(target, object(begin), object(end), object(value));
}

template <class Begin, class End>
void delslice(object const& target, Begin const& begin, End const& end)
{
    delslice(target, object(begin), object(end));
}

}}} // namespace PXR_BOOST_NAMESPACE::python::api

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_OBJECT_PROTOCOL_HPP
