//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright Joel de Guzman 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "pxr/external/boost/python/suite/indexing/map_indexing_suite.hpp"
#include "pxr/external/boost/python/module.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/implicit.hpp"

using namespace PXR_BOOST_NAMESPACE::python;

struct A
{
  int value;
  A() : value(0){};
  A(int v) : value(v) {};
};

bool operator==(const A& v1, const A& v2)
{
  return (v1.value == v2.value);
}

struct B
{
  A a;
};

// Converter from A to python int
struct AToPython 
{
  static PyObject* convert(const A& s)
  {
    return PXR_BOOST_NAMESPACE::python::incref(PXR_BOOST_NAMESPACE::python::object((int)s.value).ptr());
  }
};

// Conversion from python int to A
struct AFromPython 
{
  AFromPython()
  {
    PXR_BOOST_NAMESPACE::python::converter::registry::push_back(
        &convertible,
        &construct,
        PXR_BOOST_NAMESPACE::python::type_id< A >());
  }

  static void* convertible(PyObject* obj_ptr)
  {
#if PY_VERSION_HEX >= 0x03000000
    if (!PyLong_Check(obj_ptr)) return 0;
#else
    if (!PyInt_Check(obj_ptr)) return 0;
#endif
    return obj_ptr;
  }

  static void construct(
      PyObject* obj_ptr,
      PXR_BOOST_NAMESPACE::python::converter::rvalue_from_python_stage1_data* data)
  {
    void* storage = (
        (PXR_BOOST_NAMESPACE::python::converter::rvalue_from_python_storage< A >*)
        data)-> storage.bytes;

#if PY_VERSION_HEX >= 0x03000000
    new (storage) A((int)PyLong_AsLong(obj_ptr));
#else
    new (storage) A((int)PyInt_AsLong(obj_ptr));
#endif
    data->convertible = storage;
  }
};

void a_map_indexing_suite()
{

  to_python_converter< A , AToPython >();
  AFromPython();

  class_< std::map<int, A> >("AMap")
    .def(map_indexing_suite<std::map<int, A>, true >())
    ;

  class_< B >("B")
    .add_property("a", make_getter(&B::a, return_value_policy<return_by_value>()),
        make_setter(&B::a, return_value_policy<return_by_value>()))
    ;
}


