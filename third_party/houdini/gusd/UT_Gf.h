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
#ifndef GUSD_UT_GF_H
#define GUSD_UT_GF_H

#include "pxr/pxr.h"

#include "UT_TypeTraits.h"

// Conversion specializations require full defs for some types.
#include "pxr/base/gf/quaternion.h"
#include "pxr/base/gf/quatd.h"
#include "pxr/base/gf/quatf.h"
#include "pxr/base/gf/quath.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"

#include <SYS/SYS_TypeTraits.h>
#include <SYS/SYS_Version.h>
#include <UT/UT_VectorTypes.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Helpers for working with Gf types (vectors, matrices, etc.) within the HDK.
struct GusdUT_Gf
{
    /// Struct providing info about type equivalence between
    /// UT and Gf types. Each struct defines:
    ///
    /// \code
    ///     static const bool isSpecialized = true;
    ///     typedef ... GfType;
    ///     typedef ... UtType;
    ///     typedef ... AltType; // Type from the alternate API.
    ///                          // Eg., if this is a UT type, it will be the Gf
    ///                          // type, and vice versa.
    /// \endcode
    template <class GF_OR_UT_TYPE>
    struct TypeEquivalence
    {
        static const bool isSpecialized = false;
    };

    /// Struct defining whether or not a type is valid for direct casting to
    /// other types. We explicitly disable casting for types that require some
    /// kind of data manipulation when going in-between UT and Gf.
    template <class GF_OR_UT_TYPE>
    struct Castable
    {
        static const bool value = true;
    };

    /// Helpers for casting between UT and Gf types. The cast can go either way.
    /// This can be done with a reinterpret cast, but this cast adds a bit of
    /// extra compile-time checks to make sure that this really is safe.
    ///
    /// These cast methods only take a single template argument. The output
    /// is cast to the equivalent type from the alternate API. For example,
    /// if given a UT_Matrix4D, the cast is to a GfMatrix4d, and vice versa.
    ///
    /// \note Any type used here must be declared with a specialization of
    /// GusdUT_TypeTraits::PODTuple (see GUSD_DECLARE_POD_TUPLE).
    ///
    /// Examples:
    ///
    /// \code
    ///     // implicit cast of Gf->UT
    ///     UT_Matrix4D& mx = GusdUT_Gf::Cast(gfMatrix4dInstance);
    ///     UT_Matrix4D* mx = GusdUT_Gf::Cast(gfMatrix4dInstancePtr);
    ///     const UT_Matrix4D& mx = GusdUT_Gf::Cast(gfMatrix4dInstanceConst);
    ///     const UT_Matrix4D* mx = GusdUT_Gf::Cast(gfMatrix4dInstancePtrConst);
    ///
    ///     // implicit cast of UT->Gf
    ///     GfMatrix4d& mx = GusdUT_Gf::Cast(utMatrix4dInstance);
    ///     GfMatrix4d* mx = GusdUT_Gf::Cast(utMatrix4dInstancePtr);
    ///     const GfMatrix4d& mx = GusdUT_Gf::Cast(utMatrix4dInstanceConst);
    ///     const GfMatrix4d* mx = GusdUT_Gf::Cast(utMatrix4dInstancePtrConst);
    ///
    ///     // compile error! types are not bitwise compatible
    ///     UT_Matrix4D src;
    ///     GfMatrix4f& mx = GusdUT_Gf::Cast(src);
    ///
    ///     // compile error! discards cv-qualifier.
    ///     const UT_Matrix4F& src = ...;
    ///     GfMatrix4f& mx = GusdUT_Gf::Cast(src);
    /// \endcode
    /// @{
    template <class T>
    static inline const typename
    TypeEquivalence<T>::AltType*    Cast(const T* val);

    template <class T>
    static inline typename
    TypeEquivalence<T>::AltType*    Cast(T* val);

    template <class T>
    static inline const typename
    TypeEquivalence<T>::AltType&    Cast(const T& val);

    template <class T>
    static inline typename
    TypeEquivalence<T>::AltType&    Cast(T& val);
    /// @}

    /// Explicit casts between UT and Gf types.
    /// This is just like the implicit cast methods, except that the source and
    /// target types are explicitly specified via template arguments.
    /// This can be used for casting between types when the types aren't exact
    /// counterparts. For instance, we can safely cast a GfMatrix2d to a
    /// UT_Vector4D, even though UT_Vector4D is not UT's equivalence type for
    /// GfMatrix2d.
    ///
    /// @{
    template <class FROM, class TO>
    static inline const TO* Cast(const FROM* val);

