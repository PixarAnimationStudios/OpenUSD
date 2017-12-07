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
#ifndef _GUSD_UT_GF_H_
#define _GUSD_UT_GF_H_


#include "UT_TypeTraits.h"

#include <pxr/pxr.h>
// Conversion specializations require full defs for some types.
#include "pxr/base/gf/quaternion.h"
#include "pxr/base/gf/quatd.h"
#include "pxr/base/gf/quatf.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"

#include <UT/UT_VectorTypes.h>

PXR_NAMESPACE_OPEN_SCOPE

/** Helpers for working with Gf types (vectors, matrices, etc.) within the HDK.*/
class GusdUT_Gf
{
public:
    /** Struct providing info about type equivalence between
        UT and Gf types. Each struct defines:

        @code
        static const bool isSpecialized = true;
        typedef ... GfType;
        typedef ... UtType;
        typedef ... AltType; // Type from the alternate API. Eg., if this is a
                             // UT type, it will be the GfType. */
    template <class GF_OR_UT_TYPE>
    struct TypeEquivalence
    {
        static const bool isSpecialized = false;
    };

    /** Struct defining whether or not a type is valid for direct casting to
        other types. We explicitly disable casting for types that require some
        kind of data manipulation when going in-between UT and Gf.*/
    template <class GF_OR_UT_TYPE>
    struct Castable
    {
        static const bool value = true;
    };

    /** Helpers for casting between UT and Gf types. The cast can go either way.
        You can do this with a reinterpret cast, but this cast adds a bit of
        extra compile-time checks to make sure that this really is safe.
        
        These cast methods only take a single template argument. The output
        is cast to the corresponding type from the alternate API. Eg., if given
        a UT_Matrix4D, the cast is to a GfMatrix4d, and vice versa if given
        a GfMatrix4d.

        @note Any type used here must be declared with a specialization of
        GusdUT_TypeTraits::PODTuple (see Gusd_DECLARE_POD_TUPLE()).

        Examples:
        
        @code
        // implicit cast of Gf->UT
        UT_Matrix4D& mx = GusdUT_Gf::Cast(gfMatrix4dInstance);
        UT_Matrix4D* mx = GusdUT_Gf::Cast(gfMatrix4dInstancePtr);
        const UT_Matrix4D& mx = GusdUT_Gf::Cast(gfMatrix4dInstanceConst);
        const UT_Matrix4D* mx = GusdUT_Gf::Cast(gfMatrix4dInstancePtrConst);

        // implicit cast of UT->Gf
        GfMatrix4d& mx = GusdUT_Gf::Cast(utMatrix4dInstance);
        GfMatrix4d* mx = GusdUT_Gf::Cast(utMatrix4dInstancePtr);
        const GfMatrix4d& mx = GusdUT_Gf::Cast(utMatrix4dInstanceConst);
        const GfMatrix4d* mx = GusdUT_Gf::Cast(utMatrix4dInstancePtrConst);

        // compile error! types aren't byte-compatible
        UT_Matrix4D src;
        GfMatrix4f& mx = GusdUT_Gf::Cast(src);
    
        // compile error! constness is preserved, so can't cast a const
        // down to a non-const.
        const UT_Matrix4F& src = ...;
        GfMatrix4f& mx = GusdUT_Gf::Cast(src);
        @endcode

        @{ */
    template <class T>
    static inline const typename
    TypeEquivalence<T>::AltType*        Cast(const T* val);

    template <class T>
    static inline typename
    TypeEquivalence<T>::AltType*        Cast(T* val);

    template <class T>
    static inline const typename
    TypeEquivalence<T>::AltType&        Cast(const T& val);

    template <class T>
    static inline typename
    TypeEquivalence<T>::AltType&        Cast(T& val);
    /** *} */


    /** Explicit casts between UT and Gf types.
        This is just like the implicit cast methods, except that the source and
        target types are explicitly specified via template arguments.
        This can be used for casting between types when the types aren't exact
        counterparts. For instance, we can safely cast a GfMatrix2d to a
        UT_Vector4D, though the types aren't the same.
        
        @{ */
    template <class FROM, class TO>
    static inline const TO*             Cast(const FROM* val);

    template <class FROM, class TO>
    static inline TO*                   Cast(FROM* val);

