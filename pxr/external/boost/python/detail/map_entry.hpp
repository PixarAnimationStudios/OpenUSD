//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_MAP_ENTRY_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_MAP_ENTRY_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/map_entry.hpp>
#else

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { 

// A trivial type that works well as the value_type of associative
// vector maps
template <class Key, class Value>
struct map_entry
{
    map_entry() {}
    map_entry(Key k) : key(k), value() {}
    map_entry(Key k, Value v) : key(k), value(v) {}
    
    bool operator<(map_entry const& rhs) const
    {
        return this->key < rhs.key;
    }
        
    Key key;
    Value value;
};

template <class Key, class Value>
bool operator<(map_entry<Key,Value> const& e, Key const& k)
{
    return e.key < k;
}

template <class Key, class Value>
bool operator<(Key const& k, map_entry<Key,Value> const& e)
{
    return k < e.key;
}


}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_MAP_ENTRY_HPP
