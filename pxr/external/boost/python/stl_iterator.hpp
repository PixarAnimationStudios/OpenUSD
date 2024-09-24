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

namespace PXR_BOOST_NAMESPACE { namespace python
{ 

// An STL input iterator over a python sequence
template<typename ValueT>
struct stl_input_iterator
{
    using difference_type = std::ptrdiff_t;
    using value_type = ValueT;
    using pointer = ValueT*;
    using reference = ValueT;
    using iterator_category = std::input_iterator_tag;

    stl_input_iterator()
      : impl_()
    {
    }

    // ob is the python sequence
    stl_input_iterator(PXR_BOOST_NAMESPACE::python::object const &ob)
      : impl_(ob)
    {
    }

    stl_input_iterator& operator++()
    {
        increment();
        return *this;
    }

    stl_input_iterator operator++(int)
    {
        stl_input_iterator old(*this);
        increment();
        return old;
    }

    ValueT operator*() const
    {
        return dereference();
    }

    friend bool operator==(stl_input_iterator const& lhs, stl_input_iterator const& rhs)
    {
        return lhs.equal(rhs);
    }

    friend bool operator!=(stl_input_iterator const& lhs, stl_input_iterator const& rhs)
    {
        return !(lhs == rhs);
    }

private:
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