    template <class FROM, class TO>
    static inline const TO&             Cast(const FROM& val);

    template <class FROM, class TO>
    static inline TO&                   Cast(FROM& val);
    /** @} */


    /** Convert between UT and Gf types. This works for types that aren't bit
        compatible (eg., varying precisions). Only the tuple sizes must match.

        @note Any type used here must be declared with a specialization of
        GusdUT_TypeTraits::PODTuple (see GUSD_DECLARE_POD_TUPLE()).*/
    template <class FROM, class TO>
    static inline void                  Convert(const FROM& from, TO& to);


    /** Conversions between GF and UT quaternions.
        Gf and UT have a different ordering of the real component,
        hence the need for speciailized converters.

        XXX: 4d vector types are sometimes used in place of GfQuaternion,
        hence their inclusion here. That is primarily the fault of USD:
        if USD gets a real quaternion type, we can clean these up.
        @{ */
    template <class T>
    static inline void                  Convert(const GfQuaternion& from,
                                                UT_QuaternionT<T>& to);

    template <class T>
    static inline void                  Convert(const GfQuatd& from,
                                                UT_QuaternionT<T>& to);

    template <class T>
    static inline void                  Convert(const GfQuatf& from,
                                                UT_QuaternionT<T>& to);

    template <class T>             
    static inline void                  Convert(const GfVec4d& from,
                                                UT_QuaternionT<T>& to);

    template <class T>             
    static inline void                  Convert(const GfVec4f& from,
                                                UT_QuaternionT<T>& to);

    template <class T>
    static inline void                  Convert(const UT_QuaternionT<T>& from,
                                                GfQuaternion& to);

    template <class T>
    static inline void                  Convert(const UT_QuaternionT<T>& from,
                                                GfQuatd& to);

    template <class T>
    static inline void                  Convert(const UT_QuaternionT<T>& from,
                                                GfQuatf& to);

    template <class T>
    static inline void                  Convert(const UT_QuaternionT<T>& from,
                                                GfVec4d& to);

    template <class T>
    static inline void                  Convert(const UT_QuaternionT<T>& from,
                                                GfVec4f& to);
    /** @} */

private:
    template <class T, class GFQUAT>
    static inline void                  _ConvertQuat(const GFQUAT& from,
                                                     UT_QuaternionT<T>& to);

    template <class T>
    static inline void                  _AssertIsPodTuple();

    template <class FROM, class TO>
    static inline void                  _AssertCanCast();

    /** Our casting tricks assume that the typedefs set in SYS_Types are
        referencing the types we think they are. Let's make sure...*/
    static_assert(std::is_same<fpreal32,float>::value,
                  "std::is_same<fpreal32,float>::value");
    static_assert(std::is_same<fpreal64,double>::value,
                  "std::is_same<fpreal64,double>::value");
};


/** Declare a type as being uncastable.*/
#define _GUSDUT_DECLARE_UNCASTABLE(TYPE)        \
    template <>                                 \
    struct GusdUT_Gf::Castable<TYPE>            \
    {                                           \
        static const bool value = false;        \
    };



/** Declare a partial type equivalence. This specifies a one-way
    type equvalence.*/
#define _GUSDUT_DECLARE_PARTIAL_EQUIVALENCE(TYPE,GFTYPE,UTTYPE,ALTTYPE) \
    template <>                                                         \
    struct GusdUT_Gf::TypeEquivalence<TYPE> {                           \
        static const bool   isSpecialized = true;                       \
        typedef GFTYPE      GfType;                                     \
        typedef UTTYPE      UtType;                                     \
        typedef ALTTYPE     AltType;                                    \
    };

/** Declare type equivalent between UT and Gf types.
    Only a single equivalence relationship may be defined per type.
    
    The type info for both types must be declared first! */
#define _GUSDUT_DECLARE_EQUIVALENCE(GFTYPE,UTTYPE)                    \
    _GUSDUT_DECLARE_PARTIAL_EQUIVALENCE(GFTYPE,GFTYPE,UTTYPE,UTTYPE); \
    _GUSDUT_DECLARE_PARTIAL_EQUIVALENCE(UTTYPE,GFTYPE,UTTYPE,GFTYPE);

#define GUSDUT_DECLARE_IS_POD(T1) \
        namespace boost { \
            template <typename T> struct is_pod; \
        template<> struct is_pod<T1> : public std::true_type {}; \
        } \
        /**/

