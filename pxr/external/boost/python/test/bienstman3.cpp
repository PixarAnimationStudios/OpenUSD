//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include "pxr/external/boost/python/module.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/class.hpp"

struct V 
{
 virtual ~V() {}; // silence compiler warningsa
 virtual void f() = 0;
};

struct B 
{
    B(const V&) {}    
};

PXR_BOOST_PYTHON_MODULE(bienstman3_ext)
{
  using namespace PXR_BOOST_NAMESPACE::python;

  class_<V, boost::noncopyable>("V", no_init);
  class_<B>("B", init<const V&>());

}