    template <class FROM, class TO>
    static inline TO*       Cast(FROM* val);

    template <class FROM, class TO>
    static inline const TO& Cast(const FROM& val);

    template <class FROM, class TO>
    static inline TO&       Cast(FROM& val);
    /// @}


    /// Convert between UT and Gf types. This works for any pod tuples that have
    /// equivalent tuple sizes, even if their underlying precision differs.
    ///
    /// \note Any type used here must be declared with a specialization of
    /// GusdUT_TypeTraits::PODTuple (see GUSD_DECLARE_POD_TUPLE).
    template <class FROM, class TO>
    static inline void  Convert(const FROM& from, TO& to);


    /// Conversions between GF and UT quaternions.
    /// Gf and UT have a different ordering of the real component,
    /// hence the need for speciailized converters.
    ///
    /// XXX: 4d vector types are sometimes used in place of GfQuaternion,
    /// hence their inclusion here. That is primarily the fault of USD:
    /// if USD gets a real quaternion type, we can clean these up.
    /// @{
    template <class T>
    static inline void Convert(const GfQuaternion& from, UT_QuaternionT<T>& to);

    template <class T>
    static inline void Convert(const GfQuatd& from, UT_QuaternionT<T>& to);

    template <class T>
    static inline void Convert(const GfQuatf& from, UT_QuaternionT<T>& to);

    template <class T>
    static inline void Convert(const GfQuath& from, UT_QuaternionT<T>& to);

    template <class T>             
    static inline void Convert(const GfVec4d& from, UT_QuaternionT<T>& to);

    template <class T>             
    static inline void Convert(const GfVec4f& from, UT_QuaternionT<T>& to);

    template <class T>
    static inline void Convert(const UT_QuaternionT<T>& from, GfQuaternion& to);

    template <class T>
    static inline void Convert(const UT_QuaternionT<T>& from, GfQuatd& to);

    template <class T>
    static inline void Convert(const UT_QuaternionT<T>& from, GfQuatf& to);

    template <class T>
    static inline void Convert(const UT_QuaternionT<T>& from, GfQuath& to);

    template <class T>
    static inline void Convert(const UT_QuaternionT<T>& from, GfVec4d& to);

    template <class T>
    static inline void Convert(const UT_QuaternionT<T>& from, GfVec4f& to);
    /// @}

private:
    template <class T, class GFQUAT>
    static inline void _ConvertQuat(const GFQUAT& from, UT_QuaternionT<T>& to);

    template <class T>
    static inline void  _AssertIsPodTuple();

    template <class FROM, class TO>
    static inline void  _AssertCanCast();

    // Our casting tricks assume that the typedefs set in SYS_Types are
    // referencing the types we think they are. Verify our assumptions.
    static_assert(std::is_same<fpreal32,float>::value,
                  "std::is_same<fpreal32,float>::value");
    static_assert(std::is_same<fpreal64,double>::value,
                  "std::is_same<fpreal64,double>::value");
};


/// Declare a type as being uncastable.
#define _GUSDUT_DECLARE_UNCASTABLE(TYPE)        \
    template <>                                 \
    struct GusdUT_Gf::Castable<TYPE>            \
    {                                           \
        static const bool value = false;        \
    };


/// Declare a partial type equivalence. This specifies a one-way
/// type equivalence.
#define _GUSDUT_DECLARE_PARTIAL_EQUIVALENCE(TYPE,GFTYPE,UTTYPE,ALTTYPE) \
    template <>                                                         \
    struct GusdUT_Gf::TypeEquivalence<TYPE> {                           \
        static const bool   isSpecialized = true;                       \
        using GfType = GFTYPE;                                          \
        using UtType = UTTYPE;                                          \
        using AltType = ALTTYPE;                                        \
    };

/// Declare type equivalent between UT and Gf types.
/// Only a single equivalence relationship may be defined per type.
///
/// The type info for both types must be declared first!
#define _GUSDUT_DECLARE_EQUIVALENCE(GFTYPE,UTTYPE)                    \
    _GUSDUT_DECLARE_PARTIAL_EQUIVALENCE(GFTYPE,GFTYPE,UTTYPE,UTTYPE); \
    _GUSDUT_DECLARE_PARTIAL_EQUIVALENCE(UTTYPE,GFTYPE,UTTYPE,GFTYPE);

/// Declare POD tuples for Gf types.
GUSDUT_DECLARE_POD_TUPLE(class GfVec2h, fpreal16, 2);
GUSDUT_DECLARE_POD_TUPLE(class GfVec3h, fpreal16, 3);
GUSDUT_DECLARE_POD_TUPLE(class GfVec4h, fpreal16, 4);

