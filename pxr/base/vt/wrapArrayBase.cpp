//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/vt/wrapArray.h"

PXR_NAMESPACE_USING_DIRECTIVE

#include "pxr/base/tf/pyNanobindBridge.h"

void wrapArray(nanobind::module_&)
{
    // The actual wrapping of particular template instantiations is done in the
    // specific wrapArrayXXX.cpp files to avoid quadratic compiler behavior.
}
