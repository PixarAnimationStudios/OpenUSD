//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

// Enable specific operators for GfDuration arrays.  NUMERIC_OPERATORS is
// intentionally NOT used because it would also enable array*array and %, which
// do not exist for GfDuration.
#define ADDITION_OPERATOR       // DurationArray + DurationArray -> DurationArray
#define SUBTRACTION_OPERATOR    // DurationArray - DurationArray -> DurationArray
#define DIVISION_OPERATOR       // DurationArray / DurationArray -> DoubleArray
#define DOUBLE_MULT_OPERATOR    // DurationArray * double, double * DurationArray
#define DOUBLE_DIV_OPERATOR     // DurationArray / double
#define UNARY_NEG_OPERATOR

#include "pxr/pxr.h"
#include "pxr/base/gf/duration.h"
#include "pxr/base/gf/timeCode.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/vt/wrapArray.h"
#include "pxr/base/vt/wrapArrayEdit.h"

PXR_NAMESPACE_USING_DIRECTIVE

void wrapArrayDuration() {
    // Wrap GfDuration arrays.  The standard operator toggles above handle all
    // homogeneous Duration ops and scalar Duration ops.  The explicit .def()
    // calls below add the cross-type ops with GfTimeCode and VtTimeCodeArray.
    VtWrapArray<VtDurationArray>()
        // DurationArray op VtTimeCodeArray (cross-type array-array)
        .def("__add__",
             +[](VtDurationArray const &a, VtTimeCodeArray const &b) {
                 return a + b;
             })
        .def("__sub__",
             +[](VtDurationArray const &a, VtTimeCodeArray const &b) {
                 return a - b;
             })
        // DurationArray op GfTimeCode (cross-type scalar)
        .def("__add__",
             +[](VtDurationArray const &a, GfTimeCode const &tc) {
                 return a + tc;
             })
        .def("__radd__",
             +[](VtDurationArray const &a, GfTimeCode const &tc) {
                 return tc + a;
             })
        .def("__sub__",
             +[](VtDurationArray const &a, GfTimeCode const &tc) {
                 return a - tc;
             })
        .def("__rsub__",
             +[](VtDurationArray const &a, GfTimeCode const &tc) {
                 return tc - a;
             })
        ;

    VtWrapArrayEdit<VtArrayEdit<GfDuration>>();
}
