//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/* Copyright 2004 Jonathan Brandmeyer 
 * Use, modification and distribution are subject to the
 * Boost Software License, Version 1.0. (See accompanying file
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 * The purpose of this test is to determine if a function can be called from
 * Python with a const value type as an argument, and whether or not the 
 * presence of a prototype without the cv-qualifier will work around the
 * compiler's bug.
 */
#include "pxr/external/boost/python.hpp"
using namespace PXR_BOOST_NAMESPACE::python;


bool accept_const_arg( const object )
{
    return true; 
}


PXR_BOOST_PYTHON_MODULE( const_argument_ext )
{
    def( "accept_const_arg", accept_const_arg );
}
