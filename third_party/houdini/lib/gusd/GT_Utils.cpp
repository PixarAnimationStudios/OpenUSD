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
#include "GT_Utils.h"
#include "UT_Gf.h"
#include "UT_Version.h"

#include <GA/GA_ATIGroupBool.h>
#include <GT/GT_DANumeric.h>
#include <GT/GT_GEOPrimPacked.h>
#include <GT/GT_PrimInstance.h>
#include <GT/GT_Util.h>
#include <SYS/SYS_Version.h>

#include "pxr/base/gf/vec3h.h"
#include "pxr/base/gf/vec4h.h"
#include "pxr/base/tf/span.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/boundable.h"
#include "pxr/usd/usdGeom/xformable.h"

#include <boost/tuple/tuple.hpp>

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

using std::cout;
using std::cerr;
using std::endl;
using std::set;
using std::string;

#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif

namespace {
//#############################################################################
// struct GtDataToUsdTypename
//#############################################################################
struct GtDataToUsdTypename
{
    typedef boost::tuple<GT_Storage,
                         GT_Type,
                         int /*tupleSize*/,
                         bool /*isArray*/> KeyType;

    struct equal_func : std::binary_function<KeyType, KeyType, bool>
    {
        bool operator()(const KeyType& lhs, const KeyType& rhs) const
        {
            return lhs.get<0>() == rhs.get<0>()
                && lhs.get<1>() == rhs.get<1>()
                && lhs.get<2>() == rhs.get<2>()
                && lhs.get<3>() == rhs.get<3>();
        }
    };

    struct hash_func : std::unary_function<KeyType, std::size_t>
    {
        std::size_t operator()(const KeyType& k) const
        {
            std::size_t seed = 0;
            boost::hash_combine(seed, k.get<0>());
            boost::hash_combine(seed, k.get<1>());
            boost::hash_combine(seed, k.get<2>());
            boost::hash_combine(seed, k.get<3>());
            return seed;
        }
    };

    typedef UT_Map<KeyType, SdfValueTypeName, hash_func, equal_func>  MapType;
    MapType m_typeLookup;

