//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#define ADDITION_OPERATOR
#define SUBTRACTION_OPERATOR
#define UNARY_NEG_OPERATOR
#define DOUBLE_MULT_OPERATOR

// Vec types support *, but as a dot product, so return is a double rather than
// a Vec, so we can't use it on two Vecs, just on Vec * double
// (sure we could create special overloading for
// VtArray<double> operator* (VtArray<GfVecX>, VtArray<GfVecX>)
// and the corresponding scalar versions:
// VtArray<double> operator* (GfVecX, VtArray<GfVecX>) etc
// and the corresponding python versions for tuples and lists,
// but let's hold off on that for now)
//
// Vecs also don't generally support division.  As a special case, the non-int
// vec types support division by a double, but since it's not all Vecs we can't
// define Vec / double.
#include "pxr/pxr.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/wrapArray.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"

PXR_NAMESPACE_USING_DIRECTIVE

void wrapArrayVec() {
    TF_PP_SEQ_FOR_EACH(VT_WRAP_ARRAY, ~, VT_VEC_VALUE_TYPES);
    //BOOST_PP_SEQ_FOR_EACH(VT_WRAP_COMPARISON, ~, VT_VEC_VALUE_TYPES);
}
