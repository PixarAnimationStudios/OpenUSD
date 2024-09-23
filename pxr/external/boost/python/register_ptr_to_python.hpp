//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_REGISTER_PTR_TO_PYTHON_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_REGISTER_PTR_TO_PYTHON_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/register_ptr_to_python.hpp>
#else

#include "pxr/external/boost/python/pointee.hpp"
#include "pxr/external/boost/python/object.hpp"
#include "pxr/external/boost/python/object/class_wrapper.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python {
    
template <class P>
void register_ptr_to_python()
{
    typedef typename PXR_BOOST_NAMESPACE::python::pointee<P>::type X;
    objects::class_value_wrapper<
        P
      , objects::make_ptr_instance<
            X
          , objects::pointer_holder<P,X>
        >
    >();
}           

}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_REGISTER_PTR_TO_PYTHON_HPP


