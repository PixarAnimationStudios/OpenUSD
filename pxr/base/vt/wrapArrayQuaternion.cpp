//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#define ADDITION_OPERATOR
#define SUBTRACTION_OPERATOR
#define MULTIPLICATION_OPERATOR
#define DOUBLE_MULT_OPERATOR
#define DOUBLE_DIV_OPERATOR

#include "pxr/pxr.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/wrapArray.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"

PXR_NAMESPACE_USING_DIRECTIVE

void wrapArrayQuaternion() {
    TF_PP_SEQ_FOR_EACH(VT_WRAP_ARRAY, ~, VT_QUATERNION_VALUE_TYPES);
}