/** Declare POD tuples for Gf types.*/
GUSDUT_DECLARE_POD_TUPLE(class GfVec2f, fpreal32, 2);
GUSDUT_DECLARE_POD_TUPLE(class GfVec3f, fpreal32, 3);
GUSDUT_DECLARE_POD_TUPLE(class GfVec4f, fpreal32, 4);
GUSDUT_DECLARE_POD_TUPLE(class GfVec2d, fpreal64, 2);
GUSDUT_DECLARE_POD_TUPLE(class GfVec3d, fpreal64, 3);
GUSDUT_DECLARE_POD_TUPLE(class GfVec4d, fpreal64, 4);
GUSDUT_DECLARE_POD_TUPLE(class GfVec2i, int32, 2);
GUSDUT_DECLARE_POD_TUPLE(class GfVec3i, int32, 3);
GUSDUT_DECLARE_POD_TUPLE(class GfVec4i, int32, 4);

GUSDUT_DECLARE_POD_TUPLE(class GfQuaternion, fpreal64, 4);

GUSDUT_DECLARE_POD_TUPLE(class GfQuatf, fpreal32, 4);
GUSDUT_DECLARE_POD_TUPLE(class GfQuatd, fpreal64, 4);

GUSDUT_DECLARE_POD_TUPLE(class GfMatrix2f, fpreal32, 4);
GUSDUT_DECLARE_POD_TUPLE(class GfMatrix3f, fpreal32, 9);
GUSDUT_DECLARE_POD_TUPLE(class GfMatrix4f, fpreal32, 16);

GUSDUT_DECLARE_POD_TUPLE(class GfMatrix2d, fpreal64, 4);
GUSDUT_DECLARE_POD_TUPLE(class GfMatrix3d, fpreal64, 9);
GUSDUT_DECLARE_POD_TUPLE(class GfMatrix4d, fpreal64, 16);

GUSDUT_DECLARE_POD_TUPLE(class GfMatrix2i, int32, 4);
GUSDUT_DECLARE_POD_TUPLE(class GfMatrix3i, int32, 9);
GUSDUT_DECLARE_POD_TUPLE(class GfMatrix4i, int32, 16);

GUSDUT_DECLARE_POD_TUPLE(class GfRGB, fpreal32, 3);
GUSDUT_DECLARE_POD_TUPLE(class GfRGBA, fpreal32, 3);

GUSDUT_DECLARE_POD_TUPLE(class GfSize2, std::size_t, 2);
GUSDUT_DECLARE_POD_TUPLE(class GfSize3, std::size_t, 3);

PXR_NAMESPACE_CLOSE_SCOPE

GUSDUT_DECLARE_IS_POD(PXR_NS::GfVec2f);
GUSDUT_DECLARE_IS_POD(PXR_NS::GfVec3f);
GUSDUT_DECLARE_IS_POD(PXR_NS::GfVec4f);

GUSDUT_DECLARE_IS_POD(PXR_NS::GfVec2d);
GUSDUT_DECLARE_IS_POD(PXR_NS::GfVec3d);
GUSDUT_DECLARE_IS_POD(PXR_NS::GfVec4d);

GUSDUT_DECLARE_IS_POD(PXR_NS::GfVec2i);
GUSDUT_DECLARE_IS_POD(PXR_NS::GfVec3i);
GUSDUT_DECLARE_IS_POD(PXR_NS::GfVec4i);

GUSDUT_DECLARE_IS_POD(PXR_NS::GfQuaternion);

GUSDUT_DECLARE_IS_POD(PXR_NS::GfQuatf);
GUSDUT_DECLARE_IS_POD(PXR_NS::GfQuatd);

GUSDUT_DECLARE_IS_POD(PXR_NS::GfMatrix2f);
GUSDUT_DECLARE_IS_POD(PXR_NS::GfMatrix3f);
GUSDUT_DECLARE_IS_POD(PXR_NS::GfMatrix4f);

GUSDUT_DECLARE_IS_POD(PXR_NS::GfMatrix2d);
GUSDUT_DECLARE_IS_POD(PXR_NS::GfMatrix3d);
GUSDUT_DECLARE_IS_POD(PXR_NS::GfMatrix4d);

