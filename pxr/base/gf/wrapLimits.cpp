//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>

#include "pxr/pxr.h"
#include "pxr/base/gf/limits.h"

PXR_NAMESPACE_USING_DIRECTIVE

void wrapLimits(nanobind::module_ &m)
{
    m.attr("MIN_VECTOR_LENGTH") = GF_MIN_VECTOR_LENGTH;
    m.attr("MIN_ORTHO_TOLERANCE") = GF_MIN_ORTHO_TOLERANCE;
}
