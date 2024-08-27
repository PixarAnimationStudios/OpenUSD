//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_OBJECT_INSTANCE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_OBJECT_INSTANCE_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/object/instance.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"
# include "pxr/external/boost/python/detail/type_traits.hpp"
# include <cstddef>

namespace PXR_BOOST_NAMESPACE { namespace python
{
  struct instance_holder;
}} // namespace PXR_BOOST_NAMESPACE::python

namespace PXR_BOOST_NAMESPACE { namespace python { namespace objects { 

// Each extension instance will be one of these
template <class Data = char>
struct instance
{
    PyObject_VAR_HEAD
    PyObject* dict;
    PyObject* weakrefs; 
    instance_holder* objects;

    typedef typename PXR_BOOST_NAMESPACE::python::detail::type_with_alignment<
        PXR_BOOST_NAMESPACE::python::detail::alignment_of<Data>::value
    >::type align_t;

    union
    {
        align_t align;
        char bytes[sizeof(Data)];
    } storage;
};

template <class Data>
struct additional_instance_size
{
    typedef instance<Data> instance_data;
    typedef instance<char> instance_char;
    BOOST_STATIC_CONSTANT(std::size_t,
                          value = sizeof(instance_data) -
                             PXR_BOOST_PYTHON_OFFSETOF(instance_char,storage) +
                             PXR_BOOST_NAMESPACE::python::detail::alignment_of<Data>::value);
};

}}} // namespace PXR_BOOST_NAMESPACE::python::object

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_OBJECT_INSTANCE_HPP