GUSDUT_DECLARE_IS_POD(PXR_NS::GfMatrix2i);
GUSDUT_DECLARE_IS_POD(PXR_NS::GfMatrix3i);
GUSDUT_DECLARE_IS_POD(PXR_NS::GfMatrix4i);

GUSDUT_DECLARE_IS_POD(PXR_NS::GfRGB);
GUSDUT_DECLARE_IS_POD(PXR_NS::GfRGBA);

GUSDUT_DECLARE_IS_POD(PXR_NS::GfSize2);
GUSDUT_DECLARE_IS_POD(PXR_NS::GfSize3);

PXR_NAMESPACE_OPEN_SCOPE

/* No casting on quaternions; real component is ordered
   differently between UT and Gf. */
_GUSDUT_DECLARE_UNCASTABLE(class GfQuaternion);
_GUSDUT_DECLARE_UNCASTABLE(class GfQuatf);
_GUSDUT_DECLARE_UNCASTABLE(class GfQuatd);
_GUSDUT_DECLARE_UNCASTABLE(UT_QuaternionF);
_GUSDUT_DECLARE_UNCASTABLE(UT_QuaternionD);
// _GUSDUT_DECLARE_UNCASTABLE(UT_QuaternionR);

/** Declare equivalent between Gf and UT.*/
_GUSDUT_DECLARE_EQUIVALENCE(class GfVec2d, UT_Vector2D);
_GUSDUT_DECLARE_EQUIVALENCE(class GfVec3d, UT_Vector3D);
_GUSDUT_DECLARE_EQUIVALENCE(class GfVec4d, UT_Vector4D);

_GUSDUT_DECLARE_EQUIVALENCE(class GfVec2f, UT_Vector2F);
_GUSDUT_DECLARE_EQUIVALENCE(class GfVec3f, UT_Vector3F);
_GUSDUT_DECLARE_EQUIVALENCE(class GfVec4f, UT_Vector4F);

_GUSDUT_DECLARE_EQUIVALENCE(class GfVec2i, UT_Vector2i);
_GUSDUT_DECLARE_EQUIVALENCE(class GfVec3i, UT_Vector3i);
_GUSDUT_DECLARE_EQUIVALENCE(class GfVec4i, UT_Vector4i);

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
    static_assert(GUSDUT_IS_PODTUPLE(T), "Type is declared as a POD-tuple");
}


template <class FROM, class TO>
void
GusdUT_Gf::_AssertCanCast()
{
    _AssertIsPodTuple<FROM>();
    _AssertIsPodTuple<TO>();
    static_assert(Castable<FROM>::value,    "Source is castable");
    static_assert(Castable<TO>::value,      "Output is castable");
    static_assert(GUSDUT_PODTUPLES_ARE_BYTE_COMPATIBLE(FROM,TO),
                  "Types in cast are byte-compatible");
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
    typedef GusdUT_TypeTraits::PODTuple<FROM>   FromTuple;
    typedef GusdUT_TypeTraits::PODTuple<TO>     ToTuple;

    _AssertIsPodTuple<FROM>();
    _AssertIsPodTuple<TO>();
    static_assert(GUSDUT_PODTUPLES_ARE_COMPATIBLE(FROM,TO),
                  "Types are compatible");

    const typename FromTuple::ValueType* src =
        reinterpret_cast<const typename FromTuple::ValueType*>(&from);
    typename ToTuple::ValueType* dst =
        reinterpret_cast<typename ToTuple::ValueType*>(&to);
    for(int i = 0; i < FromTuple::tupleSize; ++i)
        dst[i] = static_cast<typename ToTuple::ValueType>(src[i]);
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
    to.SetReal(from.z());
    to.SetImaginary(GfVec3d(from.x(), from.y(), from.z()));
}


template <class T>
void
GusdUT_Gf::Convert(const UT_QuaternionT<T>& from, GfQuatd& to)
{
    to.SetReal(from.z());
    to.SetImaginary(GfVec3d(from.x(), from.y(), from.z()));
}


template <class T>
void
GusdUT_Gf::Convert(const UT_QuaternionT<T>& from, GfQuatf& to)
{
    to.SetReal(from.z());
    to.SetImaginary(GfVec3f(from.x(), from.y(), from.z()));
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

#endif /*_GUSD_UT_GF_H_*/
