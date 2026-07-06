//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#define NUMERIC_OPERATORS

#include "pxr/pxr.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/vt/wrapArray.h"
#include "pxr/base/vt/wrapArrayEdit.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"

PXR_NAMESPACE_USING_DIRECTIVE

void wrapArrayTimeCode() {
    TF_PP_SEQ_FOR_EACH(VT_WRAP_ARRAY, ~, VT_TIMECODE_VALUE_TYPES);
    TF_PP_SEQ_FOR_EACH(VT_WRAP_ARRAY_EDIT, ~, VT_TIMECODE_VALUE_TYPES);
}
