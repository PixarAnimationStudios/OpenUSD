//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file wrapPseudoRootSpec.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/pseudoRootSpec.h"
#include "pxr/usd/sdf/pySpec.h"

#include "pxr/external/boost/python.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void
wrapPseudoRootSpec()
{
    typedef SdfPseudoRootSpec This;

    class_<This, SdfHandle<This>, 
           bases<SdfPrimSpec>, noncopyable>
        ("PseudoRootSpec", no_init)
        .def(SdfPySpec())
        ;
}
