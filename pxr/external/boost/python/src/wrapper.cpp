//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "pxr/external/boost/python/wrapper.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python {

namespace detail
{
  override wrapper_base::get_override(
      char const* name
    , PyTypeObject* class_object
  ) const
  {
      if (this->m_self)
      {
          if (handle<> m = handle<>(
                  python::allow_null(
                      ::PyObject_GetAttrString(
                          this->m_self, const_cast<char*>(name))))
          )
          {
              PyObject* borrowed_f = 0;
            
              if (
                  PyMethod_Check(m.get())
                  && PyMethod_GET_SELF(m.get()) == this->m_self
                  && class_object->tp_dict != 0
              )
              {
                  borrowed_f = ::PyDict_GetItemString(
                      class_object->tp_dict, const_cast<char*>(name));


              }
              if (borrowed_f != PyMethod_GET_FUNCTION(m.get()))
                  return override(m);
          }
      }
      return override(handle<>(detail::none()));
  }
}

#if 0
namespace converter
{
  PyObject* PXR_BOOST_PYTHON_DECL do_polymorphic_ref_to_python(
      python::detail::wrapper_base const volatile* x, type_info src
  )
  {
      if (x == 0)
      {
          ::PyErr_Format(
              PyExc_TypeError
            , "Attempting to returning pointer or reference to instance of %s\n"
              "for which no corresponding Python object exists. Wrap this function"
              "with a return return value policy"
          )
      }
  }
  
}
#endif 

}} // namespace PXR_BOOST_NAMESPACE::python::detail
