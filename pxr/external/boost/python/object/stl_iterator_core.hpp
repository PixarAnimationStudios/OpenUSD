//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright Eric Niebler 2005.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_OBJECT_STL_ITERATOR_CORE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_OBJECT_STL_ITERATOR_CORE_HPP

# include "pxr/external/boost/python/object_fwd.hpp"
# include "pxr/external/boost/python/handle_fwd.hpp"

namespace boost { namespace python { namespace objects {

struct BOOST_PYTHON_DECL stl_input_iterator_impl
{
    stl_input_iterator_impl();
    stl_input_iterator_impl(boost::python::object const &ob);
    void increment();
    bool equal(stl_input_iterator_impl const &that) const;
    boost::python::handle<> const &current() const;
private:
    boost::python::object it_;
    boost::python::handle<> ob_;
};

}}} // namespace boost::python::object

#endif // PXR_EXTERNAL_BOOST_PYTHON_OBJECT_STL_ITERATOR_CORE_HPP
