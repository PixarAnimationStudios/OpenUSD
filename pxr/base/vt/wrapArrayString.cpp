//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#define ADDITION_OPERATOR

#include "pxr/pxr.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/wrapArray.h"

#include <string>
using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapArrayString() {
    VtWrapArray<VtArray<string> >();
}
