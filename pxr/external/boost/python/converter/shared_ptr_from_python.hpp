//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Copyright Stefan Seefeld 2016.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_SHARED_PTR_FROM_PYTHON_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_SHARED_PTR_FROM_PYTHON_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/converter/shared_ptr_from_python.hpp>
#else

#include "pxr/external/boost/python/handle.hpp"
#include "pxr/external/boost/python/converter/shared_ptr_deleter.hpp"
#include "pxr/external/boost/python/converter/from_python.hpp"
#include "pxr/external/boost/python/converter/rvalue_from_python_data.hpp"
#include "pxr/external/boost/python/converter/registered.hpp"
#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
# include "pxr/external/boost/python/converter/pytype_function.hpp"
#endif
#ifdef PXR_BOOST_PYTHON_HAS_BOOST_SHARED_PTR
#include <boost/shared_ptr.hpp>
#endif
#include <memory>

namespace PXR_BOOST_NAMESPACE { namespace python { namespace converter { 

template <class T, template <typename> class SP>
struct shared_ptr_from_python
{
  shared_ptr_from_python()
  {
    converter::registry::insert(&convertible, &construct, type_id<SP<T> >()
#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
				, &converter::expected_from_python_type_direct<T>::get_pytype
#endif
				);
  }

 private:
  static void* convertible(PyObject* p)
  {
    if (p == Py_None)
      return p;
        
    return converter::get_lvalue_from_python(p, registered<T>::converters);
  }
    
  static void construct(PyObject* source, rvalue_from_python_stage1_data* data)
  {
    void* const storage = ((converter::rvalue_from_python_storage<SP<T> >*)data)->storage.bytes;
    // Deal with the "None" case.
    if (data->convertible == source)
      new (storage) SP<T>();
    else
    {
      void *const storage = ((converter::rvalue_from_python_storage<SP<T> >*)data)->storage.bytes;
      // Deal with the "None" case.
      if (data->convertible == source)
        new (storage) SP<T>();
      else
      {
        SP<void> hold_convertible_ref_count((void*)0, shared_ptr_deleter(handle<>(borrowed(source))) );
        // use aliasing constructor
        new (storage) SP<T>(hold_convertible_ref_count, static_cast<T*>(data->convertible));
      }
    }
    data->convertible = storage;
  }
};

}}} // namespace PXR_BOOST_NAMESPACE::python::converter

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif
