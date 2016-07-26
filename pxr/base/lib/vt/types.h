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
/// \file vt/types.h
/// Defines all the types "TYPED" for which Vt creates a VtTYPEDArray typedef.
/// 
#ifndef VT_TYPES_H
#define VT_TYPES_H

#include "pxr/base/arch/inttypes.h"
#include "pxr/base/gf/declare.h"
#include "pxr/base/tf/token.h"

#include <boost/preprocessor.hpp>

#include <string>
#include <cstddef>

// Value types.

#define VT_FLOATING_POINT_BUILTIN_VALUE_TYPES \
((      double,                Double )) \
((      float,                 Float  )) \
((      half,                  Half   ))

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

#define VT_NONARRAY_VALUE_TYPES                 \
((      GfFrustum,           Frustum))          \
((      GfMultiInterval,     MultiInterval))

// Helper macros for extracting bits from a type tuple.
#define VT_TYPE(elem) \
BOOST_PP_TUPLE_ELEM(2, 0, elem)
#define VT_TYPE_NAME(elem) \
BOOST_PP_TUPLE_ELEM(2, 1, elem)   


// Composite groups of types.
#define VT_BUILTIN_NUMERIC_VALUE_TYPES \
VT_INTEGRAL_BUILTIN_VALUE_TYPES VT_FLOATING_POINT_BUILTIN_VALUE_TYPES 

#define VT_BUILTIN_VALUE_TYPES \
VT_BUILTIN_NUMERIC_VALUE_TYPES VT_STRING_VALUE_TYPES

#define VT_SCALAR_CLASS_VALUE_TYPES \
VT_VEC_VALUE_TYPES \
VT_MATRIX_VALUE_TYPES \
VT_RANGE_VALUE_TYPES \
VT_QUATERNION_VALUE_TYPES

#define VT_SCALAR_VALUE_TYPES \
VT_SCALAR_CLASS_VALUE_TYPES VT_BUILTIN_VALUE_TYPES 


// The following preprocessor code produces typedefs for VtArray holding
// various scalar value types.  The produced typedefs are of the form:
//
// typedef VtArray<int> VtIntArray;
// typedef VtArray<double> VtDoubleArray;
template<typename T> class VtArray;
#define VT_ARRAY_TYPEDEF(r, unused, elem) \
typedef VtArray< VT_TYPE(elem) > \
BOOST_PP_CAT(Vt, BOOST_PP_CAT(VT_TYPE_NAME(elem), Array)) ;
BOOST_PP_SEQ_FOR_EACH(VT_ARRAY_TYPEDEF, ~, VT_SCALAR_VALUE_TYPES)

// The following preprocessor code generates the boost pp sequence for
// all array value types (VT_ARRAY_VALUE_TYPES)
#define VT_ARRAY_TYPE_TUPLE(r, unused, elem) \
(( BOOST_PP_CAT(Vt, BOOST_PP_CAT(VT_TYPE_NAME(elem), Array)) , \
   BOOST_PP_CAT(VT_TYPE_NAME(elem), Array) ))
#define VT_ARRAY_VALUE_TYPES \
BOOST_PP_SEQ_FOR_EACH(VT_ARRAY_TYPE_TUPLE, ~, VT_SCALAR_VALUE_TYPES)

#define VT_CLASS_VALUE_TYPES \
VT_ARRAY_VALUE_TYPES VT_SCALAR_CLASS_VALUE_TYPES VT_NONARRAY_VALUE_TYPES
    
// Free functions to represent "zero" for various base types.  See
// specializations in Types.cpp
template<typename T>
T VtZero();

struct Vt_Reserved {
    static const size_t NumWords = 4;
    Vt_Reserved() { data[0] = 0; }
    unsigned int data[NumWords];
};

#endif // VT_TYPES_H
