//
// Copyright 2026 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

// Many tests validate the docstrings that are automatically generated
// as part of the module. These tests typically assume that full docstrings
// are enabled; however if PXR_BOOST_PYTHON_NO_PY_SIGNATURES is defined
// the module will be built with simplified docstrings, causing those tests
// to fail.
//
// This header ensures that full docstrings are enabled for a module
// regardless of the above preprocessor define. It must be included before
// any other header.

// Undefine this so that full type information is included in docstrings.
// Otherwise all types in the docstring are just identified as "object".
#undef PXR_BOOST_PYTHON_NO_PY_SIGNATURES

#include "pxr/external/boost/python/docstring_options.hpp"

// Show all function signatures for modules built after this declaration.
// In particular, this enables Python signatures in docstrings even if
// pxr_boost::python itself was built with PXR_BOOST_PYTHON_NO_PY_SIGNATURES.
static const
PXR_BOOST_NAMESPACE::python::docstring_options DocOpts(/* showAll = */true);
