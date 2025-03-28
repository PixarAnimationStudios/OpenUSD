//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_VT_TYPES_H
#define PXR_BASE_VT_TYPES_H

/// \file vt/types.h
/// Defines all the types "TYPED" for which Vt creates a VtTYPEDArray typedef.

#include "pxr/pxr.h"
#include "pxr/base/vt/api.h"
#include "pxr/base/vt/traits.h"
#include "pxr/base/arch/inttypes.h"
#include "pxr/base/gf/declare.h"
#include "pxr/base/gf/half.h"
#include "pxr/base/tf/meta.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"
#include "pxr/base/tf/token.h"

#include <cstddef>
#include <cstring>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

// Help ensure TfToken is stored in local storage in VtValue by indicating it is
// cheap to copy (just refcount operations).
VT_TYPE_IS_CHEAP_TO_COPY(TfToken);

// Value types.

#define VT_FLOATING_POINT_BUILTIN_VALUE_TYPES \
((      double,                Double )) \
((      float,                 Float  )) \
((      GfHalf,                Half   ))

#define VT_INTEGRAL_BUILTIN_VALUE_TYPES     \
((      bool,                  Bool   ))    \
((      char,                  Char   ))    \
((      unsigned char,         UChar  ))    \
((      short,                 Short  ))    \
((      unsigned short,        UShort ))    \
((      int,                   Int    ))    \
((      unsigned int,          UInt   ))    \
((      int64_t,               Int64  ))    \
((      uint64_t,              UInt64 ))

#define VT_VEC_INT_VALUE_TYPES         \
((      GfVec4i,             Vec4i  )) \
((      GfVec3i,             Vec3i  )) \
((      GfVec2i,             Vec2i  )) 

#define VT_VEC_HALF_VALUE_TYPES        \
((      GfVec4h,             Vec4h  )) \
((      GfVec3h,             Vec3h  )) \
((      GfVec2h,             Vec2h  ))

#define VT_VEC_FLOAT_VALUE_TYPES       \
((      GfVec4f,             Vec4f  )) \
((      GfVec3f,             Vec3f  )) \
((      GfVec2f,             Vec2f  ))

#define VT_VEC_DOUBLE_VALUE_TYPES      \
((      GfVec4d,             Vec4d  )) \
((      GfVec3d,             Vec3d  )) \
((      GfVec2d,             Vec2d  ))

#define VT_VEC_VALUE_TYPES   \
    VT_VEC_INT_VALUE_TYPES   \
    VT_VEC_HALF_VALUE_TYPES  \
    VT_VEC_FLOAT_VALUE_TYPES \
    VT_VEC_DOUBLE_VALUE_TYPES

#define VT_MATRIX_FLOAT_VALUE_TYPES      \
((      GfMatrix4f,          Matrix4f )) \
((      GfMatrix3f,          Matrix3f )) \
((      GfMatrix2f,          Matrix2f )) \

#define VT_MATRIX_DOUBLE_VALUE_TYPES     \
((      GfMatrix4d,          Matrix4d )) \
((      GfMatrix3d,          Matrix3d )) \
((      GfMatrix2d,          Matrix2d ))

#define VT_MATRIX_VALUE_TYPES            \
    VT_MATRIX_FLOAT_VALUE_TYPES          \
    VT_MATRIX_DOUBLE_VALUE_TYPES         \

#define VT_GFRANGE_VALUE_TYPES                 \
((      GfRange3f,           Range3f        )) \
((      GfRange3d,           Range3d        )) \
((      GfRange2f,           Range2f        )) \
((      GfRange2d,           Range2d        )) \
((      GfRange1f,           Range1f        )) \
((      GfRange1d,           Range1d        ))

#define VT_RANGE_VALUE_TYPES                   \
    VT_GFRANGE_VALUE_TYPES                     \
((      GfInterval,          Interval       )) \
((      GfRect2i,            Rect2i         ))

#define VT_STRING_VALUE_TYPES            \
((      std::string,           String )) \
((      TfToken,               Token  ))

#define VT_QUATERNION_VALUE_TYPES           \
((      GfQuath,             Quath ))       \
((      GfQuatf,             Quatf ))       \
((      GfQuatd,             Quatd ))       \
((      GfQuaternion,        Quaternion ))

