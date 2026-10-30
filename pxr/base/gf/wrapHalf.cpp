//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/half.h"
#include "pxr/base/gf/pyBufferUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Simple test function that takes and returns a GfHalf.
static GfHalf _HalfRoundTrip(GfHalf in) { return in; }

} // anonymous namespace 

void wrapHalf(nanobind::module_ &m)
{
    m.def("_HalfRoundTrip", _HalfRoundTrip);
}