GUSDUT_DECLARE_POD_TUPLE(class GfVec2f, fpreal32, 2);
GUSDUT_DECLARE_POD_TUPLE(class GfVec3f, fpreal32, 3);
GUSDUT_DECLARE_POD_TUPLE(class GfVec4f, fpreal32, 4);

GUSDUT_DECLARE_POD_TUPLE(class GfVec2d, fpreal64, 2);
GUSDUT_DECLARE_POD_TUPLE(class GfVec3d, fpreal64, 3);
GUSDUT_DECLARE_POD_TUPLE(class GfVec4d, fpreal64, 4);

GUSDUT_DECLARE_POD_TUPLE(class GfVec2i, int32, 2);
GUSDUT_DECLARE_POD_TUPLE(class GfVec3i, int32, 3);
GUSDUT_DECLARE_POD_TUPLE(class GfVec4i, int32, 4);

GUSDUT_DECLARE_POD_TUPLE(class GfQuath, fpreal16, 4);
GUSDUT_DECLARE_POD_TUPLE(class GfQuatf, fpreal32, 4);
GUSDUT_DECLARE_POD_TUPLE(class GfQuatd, fpreal64, 4);

GUSDUT_DECLARE_POD_TUPLE(class GfMatrix2f, fpreal32, 4);
GUSDUT_DECLARE_POD_TUPLE(class GfMatrix3f, fpreal32, 9);
GUSDUT_DECLARE_POD_TUPLE(class GfMatrix4f, fpreal32, 16);

GUSDUT_DECLARE_POD_TUPLE(class GfMatrix2d, fpreal64, 4);
GUSDUT_DECLARE_POD_TUPLE(class GfMatrix3d, fpreal64, 9);
GUSDUT_DECLARE_POD_TUPLE(class GfMatrix4d, fpreal64, 16);

PXR_NAMESPACE_CLOSE_SCOPE

/// Declare types as PODs, so that UT_Arrays of the types can be optimized.
/// This is done to ensure that Gf types are handled in the same manner as
/// UT types: the same is done for UT types as well (see UT/UT_VectorTypes.h).

SYS_DECLARE_IS_POD(PXR_NS::GfHalf);

SYS_DECLARE_IS_POD(PXR_NS::GfVec2h);
SYS_DECLARE_IS_POD(PXR_NS::GfVec3h);
SYS_DECLARE_IS_POD(PXR_NS::GfVec4h);

SYS_DECLARE_IS_POD(PXR_NS::GfVec2f);
SYS_DECLARE_IS_POD(PXR_NS::GfVec3f);
SYS_DECLARE_IS_POD(PXR_NS::GfVec4f);

SYS_DECLARE_IS_POD(PXR_NS::GfVec2d);
SYS_DECLARE_IS_POD(PXR_NS::GfVec3d);
SYS_DECLARE_IS_POD(PXR_NS::GfVec4d);

SYS_DECLARE_IS_POD(PXR_NS::GfVec2i);
SYS_DECLARE_IS_POD(PXR_NS::GfVec3i);
SYS_DECLARE_IS_POD(PXR_NS::GfVec4i);

SYS_DECLARE_IS_POD(PXR_NS::GfQuath);
SYS_DECLARE_IS_POD(PXR_NS::GfQuatf);
SYS_DECLARE_IS_POD(PXR_NS::GfQuatd);

SYS_DECLARE_IS_POD(PXR_NS::GfMatrix2f);
SYS_DECLARE_IS_POD(PXR_NS::GfMatrix3f);
SYS_DECLARE_IS_POD(PXR_NS::GfMatrix4f);

SYS_DECLARE_IS_POD(PXR_NS::GfMatrix2d);
SYS_DECLARE_IS_POD(PXR_NS::GfMatrix3d);
SYS_DECLARE_IS_POD(PXR_NS::GfMatrix4d);

PXR_NAMESPACE_OPEN_SCOPE

// No casting on quaternions; real component is ordered
// in a different way between UT and Gf.
_GUSDUT_DECLARE_UNCASTABLE(class GfQuaternion);
_GUSDUT_DECLARE_UNCASTABLE(class GfQuatf);
_GUSDUT_DECLARE_UNCASTABLE(class GfQuatd);
_GUSDUT_DECLARE_UNCASTABLE(UT_QuaternionF);
_GUSDUT_DECLARE_UNCASTABLE(UT_QuaternionD);
#if SYS_VERSION_FULL_INT >= 0x11000000
_GUSDUT_DECLARE_UNCASTABLE(UT_QuaternionH);
#endif