    GtDataToUsdTypename()
    {
        // Integral types

        DefineTypeLookup(GT_STORE_INT32, GT_TYPE_NONE, -1,
                         SdfValueTypeNames->Int);
        DefineTypeLookup(GT_STORE_INT64, GT_TYPE_NONE, -1,
                         SdfValueTypeNames->Int64);
        DefineTypeLookup(GT_STORE_UINT8, GT_TYPE_NONE, -1,
                         SdfValueTypeNames->UChar);

#if SYS_VERSION_FULL_INT >= 0x11000000
            // Up-cast int8/int16 to avoid precision loss.
            DefineTypeLookup(GT_STORE_INT8, GT_TYPE_NONE, -1,
                             SdfValueTypeNames->Int);
            DefineTypeLookup(GT_STORE_INT16, GT_TYPE_NONE, -1,
                             SdfValueTypeNames->Int);
#endif

        // Integral vectors.
        // USD only supports a single precision for vectors of integers.
#if SYS_VERSION_FULL_INT >= 0x11000000
        for (auto storage : {GT_STORE_UINT8, GT_STORE_INT8, GT_STORE_INT16,
                             GT_STORE_INT32, GT_STORE_INT64})
#else
        for (auto storage : {GT_STORE_UINT8, GT_STORE_INT32, GT_STORE_INT64})
#endif
        {
            DefineTypeLookup(storage, GT_TYPE_NONE, 2,
                             SdfValueTypeNames->Int2);
            DefineTypeLookup(storage, GT_TYPE_NONE, 3,
                             SdfValueTypeNames->Int3);
            DefineTypeLookup(storage, GT_TYPE_NONE, 4,
                             SdfValueTypeNames->Int4);
        }

        DefineTypeLookup(GT_STORE_REAL16, GT_TYPE_NONE, -1,
                         SdfValueTypeNames->Half);
        DefineTypeLookup(GT_STORE_REAL32, GT_TYPE_NONE, -1,
                         SdfValueTypeNames->Float);
        DefineTypeLookup(GT_STORE_REAL64, GT_TYPE_NONE, -1,
                         SdfValueTypeNames->Double);

        // Vec2
        DefineTypeLookup(GT_STORE_REAL16, GT_TYPE_NONE, 2,
                         SdfValueTypeNames->Half2);
        DefineTypeLookup(GT_STORE_REAL32, GT_TYPE_NONE, 2,
                         SdfValueTypeNames->Float2);
        DefineTypeLookup(GT_STORE_REAL64, GT_TYPE_NONE, 2,
                         SdfValueTypeNames->Double2);

        // GT_TYPE_TEXTURE
#if SYS_VERSION_FULL_INT >= 0x10050000
        DefineTypeLookup(GT_STORE_REAL16, GT_TYPE_TEXTURE, 2,
                         SdfValueTypeNames->TexCoord2h);
        DefineTypeLookup(GT_STORE_REAL32, GT_TYPE_TEXTURE, 2,
                         SdfValueTypeNames->TexCoord2f);
        DefineTypeLookup(GT_STORE_REAL64, GT_TYPE_TEXTURE, 2,
                         SdfValueTypeNames->TexCoord2d);
#endif

        // GT_TYPE_ST
        DefineTypeLookup(GT_STORE_REAL16, GT_TYPE_ST, 2,
                         SdfValueTypeNames->TexCoord2h);
        DefineTypeLookup(GT_STORE_REAL32, GT_TYPE_ST, 2,
                         SdfValueTypeNames->TexCoord2f);
        DefineTypeLookup(GT_STORE_REAL64, GT_TYPE_ST, 2,
                         SdfValueTypeNames->TexCoord2d);

        // Vec3
        DefineTypeLookup(GT_STORE_REAL16, GT_TYPE_NONE, 3,
                         SdfValueTypeNames->Half3);
        DefineTypeLookup(GT_STORE_REAL32, GT_TYPE_NONE, 3,
                         SdfValueTypeNames->Float3);
        DefineTypeLookup(GT_STORE_REAL64, GT_TYPE_NONE, 3,
                         SdfValueTypeNames->Double3);

        // GT_TYPE_VECTOR 3
        DefineTypeLookup(GT_STORE_REAL16, GT_TYPE_VECTOR, 3,
                         SdfValueTypeNames->Vector3h);
        DefineTypeLookup(GT_STORE_REAL32, GT_TYPE_VECTOR, 3,
                         SdfValueTypeNames->Vector3f);
        DefineTypeLookup(GT_STORE_REAL64, GT_TYPE_VECTOR, 3,
                         SdfValueTypeNames->Vector3d);

        // GT_TYPE_NORMAL 3
        DefineTypeLookup(GT_STORE_REAL16, GT_TYPE_NORMAL, 3,
                         SdfValueTypeNames->Normal3h);
        DefineTypeLookup(GT_STORE_REAL32, GT_TYPE_NORMAL, 3,
                         SdfValueTypeNames->Normal3f);
        DefineTypeLookup(GT_STORE_REAL64, GT_TYPE_NORMAL, 3,
                         SdfValueTypeNames->Normal3d);

        // GT_TYPE_COLOR 3
        DefineTypeLookup(GT_STORE_REAL16, GT_TYPE_COLOR, 3,
                         SdfValueTypeNames->Color3h);
        DefineTypeLookup(GT_STORE_REAL32, GT_TYPE_COLOR, 3,
                         SdfValueTypeNames->Color3f);
        DefineTypeLookup(GT_STORE_REAL64, GT_TYPE_COLOR, 3,
                         SdfValueTypeNames->Color3d);

        // GT_TYPE_POINT 3
        DefineTypeLookup(GT_STORE_REAL16, GT_TYPE_POINT, 3,
                         SdfValueTypeNames->Point3h);
        DefineTypeLookup(GT_STORE_REAL32, GT_TYPE_POINT, 3,
                         SdfValueTypeNames->Point3f);
        DefineTypeLookup(GT_STORE_REAL64, GT_TYPE_POINT, 3,
                         SdfValueTypeNames->Point3d);

        // GT_TYPE_TEXTURE 3
#if SYS_VERSION_FULL_INT >= 0x10050000
        DefineTypeLookup(GT_STORE_REAL16, GT_TYPE_TEXTURE, 3,
                         SdfValueTypeNames->TexCoord3h);
        DefineTypeLookup(GT_STORE_REAL32, GT_TYPE_TEXTURE, 3,
                         SdfValueTypeNames->TexCoord3f);
        DefineTypeLookup(GT_STORE_REAL64, GT_TYPE_TEXTURE, 3,
                         SdfValueTypeNames->TexCoord3d);
#endif

        // Vec4
        DefineTypeLookup(GT_STORE_REAL16, GT_TYPE_NONE, 4,
                         SdfValueTypeNames->Half4);
        DefineTypeLookup(GT_STORE_REAL32, GT_TYPE_NONE, 4,
                         SdfValueTypeNames->Float4);
        DefineTypeLookup(GT_STORE_REAL64, GT_TYPE_NONE, 4,
                         SdfValueTypeNames->Double4);

        // GT_TYPE_COLOR 4
        DefineTypeLookup(GT_STORE_REAL16, GT_TYPE_COLOR, 4,
                         SdfValueTypeNames->Color4h);
        DefineTypeLookup(GT_STORE_REAL32, GT_TYPE_COLOR, 4,
                         SdfValueTypeNames->Color4f);
        DefineTypeLookup(GT_STORE_REAL64, GT_TYPE_COLOR, 4,
                         SdfValueTypeNames->Color4d);

        // GT_TYPE_QUATERNION
        DefineTypeLookup(GT_STORE_REAL16, GT_TYPE_QUATERNION, 4,
                         SdfValueTypeNames->Quath);
        DefineTypeLookup(GT_STORE_REAL32, GT_TYPE_QUATERNION, 4,
                         SdfValueTypeNames->Quatf);
        DefineTypeLookup(GT_STORE_REAL64, GT_TYPE_QUATERNION, 4,
                         SdfValueTypeNames->Quatd);

        // Matrices.
        // USD only supports a single precision type for matrices.
        for (auto storage : {GT_STORE_REAL16,
                             GT_STORE_REAL32,
                             GT_STORE_REAL64}) {
            DefineTypeLookup(storage, GT_TYPE_MATRIX3, 9,
                             SdfValueTypeNames->Matrix3d);
            DefineTypeLookup(storage, GT_TYPE_MATRIX, 16,
                             SdfValueTypeNames->Matrix4d);
        }

        // String
        DefineTypeLookup(GT_STORE_STRING, GT_TYPE_NONE, -1,
                         SdfValueTypeNames->String);
    }

    SdfValueTypeName operator()(const GT_DataArrayHandle& gtData, bool isArray) const
    {
        GT_Size tupleSize = gtData->getTupleSize();
        // Types may be specialized for vectors of size 2,3,4 and matrices.
        // -1 means "any size"
        if(tupleSize != 2 && tupleSize != 3 && tupleSize != 4
           && tupleSize != 9 && tupleSize != 16) {
            tupleSize = -1;
        }
        KeyType key(gtData->getStorage(),
                    gtData->getTypeInfo(),
                    tupleSize,
                    isArray);
        MapType::const_iterator it = m_typeLookup.find(key);
        if(it != m_typeLookup.end()) {
            return it->second;
        }

        return SdfValueTypeName();
    }

    void
    DefineTypeLookup(GT_Storage storage, GT_Type type, int tupleSize,
                     const SdfValueTypeName& typeName)
    {
        // Scalar type
        m_typeLookup[KeyType(storage, type, tupleSize, /*array*/ false)] =
            typeName.GetScalarType();
        // Array type
        m_typeLookup[KeyType(storage, type, tupleSize, /*array*/ true)] =
            typeName.GetArrayType();
    }
};
//#############################################################################

//#############################################################################
// Converters
//#############################################################################


/// Copy \p count elements from \p src to \p dst.
template <typename FROM, typename TO>
void
_CopyArray(TO* dst, const FROM* src, GT_Size count)
{
    for (GT_Size i = 0; i < count; ++i) {
        dst[i] = static_cast<TO>(src[i]);
    }
}


bool _IsNumeric(GT_Storage storage)
{
    return GTisInteger(storage) || GTisFloat(storage);
}


/// Convert numeric GT types to USD.
/// This converter can only be used on types supported directly by the
/// GT_DataArray interface.
template <class UsdType>
struct _ConvertNumericToUsd
{
    using ScalarType = typename GusdPodTupleTraits<UsdType>::ValueType;
    static const int tupleSize = GusdGetTupleSize<UsdType>();

