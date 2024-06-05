//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include <boost/python/scope.hpp>

#include "pxr/pxr.h"
#include "pxr/base/gf/limits.h"

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapLimits()
{
    scope().attr("MIN_VECTOR_LENGTH") = GF_MIN_VECTOR_LENGTH;
    scope().attr("MIN_ORTHO_TOLERANCE") = GF_MIN_ORTHO_TOLERANCE;
}