// Declare type correspondances between Gf and UT.
_GUSDUT_DECLARE_EQUIVALENCE(class GfVec2h, UT_Vector2H);
_GUSDUT_DECLARE_EQUIVALENCE(class GfVec3h, UT_Vector3H);
_GUSDUT_DECLARE_EQUIVALENCE(class GfVec4h, UT_Vector4H);

_GUSDUT_DECLARE_EQUIVALENCE(class GfVec2d, UT_Vector2D);
_GUSDUT_DECLARE_EQUIVALENCE(class GfVec3d, UT_Vector3D);
_GUSDUT_DECLARE_EQUIVALENCE(class GfVec4d, UT_Vector4D);

_GUSDUT_DECLARE_EQUIVALENCE(class GfVec2f, UT_Vector2F);
_GUSDUT_DECLARE_EQUIVALENCE(class GfVec3f, UT_Vector3F);
_GUSDUT_DECLARE_EQUIVALENCE(class GfVec4f, UT_Vector4F);

_GUSDUT_DECLARE_EQUIVALENCE(class GfVec2i, UT_Vector2i);
_GUSDUT_DECLARE_EQUIVALENCE(class GfVec3i, UT_Vector3i);
_GUSDUT_DECLARE_EQUIVALENCE(class GfVec4i, UT_Vector4i);

_GUSDUT_DECLARE_EQUIVALENCE(class GfQuatd, UT_QuaternionD);
_GUSDUT_DECLARE_EQUIVALENCE(class GfQuatf, UT_QuaternionF);
#if SYS_VERSION_FULL_INT >= 0x11000000
_GUSDUT_DECLARE_EQUIVALENCE(class GfQuath, UT_QuaternionH);
#endif

_GUSDUT_DECLARE_EQUIVALENCE(class GfMatrix2d, UT_Matrix2D);
_GUSDUT_DECLARE_EQUIVALENCE(class GfMatrix3d, UT_Matrix3D);
_GUSDUT_DECLARE_EQUIVALENCE(class GfMatrix4d, UT_Matrix4D);

_GUSDUT_DECLARE_EQUIVALENCE(class GfMatrix2f, UT_Matrix2F);
_GUSDUT_DECLARE_EQUIVALENCE(class GfMatrix3f, UT_Matrix3F);
_GUSDUT_DECLARE_EQUIVALENCE(class GfMatrix4f, UT_Matrix4F);


template <class T>
void
GusdUT_Gf::_AssertIsPodTuple()
{
    static_assert(GusdIsPodTuple<T>(), "Type is not declared as a POD-tuple");
}


template <class FROM, class TO>
void
GusdUT_Gf::_AssertCanCast()
{
    _AssertIsPodTuple<FROM>();
    _AssertIsPodTuple<TO>();
    static_assert(Castable<FROM>::value, "Source is not castable");
    static_assert(Castable<TO>::value, "Output is not castable");
    static_assert(GusdPodTuplesAreBitwiseCompatible<FROM,TO>(),
                  "Types in cast are not bitwise compatible");
}


template <class FROM, class TO>
const TO*
GusdUT_Gf::Cast(const FROM* val)
{
    _AssertCanCast<FROM,TO>();
    return reinterpret_cast<const TO*>(val);
}


template <class FROM, class TO>
TO*
GusdUT_Gf::Cast(FROM* val)
{
    _AssertCanCast<FROM,TO>();
    return reinterpret_cast<TO*>(val);
}


template <class FROM, class TO>
const TO&
GusdUT_Gf::Cast(const FROM& val)
{
    _AssertCanCast<FROM,TO>();
    return reinterpret_cast<const TO&>(val);
}


template <class FROM, class TO>
TO&
GusdUT_Gf::Cast(FROM& val)
{
    _AssertCanCast<FROM,TO>();
    return reinterpret_cast<TO&>(val);
}


template <class T>
typename GusdUT_Gf::TypeEquivalence<T>::AltType&
GusdUT_Gf::Cast(T& val)
{
    _AssertIsPodTuple<T>();
    return Cast<T,typename TypeEquivalence<T>::AltType>(val);
}


template <class T>
const typename GusdUT_Gf::TypeEquivalence<T>::AltType&
GusdUT_Gf::Cast(const T& val)
{
    _AssertIsPodTuple<T>();
    return Cast<T, typename TypeEquivalence<T>::AltType>(val);
}


