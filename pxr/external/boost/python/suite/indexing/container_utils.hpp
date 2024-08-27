//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

//  (C) Copyright Joel de Guzman 2003.
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef PXR_EXTERNAL_BOOST_PYTHON_SUITE_INDEXING_CONTAINER_UTILS_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_SUITE_INDEXING_CONTAINER_UTILS_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/suite/indexing/container_utils.hpp>
#else

# include <utility>
# include <boost/foreach.hpp>
# include "pxr/external/boost/python/object.hpp"
# include "pxr/external/boost/python/handle.hpp"
# include "pxr/external/boost/python/extract.hpp"
# include "pxr/external/boost/python/stl_iterator.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace container_utils {
        
    template <typename Container>
    void
    extend_container(Container& container, object l)
    {
        typedef typename Container::value_type data_type;
        
        //  l must be iterable
        BOOST_FOREACH(object elem,
            std::make_pair(
              PXR_BOOST_NAMESPACE::python::stl_input_iterator<object>(l),
              PXR_BOOST_NAMESPACE::python::stl_input_iterator<object>()
              ))
        {
            extract<data_type const&> x(elem);
            //  try if elem is an exact data_type type
            if (x.check())
            {
                container.push_back(x());
            }
            else
            {
                //  try to convert elem to data_type type
                extract<data_type> x(elem);
                if (x.check())
                {
                    container.push_back(x());
                }
                else
                {
                    PyErr_SetString(PyExc_TypeError, "Incompatible Data Type");
                    throw_error_already_set();
                }
            }
        }          
    }

}}} // namespace PXR_BOOST_NAMESPACE::python::container_utils

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif
