//
// Copyright 2026 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Test for BOOST_PYTHON_MODULE with optional mod_gil_not_used argument

#include "pxr/external/boost/python/module.hpp"
#include "pxr/external/boost/python/def.hpp"

// Simple function to export
int get_value() {
    return 1234;
}

#if defined(PXR_BOOST_PYTHON_HAS_CXX11) && (PY_VERSION_HEX >= 0x03000000)
// C++11 build with Python 3: test with mod_gil_not_used option
PXR_BOOST_PYTHON_MODULE(module_nogil_ext, PXR_BOOST_NAMESPACE::python::mod_gil_not_used())
{
    using namespace PXR_BOOST_NAMESPACE::python;
    def("get_value", get_value);
}
#else
// C++98 build or Python 2: test without optional arguments
PXR_BOOST_PYTHON_MODULE(module_nogil_ext)
{
    using namespace PXR_BOOST_NAMESPACE::python;
    def("get_value", get_value);
}
#endif
