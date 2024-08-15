//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_IMPLICIT_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_IMPLICIT_HPP

# include "pxr/external/boost/python/detail/prefix.hpp"
# include <boost/type.hpp>
# include "pxr/external/boost/python/converter/implicit.hpp"
# include "pxr/external/boost/python/converter/registry.hpp"
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
# include "pxr/external/boost/python/converter/pytype_function.hpp"
#endif
# include "pxr/external/boost/python/type_id.hpp"

namespace boost { namespace python { 

template <class Source, class Target>
void implicitly_convertible(boost::type<Source>* = 0, boost::type<Target>* = 0)
{
    typedef converter::implicit<Source,Target> functions;
    
    converter::registry::push_back(
          &functions::convertible
        , &functions::construct
        , type_id<Target>()
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
        , &converter::expected_from_python_type_direct<Source>::get_pytype
#endif
        );
}

}} // namespace boost::python

#endif // PXR_EXTERNAL_BOOST_PYTHON_IMPLICIT_HPP