    static bool fillValue(UsdType& usdValue, const GT_DataArrayHandle& gtData)
    {
        UT_ASSERT_P(gtData);

        if (_IsNumeric(gtData->getStorage()) && gtData->entries() > 0 &&
            gtData->getTupleSize() == tupleSize) {

            auto* dst = reinterpret_cast<ScalarType*>(&usdValue);
            gtData->import(0, dst, tupleSize);
            return true;
        }
        return false;
    }

    static bool fillArray(VtArray<UsdType>& usdArray,
                          const GT_DataArrayHandle& gtData)
    {
        UT_ASSERT_P(gtData);

        if (_IsNumeric(gtData->getStorage()) &&
            gtData->getTupleSize() == tupleSize) {

            usdArray.resize(gtData->entries());
            auto* dst = reinterpret_cast<ScalarType*>(usdArray.data());
            gtData->fillArray(dst, 0, gtData->entries(), tupleSize);
            return true;
        }
        return false;
    }
};


/// Convert numeric GT types to USD.
/// This can be used on types that are not directly supported by GT_DataArray,
/// and which require a static_cast to copy into the output.
template <class UsdType>
struct _ConvertNumericWithCastToUsd
{
    using ScalarType = typename GusdPodTupleTraits<UsdType>::ValueType;
    static const int tupleSize = GusdGetTupleSize<UsdType>();

    static bool fillValue(UsdType& usdValue, const GT_DataArrayHandle& gtData)
    {
        UT_ASSERT_P(gtData);

        if (gtData->entries() > 0 && gtData->getTupleSize() == tupleSize) {
            const GT_Storage storage = gtData->getStorage();
            if (storage == GT_STORE_UINT8) {
                return _fillValue<uint8>(usdValue, gtData);
            }
#if SYS_VERSION_FULL_INT >= 0x11000000
            else if (storage == GT_STORE_INT8) {
                return _fillValue<int8>(usdValue, gtData);
            } else if (storage == GT_STORE_INT16) {
                return _fillValue<int16>(usdValue, gtData);
            }
#endif
            else if (storage == GT_STORE_INT32) {
                return _fillValue<int32>(usdValue, gtData);
            } else if (storage == GT_STORE_INT64) {
                return _fillValue<int64>(usdValue, gtData);
            } else if (storage == GT_STORE_REAL16) {
                return _fillValue<fpreal16>(usdValue, gtData);
            } else if (storage == GT_STORE_REAL32) {
                return _fillValue<fpreal32>(usdValue, gtData);
            } else if (storage == GT_STORE_REAL64) {
                return _fillValue<fpreal64>(usdValue, gtData);
            }
        }
        return false;
    }

    static bool fillArray(VtArray<UsdType>& usdArray,
                          const GT_DataArrayHandle& gtData)
    {
        UT_ASSERT_P(gtData);

        if (gtData->getTupleSize() == tupleSize) {
            const GT_Storage storage = gtData->getStorage();
            if (storage == GT_STORE_UINT8) {
                return _fillArray<uint8>(usdArray, gtData);
            }
#if SYS_VERSION_FULL_INT >= 0x11000000
            else if (storage == GT_STORE_INT8) {
                return _fillArray<int8>(usdArray, gtData);
            } else if (storage == GT_STORE_INT16) {
                return _fillArray<int16>(usdArray, gtData);
            }
#endif
            else if (storage == GT_STORE_INT32) {
                return _fillArray<int32>(usdArray, gtData);
            } else if (storage == GT_STORE_INT64) {
                return _fillArray<int64>(usdArray, gtData);
            } else if (storage == GT_STORE_REAL16) {
                return _fillArray<fpreal16>(usdArray, gtData);
            } else if (storage == GT_STORE_REAL32) {
                return _fillArray<fpreal32>(usdArray, gtData);
            } else if (storage == GT_STORE_REAL64) {
                return _fillArray<fpreal64>(usdArray, gtData);
            }
        }
        return false;
    }

private:
    template <typename GtType>
    static bool _fillValue(UsdType& usdValue,
                           const GT_DataArrayHandle& gtData,
                           GT_Offset offset=0)
    {   
        GtType src[tupleSize];
        gtData->import(offset, src, tupleSize);
        
        _CopyArray(reinterpret_cast<ScalarType*>(&usdValue), src, tupleSize);
        return true;
    }

    template <typename GtType>
    static bool _fillArray(VtArray<UsdType>& usdArray,
                           const GT_DataArrayHandle& gtData)
    {
        const GT_Size numElems = gtData->entries();
        usdArray.resize(numElems);
        auto dst = TfMakeSpan(usdArray);
        for (GT_Offset i = 0; i < numElems; ++i) {
            _fillValue<GtType>(dst[i], gtData, i);
        }
        return true;
    }
};


/// Converter specialized for converting GT data to GfQuat types.
template <class UsdType>
struct _ConvertQuatToUsd
{
    using GtScalarType = typename GusdPodTupleTraits<UsdType>::ValueType;

    static bool fillValue(UsdType& usdValue, const GT_DataArrayHandle& gtData)
    {
        UT_ASSERT_P(gtData);

        if (GTisFloat(gtData->getStorage()) &&
            gtData->entries() > 0 &&
            gtData->getTupleSize() == 4) {
            
            _fillValue(usdValue, gtData, 0);
            return true;
        }
        return false;
    }

