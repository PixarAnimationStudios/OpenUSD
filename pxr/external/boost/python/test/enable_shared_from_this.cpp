//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "pxr/external/boost/python/module.hpp"
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/call_method.hpp"
#include "pxr/external/boost/python/extract.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "test_class.hpp"

#include <memory>

using namespace PXR_BOOST_NAMESPACE::python;
using std::shared_ptr;

class Test;
typedef shared_ptr<Test> TestPtr;

class Test : public std::enable_shared_from_this<Test> {
public:
    static TestPtr construct() {
        return TestPtr(new Test);
    }

    void act() {
        TestPtr kungFuDeathGrip(shared_from_this());
    }

    void take(TestPtr t) {
    }
};

PXR_BOOST_PYTHON_MODULE(enable_shared_from_this_ext)
{
    class_<Test, TestPtr, noncopyable>("Test")
        .def("construct", &Test::construct).staticmethod("construct")
        .def("act", &Test::act)
        .def("take", &Test::take)
        ;
}

#include "module_tail.cpp"


