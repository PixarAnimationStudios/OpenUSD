//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright Stefan Seefeld 2005.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_IMPORT_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_IMPORT_HPP

# include "pxr/external/boost/python/object.hpp"
# include "pxr/external/boost/python/str.hpp"

namespace boost 
{ 
namespace python 
{

// Import the named module and return a reference to it.
object BOOST_PYTHON_DECL import(str name);

}
}

#endif