    static bool fillArray(VtArray<UsdType>& usdArray,
                          const GT_DataArrayHandle& gtData)
    {
        UT_ASSERT_P(gtData);

        if (GTisFloat(gtData->getStorage()) && gtData->getTupleSize() == 4) {

            usdArray.resize(gtData->entries());
            auto dst = TfMakeSpan(usdArray);
            for (GT_Offset i = 0; i < gtData->entries(); ++i) {
                _fillValue(dst[i], gtData, i);
            }
            return true;
        }
        return false;
    }

private:
    static void _fillValue(UsdType& usdValue,
                           const GT_DataArrayHandle& gtData,
                           GT_Offset offset=0)
    {
        GtScalarType src[4];
        gtData->import(offset, src, 4);
        
        // Houdini quaternions are stored as i,j,k,w
        using UsdScalarType = typename UsdType::ScalarType;

        usdValue.SetReal(static_cast<UsdScalarType>(src[3]));
        usdValue.SetImaginary(static_cast<UsdScalarType>(src[0]),
                              static_cast<UsdScalarType>(src[1]),
                              static_cast<UsdScalarType>(src[2]));
    }
};


void _ConvertString(const GT_String& src, std::string* dst)
{
    *dst = std::string(src ? src : "");
}


void _ConvertString(const GT_String& src, TfToken* dst)
{
    *dst = TfToken(src ? src : "");
}


void _ConvertString(const GT_String& src, SdfAssetPath* dst)
{
    *dst = SdfAssetPath(src ? src : "");
}


/// Convert a GT type to a USD string value.
struct _ConvertStringToUsd
{
    static bool fillValue(std::string& usdValue, const GT_DataArrayHandle& gtData)
    {
        UT_ASSERT_P(gtData);

        if (GTisString(gtData->getStorage()) &&
            gtData->entries() > 0 &&
            gtData->getTupleSize() == 1) {

            _ConvertString(gtData->getS(0), &usdValue);
            return true;
        }
        return false;
    }

    static bool fillArray(VtStringArray& usdArray,
                          const GT_DataArrayHandle& gtData)
    {
        UT_ASSERT_P(gtData);

        if (GTisString(gtData->getStorage()) &&
            gtData->getTupleSize() == 1) {
            // XXX tuples of strings not supported
            usdArray.resize(gtData->entries());
            gtData->fillStrings(usdArray.data());
            return true;
        }
        return false;
    }
};


/// Convert a GT type to string-like types (eg., TfToken)
struct _ConvertStringLikeToUsd
{
    template <class UsdType>
    static bool fillValue(UsdType& usdValue, const GT_DataArrayHandle& gtData)
    {
        UT_ASSERT_P(gtData);

        if (GTisString(gtData->getStorage()) &&
            gtData->entries() > 0 &&
            gtData->getTupleSize() == 1) {

            _ConvertString(gtData->getS(0), &usdValue);
            return true;
        }
        return false;
    }

