//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

// Enable specific operators for GfTimeCode arrays.  NUMERIC_OPERATORS is
// intentionally NOT used because it would also enable array*array and %, which
// do not exist for GfTimeCode.
#define ADDITION_OPERATOR       // TimeCodeArray + TimeCodeArray -> TimeCodeArray
#define SUBTRACTION_OPERATOR    // TimeCodeArray - TimeCodeArray -> DurationArray
#define DIVISION_OPERATOR       // TimeCodeArray / TimeCodeArray -> DoubleArray
#define DOUBLE_MULT_OPERATOR    // TimeCodeArray * double, double * TimeCodeArray
#define DOUBLE_DIV_OPERATOR     // TimeCodeArray / double
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

void wrapArrayTimeCode() {
    // Wrap GfTimeCode arrays.  The standard operator toggles above handle all
    // homogeneous TimeCode ops and scalar TimeCode ops.  The explicit .def()
    // calls below add the cross-type ops with GfDuration and VtDurationArray.
    VtWrapArray<VtTimeCodeArray>()
        // TimeCodeArray op VtDurationArray (cross-type array-array)
        .def("__add__",
             +[](VtTimeCodeArray const &a, VtDurationArray const &b) {
                 return a + b;
             })
        .def("__sub__",
             +[](VtTimeCodeArray const &a, VtDurationArray const &b) {
                 return a - b;
             })
        // TimeCodeArray op GfDuration (cross-type scalar)
        .def("__add__",
             +[](VtTimeCodeArray const &a, GfDuration const &d) {
                 return a + d;
             })
        .def("__radd__",
             +[](VtTimeCodeArray const &a, GfDuration const &d) {
                 return d + a;
             })
        .def("__sub__",
             +[](VtTimeCodeArray const &a, GfDuration const &d) {
                 return a - d;
             })
        .def("__rsub__",
             +[](VtTimeCodeArray const &a, GfDuration const &d) {
                 return d - a;
             })
        ;

    VtWrapArrayEdit<VtArrayEdit<GfTimeCode>>();
}
