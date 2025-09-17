//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
//  (C) Copyright David Abrahams 2000.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  The author gratefully acknowleges the support of Dragon Systems, Inc., in
//  producing this work.

#include "pxr/external/boost/python/scope.hpp"
#include "pxr/external/boost/python/object/add_to_namespace.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail {

namespace
{
    PyObject* init_module_in_scope(PyObject* m, void(*init_function)())
    {
        if (m != 0)
        {
            // Create the current module scope
            object m_obj(((borrowed_reference_t*)m));
            scope current_module(m_obj);

            if (handle_exception(init_function)) return NULL;
        }

        return m;
    }
}

PXR_BOOST_PYTHON_DECL void scope_setattr_doc(char const* name, object const& x, char const* doc)
{
    // Use function::add_to_namespace to achieve overloading if
    // appropriate.
    scope current;
    objects::add_to_namespace(current, name, x, doc);
}

#if PY_VERSION_HEX >= 0x03000000

PXR_BOOST_PYTHON_DECL PyObject* init_module(PyModuleDef& moduledef, void(*init_function)())
{
    return init_module_in_scope(
        PyModule_Create(&moduledef),
        init_function);
}

#else

namespace
{
    PyMethodDef initial_methods[] = { { 0, 0, 0, 0 } };
}

PXR_BOOST_PYTHON_DECL PyObject* init_module(char const* name, void(*init_function)())
{
    return init_module_in_scope(
        Py_InitModule(const_cast<char*>(name), initial_methods),
        init_function);
}

#endif

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

namespace PXR_BOOST_NAMESPACE { namespace python {

namespace detail
{
  PXR_BOOST_PYTHON_DECL PyObject* current_scope = 0;
}

}}