    template <class UsdType>
    static bool fillArray(VtArray<UsdType>& usdArray,
                          const GT_DataArrayHandle& gtData)
    {
        UT_ASSERT_P(gtData);

        if (GTisString(gtData->getStorage()) &&
            gtData->getTupleSize() == 1) {

            // XXX tuples of strings not supported
            const GT_Size numElems = gtData->entries();
            usdArray.resize(numElems);

            auto dst = TfMakeSpan(usdArray);
            for (GT_Size i = 0; i < numElems; ++i) {
                _ConvertString(gtData->getS(i), &dst[i]);
            }
            return true;
        }
        return false;
    }
};


template <class UsdType> struct _ConvertToUsd {};

#define _GUSD_DEFINE_CONVERTER(type, base)                      \
    template <> struct _ConvertToUsd<type> : public base {};

// Scalars
_GUSD_DEFINE_CONVERTER(double,  _ConvertNumericToUsd<double>);
_GUSD_DEFINE_CONVERTER(float,   _ConvertNumericToUsd<float>);
_GUSD_DEFINE_CONVERTER(GfHalf,  _ConvertNumericWithCastToUsd<GfHalf>);
_GUSD_DEFINE_CONVERTER(bool,    _ConvertNumericWithCastToUsd<bool>);
_GUSD_DEFINE_CONVERTER(int,     _ConvertNumericToUsd<int>);
_GUSD_DEFINE_CONVERTER(uint8,   _ConvertNumericToUsd<uint8>);
_GUSD_DEFINE_CONVERTER(int64,   _ConvertNumericToUsd<int64>);
_GUSD_DEFINE_CONVERTER(uint32,  _ConvertNumericWithCastToUsd<uint32>);
_GUSD_DEFINE_CONVERTER(uint64,  _ConvertNumericWithCastToUsd<uint64>);

// Vectors
_GUSD_DEFINE_CONVERTER(GfVec2d, _ConvertNumericToUsd<GfVec2d>);
_GUSD_DEFINE_CONVERTER(GfVec2f, _ConvertNumericToUsd<GfVec2f>);
_GUSD_DEFINE_CONVERTER(GfVec2h, _ConvertNumericToUsd<GfVec2h>);
_GUSD_DEFINE_CONVERTER(GfVec2i, _ConvertNumericToUsd<GfVec2i>);

_GUSD_DEFINE_CONVERTER(GfVec3d, _ConvertNumericToUsd<GfVec3d>);
_GUSD_DEFINE_CONVERTER(GfVec3f, _ConvertNumericToUsd<GfVec3f>);
_GUSD_DEFINE_CONVERTER(GfVec3h, _ConvertNumericToUsd<GfVec3h>);
_GUSD_DEFINE_CONVERTER(GfVec3i, _ConvertNumericToUsd<GfVec3i>);

_GUSD_DEFINE_CONVERTER(GfVec4d, _ConvertNumericToUsd<GfVec4d>);
_GUSD_DEFINE_CONVERTER(GfVec4f, _ConvertNumericToUsd<GfVec4f>);
_GUSD_DEFINE_CONVERTER(GfVec4h, _ConvertNumericToUsd<GfVec4h>);
_GUSD_DEFINE_CONVERTER(GfVec4i, _ConvertNumericToUsd<GfVec4i>);

// Quat types
_GUSD_DEFINE_CONVERTER(GfQuatd, _ConvertQuatToUsd<GfQuatd>);
_GUSD_DEFINE_CONVERTER(GfQuatf, _ConvertQuatToUsd<GfQuatf>);
_GUSD_DEFINE_CONVERTER(GfQuath, _ConvertQuatToUsd<GfQuath>);

// Matrices
_GUSD_DEFINE_CONVERTER(GfMatrix2d, _ConvertNumericToUsd<GfMatrix2d>);
_GUSD_DEFINE_CONVERTER(GfMatrix3d, _ConvertNumericToUsd<GfMatrix3d>);
_GUSD_DEFINE_CONVERTER(GfMatrix4d, _ConvertNumericToUsd<GfMatrix4d>);

// Strings a string-like types.
_GUSD_DEFINE_CONVERTER(std::string,  _ConvertStringToUsd);
_GUSD_DEFINE_CONVERTER(SdfAssetPath, _ConvertStringLikeToUsd);
_GUSD_DEFINE_CONVERTER(TfToken,      _ConvertStringLikeToUsd);


template <class UsdType>
bool _SetUsdAttributeT(const UsdAttribute& destAttr,
                       const GT_DataArrayHandle& sourceAttr,
                       const SdfValueTypeName& usdType,
                       UsdTimeCode time)
{
    UT_ASSERT(usdType);

    if (usdType.IsArray()) {
        VtArray<UsdType> usdArray;
        if (_ConvertToUsd<UsdType>::fillArray(usdArray, sourceAttr)) {
            return destAttr.Set(usdArray, time);
        }
    } else {
        UsdType usdValue;
        if (_ConvertToUsd<UsdType>::fillValue(usdValue, sourceAttr)) {
            return destAttr.Set(usdValue, time);
        }
    }
    return false;
}


bool
_SetUsdAttribute(const UsdAttribute& destAttr,
                 const GT_DataArrayHandle& sourceAttr,
                 const SdfValueTypeName& usdType,
                 UsdTimeCode time)
{
    if (!sourceAttr || !destAttr) {
        return false;
    }

    const SdfValueTypeName scalarType = usdType.GetScalarType();

    // GfVec3
    // GfVec3f is the most common type
    // XXX: We compare using the TfType rather than the Sdf type name so that
    // the same converters are employed regardless of the Sdf role.

    if (scalarType.GetType() == SdfValueTypeNames->Float3.GetType()) {
        return _SetUsdAttributeT<GfVec3f>(destAttr, sourceAttr, usdType, time);
    }
    if (scalarType.GetType() == SdfValueTypeNames->Double3.GetType()) {
        return _SetUsdAttributeT<GfVec3d>(destAttr, sourceAttr, usdType, time);
    }
    if (scalarType.GetType() == SdfValueTypeNames->Half3.GetType()) {
        return _SetUsdAttributeT<GfVec3h>(destAttr, sourceAttr, usdType, time);
    }

    // GfVec2
    if (scalarType.GetType() == SdfValueTypeNames->Double2.GetType()) {
        return _SetUsdAttributeT<GfVec2d>(destAttr, sourceAttr, usdType, time);
    }
    if (scalarType.GetType() == SdfValueTypeNames->Float2.GetType()) {
        return _SetUsdAttributeT<GfVec2f>(destAttr, sourceAttr, usdType, time);
    }
    if (scalarType.GetType() == SdfValueTypeNames->Half2.GetType()) {
        return _SetUsdAttributeT<GfVec2h>(destAttr, sourceAttr, usdType, time);
    }

    // GfVec4
    if (scalarType.GetType() == SdfValueTypeNames->Double4.GetType()) {
        return _SetUsdAttributeT<GfVec4d>(destAttr, sourceAttr, usdType, time);
    }
    if (scalarType.GetType() == SdfValueTypeNames->Float4.GetType()) {
        return _SetUsdAttributeT<GfVec4f>(destAttr, sourceAttr, usdType, time);
    }
    if (scalarType.GetType() == SdfValueTypeNames->Half4.GetType()) {
        return _SetUsdAttributeT<GfVec4h>(destAttr, sourceAttr, usdType, time);
    }

    // Quaternions
    if (scalarType == SdfValueTypeNames->Quatd) {
        return _SetUsdAttributeT<GfQuatd>(destAttr, sourceAttr, usdType, time);
    }
    if (scalarType == SdfValueTypeNames->Quatf) {
        return _SetUsdAttributeT<GfQuatf>(destAttr, sourceAttr, usdType, time);
    }
    if (scalarType == SdfValueTypeNames->Quath) {
        return _SetUsdAttributeT<GfQuath>(destAttr, sourceAttr, usdType, time);
    }

    // Scalars.
    if (scalarType == SdfValueTypeNames->Float) {
        return _SetUsdAttributeT<float>(destAttr, sourceAttr, usdType, time);
    }
    if (scalarType == SdfValueTypeNames->Double) {
        return _SetUsdAttributeT<double>(destAttr, sourceAttr, usdType, time);
    }
    if (scalarType == SdfValueTypeNames->Half) {
        return _SetUsdAttributeT<GfHalf>(destAttr, sourceAttr, usdType, time);
    }
    if (scalarType == SdfValueTypeNames->Int) {
        return _SetUsdAttributeT<int>(destAttr, sourceAttr, usdType, time);
    }
    if (scalarType == SdfValueTypeNames->Int64) {
        return _SetUsdAttributeT<int64>(destAttr, sourceAttr, usdType, time);
    }
    if (scalarType == SdfValueTypeNames->UChar) {
        return _SetUsdAttributeT<uint8>(destAttr, sourceAttr, usdType, time);
    }
    if (scalarType == SdfValueTypeNames->UInt) {
        return _SetUsdAttributeT<uint32>(destAttr, sourceAttr, usdType, time);
    }
    if (scalarType == SdfValueTypeNames->UInt64) {
        return _SetUsdAttributeT<uint64>(destAttr, sourceAttr, usdType, time);
    }

    // Matrices
    if (scalarType.GetType() == SdfValueTypeNames->Matrix2d.GetType()) {
        return _SetUsdAttributeT<GfMatrix2d>(destAttr, sourceAttr,
                                             usdType, time);
    }
    if (scalarType.GetType() == SdfValueTypeNames->Matrix3d.GetType()) {
        return _SetUsdAttributeT<GfMatrix3d>(destAttr, sourceAttr,
                                             usdType, time);
    }
    if (scalarType.GetType() == SdfValueTypeNames->Matrix4d.GetType()) {
        return _SetUsdAttributeT<GfMatrix4d>(destAttr, sourceAttr,
                                             usdType, time);
    }

    // Strings
    if (scalarType == SdfValueTypeNames->String) {
        return _SetUsdAttributeT<std::string>(destAttr, sourceAttr,
                                              usdType, time);
    }
    if (scalarType == SdfValueTypeNames->Token) {
        return _SetUsdAttributeT<TfToken>(destAttr, sourceAttr,
                                          usdType, time);
    }
    if (scalarType == SdfValueTypeNames->Asset) {
        return _SetUsdAttributeT<SdfAssetPath>(destAttr, sourceAttr,
                                               usdType, time);
    }

    TF_WARN("setUsdAttribute: type not implemented: %s",
            usdType.GetAsToken().GetText());
    return false;
}


//#############################################################################

bool
setPvSample(const UsdGeomImageable&     usdPrim,
            const TfToken&              name,
            const GT_DataArrayHandle&   gtData,
            const TfToken&              interpolationIn,
            UsdTimeCode                 time)
{
    static const GtDataToUsdTypename usdTypename;

    TfToken interpolation = interpolationIn;
    //bool isArrayType = (interpolation != UsdGeomTokens->constant);
    SdfValueTypeName typeName = usdTypename( gtData, true );
    if( !typeName ) {
        TF_WARN( "Unsupported primvar type %s, %s, tupleSize = %zd",
                 name.GetText(), GTstorage(gtData->getStorage()),
                 gtData->getTupleSize() );
        return false;
    }
    const UsdGeomPrimvar existingPrimvar = usdPrim.GetPrimvar( name );
    if( existingPrimvar && typeName != existingPrimvar.GetTypeName() ) {

        // If this primvar already exists, we can't change its type. Most notably,
        // we change change a scalar to an array type.
        typeName = existingPrimvar.GetTypeName();
        if( !typeName.IsArray() ) {
            interpolation = UsdGeomTokens->constant;
        }
    }

    UsdGeomPrimvar primvar = usdPrim.CreatePrimvar( name, typeName, interpolation );

    if(!primvar)
        return false;

    return _SetUsdAttribute(primvar.GetAttr(), gtData, typeName, time);
}

} // anon namespace


