//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/types.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/tf/preprocessorUtilsLite.h"
#include "pxr/base/tf/type.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

// The following preprocessor code generates specializations for free functions
// that produce
// "zero" values for various scalar types held in arrays.  These can be used
// to generically test a value for "zero", for initialization, etc.
//
// The resulting templated functions appear as follows:
// VtZero<double>()
// VtZero<GfVec3d>()
// etc.
#define VT_ZERO_0_CONSTRUCTOR(unused, elem)         \
template<>                                          \
VT_API VT_TYPE(elem) VtZero() {                     \
    return (VT_TYPE(elem))(0);                      \
}
#define VT_ZERO_0FLOAT_CONSTRUCTOR(unused, elem)    \
template<>                                          \
VT_API VT_TYPE(elem) VtZero() {                     \
    return VT_TYPE(elem)(0.0f);                     \
}
#define VT_ZERO_0DOUBLE_CONSTRUCTOR(unused, elem)   \
template<>                                          \
VT_API VT_TYPE(elem) VtZero() {                     \
    return VT_TYPE(elem)(0.0);                      \
}
#define VT_ZERO_EMPTY_CONSTRUCTOR(unused, elem)     \
template<>                                          \
VT_API VT_TYPE(elem) VtZero() {                     \
    return VT_TYPE(elem)() ;                        \
}

TF_PP_SEQ_FOR_EACH(VT_ZERO_0_CONSTRUCTOR , ~,       \
    VT_BUILTIN_NUMERIC_VALUE_TYPES                  \
    VT_VEC_VALUE_TYPES                              \
    VT_QUATERNION_VALUE_TYPES                       \
    VT_DUALQUATERNION_VALUE_TYPES)
TF_PP_SEQ_FOR_EACH(VT_ZERO_0FLOAT_CONSTRUCTOR , ~,  \
    VT_MATRIX_FLOAT_VALUE_TYPES)
TF_PP_SEQ_FOR_EACH(VT_ZERO_0DOUBLE_CONSTRUCTOR , ~, \
    VT_MATRIX_DOUBLE_VALUE_TYPES)
TF_PP_SEQ_FOR_EACH(VT_ZERO_EMPTY_CONSTRUCTOR, ~,    \
    VT_RANGE_VALUE_TYPES                            \
    VT_STRING_VALUE_TYPES                           \
    VT_NONARRAY_VALUE_TYPES)


TF_REGISTRY_FUNCTION(TfType)
{
    // The following preprocessor code instantiates TfTypes for VtArray holding
    // various scalar value types.

#   define _INSTANTIATE_ARRAY(unused, elem) \
        TfType::Define< VtArray<VT_TYPE(elem)> >();

    TF_PP_SEQ_FOR_EACH(_INSTANTIATE_ARRAY, ~, VT_SCALAR_VALUE_TYPES)
}

// Floating point conversions... in future, we might hope to use SSE here.
// Where is the right place to document the existence of these?
namespace {

// A function object that converts a 'From' to a 'To'.
template <class To>
struct _Convert {
    template <class From>
    inline To operator()(From const &from) const { return To(from); }
};

// A function object that converts a GfRange type to another GfRange type.
template <class ToRng>
struct _ConvertRng {
    template <class FromRng>
    inline ToRng operator()(FromRng const &from) const {
        return ToRng(typename ToRng::MinMaxType(from.GetMin()),
                     typename ToRng::MinMaxType(from.GetMax()));
    }
};

template <class FromArray, class ToArray, template <class> class Convert>
VtValue _ConvertArray(VtValue const &array) {
    const FromArray &src = array.Get<FromArray>();
    ToArray dst(src.size());
    std::transform(src.begin(), src.end(), dst.begin(),
                   Convert<typename ToArray::ElementType>());
    return VtValue::Take(dst);
}

template <class A1, class A2>
void _RegisterArrayCasts() {
    VtValue::RegisterCast<A1, A2>(_ConvertArray<A1, A2, _Convert>);
    VtValue::RegisterCast<A2, A1>(_ConvertArray<A2, A1, _Convert>);
}

template <class A1, class A2>
void _RegisterRangeArrayCasts() {
    VtValue::RegisterCast<A1, A2>(_ConvertArray<A1, A2, _ConvertRng>);
    VtValue::RegisterCast<A2, A1>(_ConvertArray<A2, A1, _ConvertRng>);
}

} // anon

TF_REGISTRY_FUNCTION(VtValue)
{
    
    VtValue::RegisterSimpleCast<GfVec2i, GfVec2h>();
    VtValue::RegisterSimpleCast<GfVec2i, GfVec2f>();
    VtValue::RegisterSimpleCast<GfVec2i, GfVec2d>();
    VtValue::RegisterSimpleBidirectionalCast<GfVec2h, GfVec2d>();
    VtValue::RegisterSimpleBidirectionalCast<GfVec2h, GfVec2f>();
    VtValue::RegisterSimpleBidirectionalCast<GfVec2f, GfVec2d>();

    VtValue::RegisterSimpleCast<GfVec3i, GfVec3h>();
    VtValue::RegisterSimpleCast<GfVec3i, GfVec3f>();
    VtValue::RegisterSimpleCast<GfVec3i, GfVec3d>();
    VtValue::RegisterSimpleBidirectionalCast<GfVec3h, GfVec3d>();
    VtValue::RegisterSimpleBidirectionalCast<GfVec3h, GfVec3f>();
    VtValue::RegisterSimpleBidirectionalCast<GfVec3f, GfVec3d>();

    VtValue::RegisterSimpleCast<GfVec4i, GfVec4h>();
    VtValue::RegisterSimpleCast<GfVec4i, GfVec4f>();
    VtValue::RegisterSimpleCast<GfVec4i, GfVec4d>();
    VtValue::RegisterSimpleBidirectionalCast<GfVec4h, GfVec4d>();
    VtValue::RegisterSimpleBidirectionalCast<GfVec4h, GfVec4f>();
    VtValue::RegisterSimpleBidirectionalCast<GfVec4f, GfVec4d>();

    // Precision casts.
    _RegisterArrayCasts<VtHalfArray, VtFloatArray>();
    _RegisterArrayCasts<VtHalfArray, VtDoubleArray>();
    _RegisterArrayCasts<VtFloatArray, VtDoubleArray>();
    _RegisterArrayCasts<VtVec2hArray, VtVec2fArray>();
    _RegisterArrayCasts<VtVec2hArray, VtVec2dArray>();
    _RegisterArrayCasts<VtVec2fArray, VtVec2dArray>();
    _RegisterArrayCasts<VtVec3hArray, VtVec3fArray>();
    _RegisterArrayCasts<VtVec3hArray, VtVec3dArray>();
    _RegisterArrayCasts<VtVec3fArray, VtVec3dArray>();
    _RegisterArrayCasts<VtVec4hArray, VtVec4fArray>();
    _RegisterArrayCasts<VtVec4hArray, VtVec4dArray>();
    _RegisterArrayCasts<VtVec4fArray, VtVec4dArray>();

    // Not sure how necessary these are; here for consistency
    _RegisterRangeArrayCasts<VtRange1fArray, VtRange1dArray>();
    _RegisterRangeArrayCasts<VtRange2fArray, VtRange2dArray>();
    _RegisterRangeArrayCasts<VtRange3fArray, VtRange3dArray>();
}

PXR_NAMESPACE_CLOSE_SCOPE
