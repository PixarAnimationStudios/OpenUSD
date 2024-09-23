//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright Joel de Guzman 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// If PXR_BOOST_PYTHON_NO_PY_SIGNATURES was defined when building this module,
// boost::python will generate simplified docstrings that break the associated
// test unless we undefine it before including any headers.
#undef PXR_BOOST_PYTHON_NO_PY_SIGNATURES

#include "pxr/external/boost/python/module.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/docstring_options.hpp"
#include "pxr/external/boost/python/extract.hpp"
#include "pxr/external/boost/python/to_python_converter.hpp"
#include "pxr/external/boost/python/class.hpp"

using namespace PXR_BOOST_NAMESPACE::python;

struct A
{
};

struct B
{
  A a;
  B(const A& a_):a(a_){}
};

// Converter from A to python int
struct BToPython
#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
 : converter::to_python_target_type<A>  //inherits get_pytype 
#endif
{
  static PyObject* convert(const B& b)
  {
    return PXR_BOOST_NAMESPACE::python::incref(PXR_BOOST_NAMESPACE::python::object(b.a).ptr());
  }
};

// Conversion from python int to A
struct BFromPython 
{
  BFromPython()
  {
    PXR_BOOST_NAMESPACE::python::converter::registry::push_back(
        &convertible,
        &construct,
        PXR_BOOST_NAMESPACE::python::type_id< B >()
#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
        , &converter::expected_from_python_type<A>::get_pytype//convertible to A can be converted to B
#endif
        );
  }

  static void* convertible(PyObject* obj_ptr)
  {
      extract<const A&> ex(obj_ptr);
      if (!ex.check()) return 0;
      return obj_ptr;
  }

  static void construct(
      PyObject* obj_ptr,
      PXR_BOOST_NAMESPACE::python::converter::rvalue_from_python_stage1_data* data)
  {
    void* storage = (
        (PXR_BOOST_NAMESPACE::python::converter::rvalue_from_python_storage< B >*)data)-> storage.bytes;

    extract<const A&> ex(obj_ptr);
    new (storage) B(ex());
    data->convertible = storage;
  }
};


B func(const B& b) { return b ; }


PXR_BOOST_PYTHON_MODULE(pytype_function_ext)
{
  // Explicitly enable Python signatures in docstrings in case boost::python
  // was built with PXR_BOOST_PYTHON_NO_PY_SIGNATURES, which disables those
  // signatures by default.
  docstring_options doc_options;
  doc_options.enable_py_signatures();

  to_python_converter< B , BToPython,true >(); //has get_pytype
  BFromPython();

  class_<A>("A") ;

  def("func", &func);

}

#include "module_tail.cpp"