//#############################################################################
// class GusdGT_AttrFilter
//#############################################################################
GusdGT_AttrFilter::
GusdGT_AttrFilter(const std::string& pattern)
{
    // always override these
    m_overridePattern = 
        " ^__point_id"
        " ^__vertex_id"
        " ^__primitive_id"
        " ^__topology"
        " ^__primitivelist"
        " ^usdMeta_*"
        " ^usdvisible"
        " ^usdactive";

    setPattern(GT_OWNER_POINT, pattern);
    setPattern(GT_OWNER_VERTEX, pattern);
    setPattern(GT_OWNER_UNIFORM, pattern);
    setPattern(GT_OWNER_CONSTANT, pattern);
}

GusdGT_AttrFilter::
GusdGT_AttrFilter(const GusdGT_AttrFilter &rhs)
    : m_patterns( rhs.m_patterns )
    , m_activeOwners( rhs.m_activeOwners )
{
    m_overridePattern = 
        " ^__point_id"
        " ^__vertex_id"
        " ^__primitive_id"
        " ^__topology"
        " ^usdMeta_*"
        " ^usdvisible"
        " ^usdactive";
}

void GusdGT_AttrFilter::
setPattern(GT_Owner owner, const std::string& pattern)
{
    m_patterns[owner] = " " + pattern + m_overridePattern;
}

void GusdGT_AttrFilter::
appendPattern(GT_Owner owner, const std::string& pattern)
{
    m_patterns[owner] += " " + pattern;
}

void GusdGT_AttrFilter::
setActiveOwners(const OwnerArgs& owners) const
{
    m_activeOwners = owners;
}

bool GusdGT_AttrFilter::
matches(const std::string& attrName) const
{
    for(int i=0; i<m_activeOwners.entries(); ++i) {
        UT_Map<GT_Owner, std::string>::const_iterator mapIt = 
            m_patterns.find(m_activeOwners.item(i));
        if(mapIt == m_patterns.end()) continue;

        UT_String str(attrName); 
        if(str.multiMatch(mapIt->second.c_str()) != 0) {
            return true;
        }
    }
    return false;
}

//#############################################################################
// GusdGT_Utils implementation
//#############################################################################


GT_Type
GusdGT_Utils::getType(const SdfValueTypeName& typeName)
{
    const TfToken& role = typeName.GetRole();

    if (role == SdfValueRoleNames->Point) {
        return GT_TYPE_POINT;
    } else if (role == SdfValueRoleNames->Normal) {
        return GT_TYPE_NORMAL;
    } else if (role == SdfValueRoleNames->Vector) {
        return GT_TYPE_VECTOR;
    } else if (role == SdfValueRoleNames->Color) {
        return GT_TYPE_COLOR;
    } else if (role == SdfValueRoleNames->TextureCoordinate) {
#if SYS_VERSION_FULL_INT >= 0x10050000
        return GT_TYPE_TEXTURE;
#endif
    } else if (typeName == SdfValueTypeNames->Matrix4d) {
        return GT_TYPE_MATRIX;
    } else if (typeName == SdfValueTypeNames->Matrix3d) {
        return GT_TYPE_MATRIX3;
    }
    return GT_TYPE_NONE;
}


TfToken
GusdGT_Utils::getRole(GT_Type type)
{
    switch (type)
    {
    case GT_TYPE_POINT:  return SdfValueRoleNames->Point;
    case GT_TYPE_VECTOR: return SdfValueRoleNames->Vector;
    case GT_TYPE_NORMAL: return SdfValueRoleNames->Normal;
    case GT_TYPE_COLOR:  return SdfValueRoleNames->Color;
    case GT_TYPE_ST:
#if SYS_VERSION_FULL_INT >= 0x10050000
    case GT_TYPE_TEXTURE:
        return SdfValueRoleNames->TextureCoordinate;
#endif
    default:
        return TfToken();
    };
}