#define VT_DUALQUATERNION_VALUE_TYPES       \
((      GfDualQuath,         DualQuath ))   \
((      GfDualQuatf,         DualQuatf ))   \
((      GfDualQuatd,         DualQuatd ))

#define VT_NONARRAY_VALUE_TYPES                 \
((      GfFrustum,           Frustum))          \
((      GfMultiInterval,     MultiInterval))

// Helper macros for extracting bits from a type tuple.
#define VT_TYPE(elem) \
TF_PP_TUPLE_ELEM(0, elem)
#define VT_TYPE_NAME(elem) \
TF_PP_TUPLE_ELEM(1, elem)


// Composite groups of types.
#define VT_BUILTIN_NUMERIC_VALUE_TYPES \
VT_INTEGRAL_BUILTIN_VALUE_TYPES VT_FLOATING_POINT_BUILTIN_VALUE_TYPES 

#define VT_BUILTIN_VALUE_TYPES \
VT_BUILTIN_NUMERIC_VALUE_TYPES VT_STRING_VALUE_TYPES

#define VT_SCALAR_CLASS_VALUE_TYPES \
VT_VEC_VALUE_TYPES \
VT_MATRIX_VALUE_TYPES \
VT_RANGE_VALUE_TYPES \
VT_QUATERNION_VALUE_TYPES \
VT_DUALQUATERNION_VALUE_TYPES

#define VT_SCALAR_VALUE_TYPES \
VT_SCALAR_CLASS_VALUE_TYPES VT_BUILTIN_VALUE_TYPES 


// The following preprocessor code produces typedefs for VtArray holding
// various scalar value types.  The produced typedefs are of the form:
//
// typedef VtArray<int> VtIntArray;
// typedef VtArray<double> VtDoubleArray;
template<typename T> class VtArray;
#define VT_ARRAY_TYPEDEF(unused, elem) \
typedef VtArray< VT_TYPE(elem) > \
TF_PP_CAT(Vt, TF_PP_CAT(VT_TYPE_NAME(elem), Array)) ;
TF_PP_SEQ_FOR_EACH(VT_ARRAY_TYPEDEF, ~, VT_SCALAR_VALUE_TYPES)

// The following preprocessor code generates the boost pp sequence for
// all array value types (VT_ARRAY_VALUE_TYPES)
#define VT_ARRAY_TYPE_TUPLE(unused, elem) \
(( TF_PP_CAT(Vt, TF_PP_CAT(VT_TYPE_NAME(elem), Array)) , \
   TF_PP_CAT(VT_TYPE_NAME(elem), Array) ))
#define VT_ARRAY_VALUE_TYPES \
TF_PP_SEQ_FOR_EACH(VT_ARRAY_TYPE_TUPLE, ~, VT_SCALAR_VALUE_TYPES)

#define VT_CLASS_VALUE_TYPES \
VT_ARRAY_VALUE_TYPES VT_SCALAR_CLASS_VALUE_TYPES VT_NONARRAY_VALUE_TYPES

#define VT_VALUE_TYPES \
    VT_BUILTIN_VALUE_TYPES VT_CLASS_VALUE_TYPES

#define _VT_MAP_TYPE_LIST(unused, elem) , VT_TYPE(elem)

// Populate a type list from the preprocessor sequence.
// void is prepended to match the comma for the first type
// and then dropped by TfMetaTail.
using Vt_ValueTypeList =
    TfMetaApply<TfMetaTail, TfMetaList<
        void TF_PP_SEQ_FOR_EACH(_VT_MAP_TYPE_LIST, ~, VT_VALUE_TYPES)>>;

namespace Vt_KnownValueTypeDetail
{

// Implement compile-time value type indexes.
// Base case -- unknown types get index -1.
template <typename T>
constexpr int
GetIndexImpl(TfMetaList<>) {
    return -1;
}

template <typename T, typename Typelist>
constexpr int
GetIndexImpl(Typelist) {
    if (std::is_same_v<T, TfMetaApply<TfMetaHead, Typelist>>) {
        return 0;
    }
    else if (const int indexOfTail =
             GetIndexImpl<T>(TfMetaApply<TfMetaTail, Typelist>{});
             indexOfTail >= 0) {
        return 1 + indexOfTail;
    }
    else {
        return -1;
    }
}

template <typename T>
constexpr int
GetIndex() {
    return GetIndexImpl<T>(Vt_ValueTypeList{});
}

} // Vt_KnownValueTypeDetail

