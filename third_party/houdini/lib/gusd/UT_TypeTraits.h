//
// Copyright 2017 Pixar
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
/**
   \file
   \brief
*/
#ifndef _GUSD_UT_TYPETRAITS_H_
#define _GUSD_UT_TYPETRAITS_H_

#include <pxr/pxr.h>
#include <SYS/SYS_Types.h>
#include <SYS/SYS_TypeTraits.h>
#include <SYS/SYS_Version.h>
#include <UT/UT_VectorTypes.h>

PXR_NAMESPACE_OPEN_SCOPE

/** Common type traits.
    Some traits span both HDK and external types.*/
class GusdUT_TypeTraits
{
public:
    /** Traits for a POD tuple (a fixed-size tuple of a single POD type).
        We wouldn't need these if only using UT types.
        We define them in order to have an API to type traits common to both
        the HDK and internal types.
        Use GUSDUT_DECLARE_POD_TUPLE() to declare the type info for your own info.

        TIP: Type traits can be declared against forward-declared classes.*/
    template <class T>
    struct PODTuple
    {
        static const bool   isSpecialized = false;
    };
};


/** Helper for declaring a POD tuple.*/
#define GUSDUT_DECLARE_POD_TUPLE(TYPE, VALUETYPE, TUPLESIZE)          \
    template <>                                                       \
    struct GusdUT_TypeTraits::PODTuple<TYPE> {                        \
        static const bool   isSpecialized = true;                     \
        static const int    tupleSize = TUPLESIZE;                    \
        typedef VALUETYPE   ValueType;                                \
    };

/** Check if a type is a POD tuple.
    TODO: replace these with a constexpr after moving to  C++11 */
#define GUSDUT_IS_PODTUPLE(TYPE)                        \
    (GusdUT_TypeTraits::PODTuple<TYPE>::isSpecialized)

/** Check if two POD tuples are compatible
    (I.e., same tuple size, not necessarily same types).
    TODO: replace these with a constexpr after oving to C++11 */
#define GUSDUT_PODTUPLES_ARE_COMPATIBLE(A,B)        \
    (GusdUT_TypeTraits::PODTuple<A>::tupleSize ==   \
        GusdUT_TypeTraits::PODTuple<B>::tupleSize)

/** Check if a type is a POD tuple.
    TODO: replace these with a constexpr after moving to C++11 */
#define GUSDUT_PODTUPLES_ARE_BYTE_COMPATIBLE(A,B)                       \
    (GUSDUT_PODTUPLES_ARE_COMPATIBLE(A,B) &&                            \
     SYS_IsSame<typename GusdUT_TypeTraits::PODTuple<A>::ValueType, \
                typename GusdUT_TypeTraits::PODTuple<B>::ValueType>::value)


/* Declare traits on core types. */   
GUSDUT_DECLARE_POD_TUPLE(UT_Vector2H, fpreal16, 2);
GUSDUT_DECLARE_POD_TUPLE(UT_Vector3H, fpreal16, 3);
GUSDUT_DECLARE_POD_TUPLE(UT_Vector4H, fpreal16, 4);

GUSDUT_DECLARE_POD_TUPLE(UT_Vector2F, fpreal32, 2);
GUSDUT_DECLARE_POD_TUPLE(UT_Vector3F, fpreal32, 3);
GUSDUT_DECLARE_POD_TUPLE(UT_Vector4F, fpreal32, 4);

GUSDUT_DECLARE_POD_TUPLE(UT_Vector2D, fpreal64, 2);
GUSDUT_DECLARE_POD_TUPLE(UT_Vector3D, fpreal64, 3);
GUSDUT_DECLARE_POD_TUPLE(UT_Vector4D, fpreal64, 4);

GUSDUT_DECLARE_POD_TUPLE(UT_Vector2I, int64, 2);
GUSDUT_DECLARE_POD_TUPLE(UT_Vector3I, int64, 3);
GUSDUT_DECLARE_POD_TUPLE(UT_Vector4I, int64, 4);

GUSDUT_DECLARE_POD_TUPLE(UT_Vector2i, int32, 2);
GUSDUT_DECLARE_POD_TUPLE(UT_Vector3i, int32, 3);
GUSDUT_DECLARE_POD_TUPLE(UT_Vector4i, int32, 4);
 
#if SYS_VERSION_FULL_INT >= 0x11000000
GUSDUT_DECLARE_POD_TUPLE(UT_QuaternionH, fpreal16, 4);
#endif
GUSDUT_DECLARE_POD_TUPLE(UT_QuaternionF, fpreal32, 4);
GUSDUT_DECLARE_POD_TUPLE(UT_QuaternionD, fpreal64, 4);

GUSDUT_DECLARE_POD_TUPLE(UT_Matrix2F, fpreal32, 4);
GUSDUT_DECLARE_POD_TUPLE(UT_Matrix3F, fpreal32, 9);
GUSDUT_DECLARE_POD_TUPLE(UT_Matrix4F, fpreal32, 16);

GUSDUT_DECLARE_POD_TUPLE(UT_Matrix2D, fpreal64, 4);
GUSDUT_DECLARE_POD_TUPLE(UT_Matrix3D, fpreal64, 9);
GUSDUT_DECLARE_POD_TUPLE(UT_Matrix4D, fpreal64, 16);

/* Declare PODs as POD tuples of tupleSize=1.*/
GUSDUT_DECLARE_POD_TUPLE(bool,      bool, 1);
GUSDUT_DECLARE_POD_TUPLE(uint8,     uint8, 1);
GUSDUT_DECLARE_POD_TUPLE(uint16,    uint16, 1);
GUSDUT_DECLARE_POD_TUPLE(uint32,    uint32, 1);
GUSDUT_DECLARE_POD_TUPLE(uint64,    uint64, 1);
GUSDUT_DECLARE_POD_TUPLE(int8,      int8, 1);
GUSDUT_DECLARE_POD_TUPLE(int16,     int16, 1);
GUSDUT_DECLARE_POD_TUPLE(int32,     int32, 1);
GUSDUT_DECLARE_POD_TUPLE(int64,     int64, 1);
GUSDUT_DECLARE_POD_TUPLE(fpreal16,  fpreal16, 1);
GUSDUT_DECLARE_POD_TUPLE(fpreal32,  fpreal32, 1);
GUSDUT_DECLARE_POD_TUPLE(fpreal64,  fpreal64, 1);

PXR_NAMESPACE_CLOSE_SCOPE

#endif /*_GUSD_UT_TYPETRAITS_H_*/