bool GusdGT_Utils::
setUsdAttribute(const UsdAttribute& destAttr,
                const GT_DataArrayHandle& sourceAttr,
                UsdTimeCode time)
{
    return _SetUsdAttribute(destAttr, sourceAttr, destAttr.GetTypeName(), time);
}


GT_DataArrayHandle GusdGT_Utils::
getExtentsArray(const GT_PrimitiveHandle& gtPrim)
{
    GT_DataArrayHandle ret;
    if(gtPrim) {
        UT_BoundingBox houBoundsArray[]
            = {UT_BoundingBox(SYS_FP32_MAX, SYS_FP32_MAX, SYS_FP32_MAX,
                              SYS_FP32_MIN, SYS_FP32_MIN, SYS_FP32_MIN)};
        houBoundsArray[0].initBounds();
        gtPrim->enlargeRenderBounds(houBoundsArray, 1);
        GT_Real32Array* gtExtents = new GT_Real32Array(2, 3);
        gtExtents->setTupleBlock(houBoundsArray[0].minvec().data(), 1, 0);
        gtExtents->setTupleBlock(houBoundsArray[0].maxvec().data(), 1, 1);
        ret = gtExtents;
    }
    return ret;
}

#if (GUSD_VER_CMP_1(>=,15))
typedef GT_AttributeMap::const_names_iterator   AttrMapIterator;
#else
typedef UT_SymbolTable::traverser               AttrMapIterator;
#endif


bool
GusdGT_Utils::setPrimvarSample( 
    const UsdGeomImageable& usdPrim, 
    const TfToken &name, 
    const GT_DataArrayHandle& data, 
    const TfToken& interpolation,
    UsdTimeCode time )
{
    DBG(cerr << "GusdGT_Utils::setPrimvarSample: " << name
             << ", " << GTstorage( data->getStorage() ) << ", "
              << data->getTupleSize() << ", " << interpolation << endl);
    
    return setPvSample(usdPrim, name, data, interpolation, time);
}

template <typename T> bool
isDataConst( const T* p, GT_Size entries, GT_Size tupleSize ) {

    if( tupleSize == 1 ) {
        T first = *p++;
        for( GT_Size i = 1; i < entries; ++i ) {
            if( *p++ != first ) {
                return false;
            }
        }
        return true;
    }
    else if ( tupleSize == 3 ) {
        T first_0 = *(p+0);
        T first_1 = *(p+1);
        T first_2 = *(p+2);
        p += 3;
        for( GT_Size i = 1; i < entries; ++i ) {
            if( *(p+0) != first_0  ||
                *(p+1) != first_1  ||
                *(p+2) != first_2 ) {
                return false;
            }
            p += 3;
        }
        return true;
    }
    else {
        const T* firstP = p;
        p += tupleSize;
        for( GT_Size i = 1; i < entries; ++i ) {
            for( GT_Size j = 0; j < tupleSize; ++ j ) {
                if( *(p + j) != *(firstP + j) ) {
                    return false;
                }
            }
            p += tupleSize;
        }
        return true;
    }
}

bool 
GusdGT_Utils::
isDataConstant( const GT_DataArrayHandle& data )
{
    const GT_Storage storage = data->getStorage();
    const GT_Size tupleSize = data->getTupleSize();
    const GT_Size entries = data->entries();

    if (entries == 0) {
        return true;
     }

    if (storage == GT_STORE_UINT8) {
        GT_DataArrayHandle buffer;
        return isDataConst<uint8>(data->getU8Array(buffer),
                                  entries, tupleSize);

    }  
#if SYS_VERSION_FULL_INT >= 0x11000000
    else if (storage == GT_STORE_INT8) {
        GT_DataArrayHandle buffer;
        return isDataConst<int8>(data->getI8Array(buffer),
                                 entries, tupleSize);
    } else if (storage == GT_STORE_INT16) {
        GT_DataArrayHandle buffer;
        return isDataConst<int16>(data->getI16Array(buffer),
                                  entries, tupleSize);
    }
#endif
    else if (storage == GT_STORE_INT32) {
        GT_DataArrayHandle buffer;
        return isDataConst<int32>(data->getI32Array(buffer),
                                  entries, tupleSize);

    }  else if (storage == GT_STORE_INT64) {
        GT_DataArrayHandle buffer;
        return isDataConst<int64>(data->getI64Array(buffer),
                                  entries, tupleSize);

    }  else if (storage == GT_STORE_REAL16) {
        GT_DataArrayHandle buffer;
        return isDataConst<fpreal16>(data->getF16Array(buffer),
                                     entries, tupleSize);

    } else if (storage == GT_STORE_REAL32) {
        GT_DataArrayHandle buffer;
        return isDataConst<fpreal32>(data->getF32Array(buffer),
                                     entries, tupleSize);

    } else if (storage == GT_STORE_REAL64) {
        GT_DataArrayHandle buffer;
        return isDataConst<fpreal64>(data->getF64Array(buffer),
                                     entries, tupleSize);
    } else if (storage == GT_STORE_STRING) {
        if (data->getStringIndexCount() >= 0) {
            // If this is an indexed string array, we can just compare the indices.
            // One would think that getIndexedStrings would return indices,
            // but it doesn't. If we look at the header file for GT_DAIndexedString,
            // we can deduce that getI32Array gets you the indices.
            GT_DataArrayHandle buffer;
            const int32* indices = data->getI32Array(buffer);
            if (indices ) {
                int32 first = indices[0];
                for (GT_Size i = 1; i < entries; ++i) {
                    if (indices[i] != first) {
                        return false;
                    }
                }
                return true;
            }
        }

        UT_StringArray strings;
        data->getStrings(strings);

        // beware of arrays of strings, I don't how to compare these.
        if (strings.entries() == 0) {
            return false;
        }

        const UT_StringHolder &first = strings(0);
        for (GT_Size i = 1, end = std::min( entries, strings.entries() ); i < end; ++i) {
            if (strings(i) != first) {
                return false;
            }
        }
        return true;
    }
    TF_WARN("Unsupported primvar type: %s, tupleSize = %zd", 
            GTstorage(storage), tupleSize);
    return false;
}

