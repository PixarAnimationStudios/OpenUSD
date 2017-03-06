//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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

PXR_NAMESPACE_OPEN_SCOPE

void wrapArrayVec() {
    BOOST_PP_SEQ_FOR_EACH(VT_WRAP_ARRAY, ~, VT_VEC_VALUE_TYPES);
    //BOOST_PP_SEQ_FOR_EACH(VT_WRAP_COMPARISON, ~, VT_VEC_VALUE_TYPES);
}

PXR_NAMESPACE_CLOSE_SCOPE
