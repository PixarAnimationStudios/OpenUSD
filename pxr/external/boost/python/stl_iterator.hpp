//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright Eric Niebler 2005.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_STL_ITERATOR_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_STL_ITERATOR_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/stl_iterator.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"

# include "pxr/external/boost/python/object/stl_iterator_core.hpp"

# include <boost/iterator/iterator_facade.hpp>

namespace PXR_BOOST_NAMESPACE { namespace python
{ 

// An STL input iterator over a python sequence
template<typename ValueT>
struct stl_input_iterator
  : boost::iterator_facade<
        stl_input_iterator<ValueT>
      , ValueT
      , std::input_iterator_tag
      , ValueT
    >
{
    stl_input_iterator()
      : impl_()
    {
    }

    // ob is the python sequence
    stl_input_iterator(PXR_BOOST_NAMESPACE::python::object const &ob)
      : impl_(ob)
    {
    }

private:
    friend class boost::iterator_core_access;

    void increment()
    {
        this->impl_.increment();
    }

    ValueT dereference() const
    {
        return extract<ValueT>(this->impl_.current().get())();
    }

    bool equal(stl_input_iterator<ValueT> const &that) const
    {
        return this->impl_.equal(that.impl_);
    }

    objects::stl_input_iterator_impl impl_;
};

}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_STL_ITERATOR_HPP