void GusdGT_Utils::
setCustomAttributesFromGTPrim(
    const UsdGeomImageable &usdGeomPrim,
    const GT_AttributeListHandle& gtAttrs,
    set<string>& excludeSet,
    UsdTimeCode time )
{
    //TODO: The exclude set should be a GT_GEOAttributeFilter

    static const GtDataToUsdTypename usdTypename;

    UsdPrim prim = usdGeomPrim.GetPrim();
    if( !gtAttrs )
        return;

    const GT_AttributeMapHandle attrMapHandle = gtAttrs->getMap();
    for(AttrMapIterator mapIt=attrMapHandle->begin(); !mapIt.atEnd(); ++mapIt) {

#if SYS_VERSION_FULL_INT < 0x11000000
        string name = mapIt.name();
#else
        string name = mapIt->first.toStdString();
#endif

#if (GUSD_VER_CMP_1(>=,15))
        const int attrIndex = attrMapHandle->get(name);
#else
        const int attrIndex = attrMapHandle->getMapIndex(mapIt.thing());
#endif
        const GT_DataArrayHandle& gtData = gtAttrs->get(attrIndex);

        if( TfStringStartsWith( name, "__" ) || 
            excludeSet.find( name ) != excludeSet.end() ) {
            continue;
        }

        const SdfValueTypeName typeName = usdTypename( gtData, false );

        UsdAttribute attr = 
            prim.CreateAttribute( TfToken( name ), typeName );

        setUsdAttribute( attr, gtData, time );
    }
}




GT_DataArrayHandle GusdGT_Utils::
getTransformArray(const GT_PrimitiveHandle& gtPrim)
{
    GT_DataArrayHandle ret;
    if(gtPrim) {
        UT_Matrix4D houXform;
        gtPrim->getPrimitiveTransform()->getMatrix(houXform);
        GT_Real64Array* gtData = new GT_Real64Array(houXform.data(), 1, 16);
        ret = gtData;
    }
    return ret;
}


GT_DataArrayHandle GusdGT_Utils::
getPackedTransformArray(const GT_PrimitiveHandle& gtPrim)
{
    GT_DataArrayHandle ret;
    if(gtPrim) {
        const GT_GEOPrimPacked* gtPacked
            = dynamic_cast<const GT_GEOPrimPacked*>(gtPrim.get());
        if(gtPacked) {
            UT_Matrix4D houXform;
            gtPacked->getPrim()->getFullTransform4(houXform);
            GT_Real64Array* gtData = new GT_Real64Array(houXform.data(), 1, 16);
            ret = gtData;
        }
    }
    return ret;
}


bool GusdGT_Utils::
setTransformFromGTArray(const UsdGeomXformable& usdGeom,
                        const GT_DataArrayHandle& xform,
                        const TransformLevel transformLevel,
                        UsdTimeCode time)
{
    if(!usdGeom || !xform) return false;

    bool resetsXformStack=false;
    std::vector<UsdGeomXformOp> xformOps = usdGeom.GetOrderedXformOps(
        &resetsXformStack);
    if(xformOps.size() <= transformLevel) return false;

    GfMatrix4d mat4;
    if (_ConvertToUsd<GfMatrix4d>::fillValue(mat4, xform)) {
        return xformOps[transformLevel].Set(mat4, time);
    }
    return false;
}

GfMatrix4d GusdGT_Utils::
getMatrixFromGTArray(const GT_DataArrayHandle& xform)
{
    GfMatrix4d mat4;
    if (!_ConvertToUsd<GfMatrix4d>::fillValue(mat4, xform)) {
        mat4.SetIdentity();
    }
    return mat4;
}
        
GT_DataArrayHandle GusdGT_Utils::
transformPoints( 
    GT_DataArrayHandle pts, 
    const UT_Matrix4D& objXform )
{
    GT_Real32Array *newPts = 
        new GT_Real32Array( pts->entries(), 3, pts->getTypeInfo() );
    UT_Vector3F* dstP = reinterpret_cast<UT_Vector3F*>(newPts->data());
            
    GT_DataArrayHandle buffer;
    const UT_Vector3F* srcP = 
        reinterpret_cast<const UT_Vector3F *>(pts->getF32Array( buffer ));
    for( GT_Size i = 0; i < pts->entries(); ++i ) {
        *dstP++ = *srcP++ * objXform;
    }
    return newPts;
}

GT_DataArrayHandle GusdGT_Utils::
transformPoints( 
    GT_DataArrayHandle pts, 
    const GfMatrix4d& objXform )
{
    return transformPoints( pts, GusdUT_Gf::Cast( objXform ) );
}
//#############################################################################

GT_AttributeListHandle GusdGT_Utils::
getAttributesFromPrim( const GEO_Primitive *prim )
{
    const GA_Detail& detail = prim->getDetail();
    GA_Offset offset = prim->getMapOffset();
    GA_Range range = GA_Range( detail.getPrimitiveMap(), offset, offset + 1 );
    const GA_AttributeDict& attrDict = detail.getAttributeDict(GA_ATTRIB_PRIMITIVE);
    if( attrDict.entries() == 0 )
        return GT_AttributeListHandle();

    GT_AttributeListHandle attrList = new GT_AttributeList(new GT_AttributeMap());
    for( GA_AttributeDict::iterator it=attrDict.begin(); !it.atEnd(); ++it)
    {
        GA_Attribute *attr = it.attrib();
        // Ignore any attributes which define groups.
        if( attr && !GA_ATIGroupBool::isType( attr ))
        {
            GT_DataArrayHandle array = GT_Util::extractAttribute( *attr, range );
            attrList = attrList->addAttribute( attr->getName(), array, true );
        }
    }
    return attrList;
}

std::string GusdGT_Utils::
makeValidIdentifier(const TfToken& usdFilePath, const SdfPath& nodePath)
{
    return TfMakeValidIdentifier(usdFilePath)
            + "__" + TfMakeValidIdentifier(nodePath.GetString());
}

PXR_NAMESPACE_CLOSE_SCOPE