template <class T>
typename GusdUT_Gf::TypeEquivalence<T>::AltType*
GusdUT_Gf::Cast(T* val)
{
    _AssertIsPodTuple<T>();
    return Cast<T, typename TypeEquivalence<T>::AltType>(val);
}


template <class T>
const typename GusdUT_Gf::TypeEquivalence<T>::AltType*
GusdUT_Gf::Cast(const T* val)
{
    _AssertIsPodTuple<T>();
    return Cast<T, typename TypeEquivalence<T>::AltType>(val);
}


template <class FROM, class TO>
void
GusdUT_Gf::Convert(const FROM& from, TO& to)
{
    using FromPodType = typename GusdPodTupleTraits<FROM>::ValueType;
    using ToPodType =   typename GusdPodTupleTraits<TO>::ValueType;

    _AssertIsPodTuple<FROM>();
    _AssertIsPodTuple<TO>();
    static_assert(GusdPodTuplesAreCompatible<FROM,TO>(),
                  "Types are not compatible (mismatched tuple sizes)");

    const auto* src = reinterpret_cast<const FromPodType*>(&from);
    ToPodType* dst =  reinterpret_cast<ToPodType*>(&to);

    for (int i = 0; i < GusdGetTupleSize<FROM>(); ++i) {
        dst[i] = static_cast<ToPodType>(src[i]);
    }
}


template <class T, class GFQUAT>
void
GusdUT_Gf::_ConvertQuat(const GFQUAT& from, UT_QuaternionT<T>& to)
{
    reinterpret_cast<UT_Vector3T<T>&>(to) = Cast(from.GetImaginary());
    to(3) = from.GetReal();
}


template <class T>
void
GusdUT_Gf::Convert(const GfQuaternion& from, UT_QuaternionT<T>& to)
{
    return _ConvertQuat(from, to);
}


template <class T>
void
GusdUT_Gf::Convert(const GfQuatd& from, UT_QuaternionT<T>& to)
{
    return _ConvertQuat(from, to);
}


template <class T>
void
GusdUT_Gf::Convert(const GfQuatf& from, UT_QuaternionT<T>& to)
{
    return _ConvertQuat(from, to);
}


template <class T>
void
GusdUT_Gf::Convert(const GfQuath& from, UT_QuaternionT<T>& to)
{
    return _ConvertQuat(from, to);
}


template <class T>
void
GusdUT_Gf::Convert(const GfVec4d& from, UT_QuaternionT<T>& to)
{
    to = UT_QuaternionT<T>(from[1],from[2],from[3],from[0]);
}


template <class T>
void
GusdUT_Gf::Convert(const GfVec4f& from, UT_QuaternionT<T>& to)
{
    to = UT_QuaternionT<T>(from[1],from[2],from[3],from[0]);
}


template <class T>
void
GusdUT_Gf::Convert(const UT_QuaternionT<T>& from, GfQuaternion& to)
{
    to.SetReal(from.w());
    to.SetImaginary(GfVec3d(from.x(), from.y(), from.z()));
}


template <class T>
void
GusdUT_Gf::Convert(const UT_QuaternionT<T>& from, GfQuatd& to)
{
    to.SetReal(from.w());
    to.SetImaginary(GfVec3d(from.x(), from.y(), from.z()));
}


template <class T>
void
GusdUT_Gf::Convert(const UT_QuaternionT<T>& from, GfQuatf& to)
{
    to.SetReal(from.w());
    to.SetImaginary(GfVec3f(from.x(), from.y(), from.z()));
}


template <class T>
void
GusdUT_Gf::Convert(const UT_QuaternionT<T>& from, GfQuath& to)
{
    to.SetReal(GfHalf(from.w()));
    to.SetImaginary(
        GfVec3h(GfHalf(from.x()), GfHalf(from.y()), GfHalf(from.z())));
}


template <class T>
void
GusdUT_Gf::Convert(const UT_QuaternionT<T>& from, GfVec4d& to)
{
    to = GfVec4d(from.w(), from.x(), from.y(), from.z());
}


template <class T>
void
GusdUT_Gf::Convert(const UT_QuaternionT<T>& from, GfVec4f& to)
{
    to = GfVec4f(from.w(), from.x(), from.y(), from.z());
}


#undef _GUSDUT_DECLARE_UNCASTABLE
#undef _GUSDUT_DECLARE_PARTIAL_EQUIVALENCE
#undef _GUSDUT_DECLARE_EQUIVALENCE

PXR_NAMESPACE_CLOSE_SCOPE

#endif /*GUSD_UT_GF_H*/
