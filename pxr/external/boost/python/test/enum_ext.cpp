//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include "pxr/external/boost/python/enum.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/module.hpp"
#include "pxr/external/boost/python/class.hpp"
#if BOOST_WORKAROUND(__MWERKS__, <= 0x2407)
#include "pxr/external/boost/python/detail/type_traits.hpp"
# include <boost/mpl/bool.hpp>
#endif 
using namespace PXR_BOOST_NAMESPACE::python;

enum color { red = 1, green = 2, blue = 4, blood = 1 };

#if BOOST_WORKAROUND(__MWERKS__, <= 0x2407)
namespace PXR_BOOST_NAMESPACE  // Pro7 has a hard time detecting enums
{
  template <> struct PXR_BOOST_NAMESPACE::python::detail::is_enum<color> : boost::mpl::true_ {};
}
#endif 

color identity_(color x) { return x; }

struct colorized {
    colorized() : x(red) {}
    color x;
};

PXR_BOOST_PYTHON_MODULE(enum_ext)
{
    enum_<color>("color")
        .value("red", red)
        .value("green", green)
        .value("blue", blue)
        .value("blood", blood)
        .export_values()
        ;
    
    def("identity", identity_);

#if BOOST_WORKAROUND(__MWERKS__, <=0x2407)
    color colorized::*px = &colorized::x;
    class_<colorized>("colorized")
        .def_readwrite("x", px)
        ;
#else 
    class_<colorized>("colorized")
        .def_readwrite("x", &colorized::x)
        ;
#endif 
}

#include "module_tail.cpp"
