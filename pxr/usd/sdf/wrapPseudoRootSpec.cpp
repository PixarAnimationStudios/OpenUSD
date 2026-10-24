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
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

void
wrapPseudoRootSpec(nb::module_& m)
{
    typedef SdfPseudoRootSpec This;

    nb::class_<This, SdfPrimSpec>(m, "PseudoRootSpec")
        .def(SdfPySpec())
        ;
}