// Total number of 'known' value types.
constexpr int
VtGetNumKnownValueTypes() {
    return TfMetaApply<TfMetaLength, Vt_ValueTypeList>::value;
}

/// Provide compile-time value type indexes for types that are "known" to Vt --
/// specifically, those types that appear in VT_VALUE_TYPES.  Note that VtArray
/// and VtValue can work with other types that are not these "known" types.
///
/// VtGetKnownValueTypeIndex can only be used with known types.  Querying a
/// type that is not known to Vt results in a compilation error.  The set of
/// known types and their indexes are not guaranteed to be stable across
/// releases of the library.
///
/// Most clients should prefer VtVisitValue over direct use of the type index
/// as VtVisitValue provides convenient and efficient access to the held
/// value.
template <class T>
constexpr int
VtGetKnownValueTypeIndex()
{
    constexpr int index = Vt_KnownValueTypeDetail::GetIndex<T>();
    static_assert(index != -1, "T is not one of the known VT_VALUE_TYPES.");
    return index;
}

/// Returns true if `T` is a type that appears in VT_VALUE_TYPES.
template <class T>
constexpr bool
VtIsKnownValueType()
{
    return Vt_KnownValueTypeDetail::GetIndex<T>() != -1;
}

// XXX: Works around an MSVC bug where constexpr functions cannot be used as the
// condition in enable_if, fixed in MSVC 2022 version 14.33 1933 (version 17.3).
// https://developercommunity.visualstudio.com/t/function-template-has-already-been-defined-using-s/833543
template <class T>
struct VtIsKnownValueType_Workaround
{
    static const bool value = VtIsKnownValueType<T>();
};

// None of the VT_VALUE_TYPES are value proxies.  We want to specialize these
// templates here, since otherwise the VtIsTypedValueProxy will require a
// complete type to check if it derives VtTypedValueProxyBase.
#define VT_SPECIALIZE_IS_VALUE_PROXY(unused, elem)                             \
    template <> struct                                                         \
    VtIsValueProxy< VT_TYPE(elem) > : std::false_type {};                      \
    template <> struct                                                         \
    VtIsTypedValueProxy< VT_TYPE(elem) > : std::false_type {};                 \
    template <> struct                                                         \
    VtIsErasedValueProxy< VT_TYPE(elem) > : std::false_type {};
TF_PP_SEQ_FOR_EACH(VT_SPECIALIZE_IS_VALUE_PROXY, ~, VT_VALUE_TYPES)
#undef VT_SPECIALIZE_IS_VALUE_PROXY

// Free functions to represent "zero" for various base types.  See
// specializations in Types.cpp
template<typename T>
T VtZero();

// Shape representation used in VtArray for legacy code.  This is not supported
// at the pxr level or in usd.  Shape is represented by a total size, plus sized
// dimensions other than the last.  The size of the last dimension is computed
// as totalSize / (product-of-other-dimensions).
struct Vt_ShapeData {
    unsigned int GetRank() const {
        return
            otherDims[0] == 0 ? 1 :
            otherDims[1] == 0 ? 2 :
            otherDims[2] == 0 ? 3 : 4;
    }
    bool operator==(Vt_ShapeData const &other) const {
        if (totalSize != other.totalSize)
            return false;
        unsigned int thisRank = GetRank(), otherRank = other.GetRank();
        if (thisRank != otherRank)
            return false;
        return std::equal(otherDims, otherDims + GetRank() - 1,
                          other.otherDims);
    }
    bool operator!=(Vt_ShapeData const &other) const {
        return !(*this == other);
    }
    void clear() {
        memset(this, 0, sizeof(*this));
    }
    static const int NumOtherDims = 3;
    size_t totalSize;
    unsigned int otherDims[NumOtherDims];
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_VT_TYPES_H
