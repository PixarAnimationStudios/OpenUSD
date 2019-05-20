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

#include <GT/GT_DANumeric.h>
#include <GT/GT_GEOPrimPacked.h>
#include <GT/GT_PrimInstance.h>
#include <GT/GT_Util.h>
#include <GA/GA_ATIGroupBool.h>
#include <SYS/SYS_Version.h>

#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/gf/vec3h.h>
#include <pxr/base/gf/vec4h.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/boundable.h>
#include <pxr/usd/usdGeom/xformable.h>

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
        // Int
        m_typeLookup[KeyType(GT_STORE_INT32, GT_TYPE_NONE, -1, false)]
            = SdfValueTypeNames->Int;
        m_typeLookup[KeyType(GT_STORE_INT32, GT_TYPE_NONE, -1, true)]
            = SdfValueTypeNames->IntArray;
        // Int64
        m_typeLookup[KeyType(GT_STORE_INT64, GT_TYPE_NONE, -1, false)]
            = SdfValueTypeNames->Int64;
        m_typeLookup[KeyType(GT_STORE_INT64, GT_TYPE_NONE, -1, true)]
            = SdfValueTypeNames->Int64Array;
        // Vec3i
        m_typeLookup[KeyType(GT_STORE_INT32, GT_TYPE_VECTOR, 3, false)]
            = SdfValueTypeNames->Int3;
        m_typeLookup[KeyType(GT_STORE_INT32, GT_TYPE_VECTOR, 3, true)]
            = SdfValueTypeNames->Int3Array;
        m_typeLookup[KeyType(GT_STORE_INT32, GT_TYPE_NONE, 3, false)]
            = SdfValueTypeNames->Int3;
        m_typeLookup[KeyType(GT_STORE_INT32, GT_TYPE_NONE, 3, true)]
            = SdfValueTypeNames->Int3Array;
        // Vec4i
        m_typeLookup[KeyType(GT_STORE_INT32, GT_TYPE_NONE, 4, false)]
            = SdfValueTypeNames->Int4;
        m_typeLookup[KeyType(GT_STORE_INT32, GT_TYPE_NONE, 4, true)]
            = SdfValueTypeNames->Int4Array;
        // Float
        m_typeLookup[KeyType(GT_STORE_REAL32, GT_TYPE_NONE, -1, false)]
            = SdfValueTypeNames->Float;
        m_typeLookup[KeyType(GT_STORE_REAL32, GT_TYPE_NONE, -1, true)]
            = SdfValueTypeNames->FloatArray;
        // Vec2f
        m_typeLookup[KeyType(GT_STORE_REAL32, GT_TYPE_NONE, 2, false)]
            = SdfValueTypeNames->Float2;
        m_typeLookup[KeyType(GT_STORE_REAL32, GT_TYPE_NONE, 2, true)]
            = SdfValueTypeNames->Float2Array;
        // Vec3f
        m_typeLookup[KeyType(GT_STORE_REAL32, GT_TYPE_NONE, 3, false)]
            = SdfValueTypeNames->Float3;
        m_typeLookup[KeyType(GT_STORE_REAL32, GT_TYPE_NONE, 3, true)]
            = SdfValueTypeNames->Float3Array;
        // VectorFloat
        m_typeLookup[KeyType(GT_STORE_REAL32, GT_TYPE_VECTOR, 3, false)]
            = SdfValueTypeNames->Vector3f;
        m_typeLookup[KeyType(GT_STORE_REAL32, GT_TYPE_VECTOR, 3, true)]
            = SdfValueTypeNames->Vector3fArray;
        // NormalFloat
        m_typeLookup[KeyType(GT_STORE_REAL32, GT_TYPE_NORMAL, 3, false)]
            = SdfValueTypeNames->Normal3f;
        m_typeLookup[KeyType(GT_STORE_REAL32, GT_TYPE_NORMAL, 3, true)]
            = SdfValueTypeNames->Normal3fArray;
        // ColorFloat
        m_typeLookup[KeyType(GT_STORE_REAL32, GT_TYPE_COLOR, 3, false)]
            = SdfValueTypeNames->Color3f;
        m_typeLookup[KeyType(GT_STORE_REAL32, GT_TYPE_COLOR, 3, true)]
            = SdfValueTypeNames->Color3fArray;
        // PointFloat
        m_typeLookup[KeyType(GT_STORE_REAL32, GT_TYPE_POINT, 3, false)]
            = SdfValueTypeNames->Point3f;
        m_typeLookup[KeyType(GT_STORE_REAL32, GT_TYPE_POINT, 3, true)]
            = SdfValueTypeNames->Point3fArray;
        // Vec4f
        m_typeLookup[KeyType(GT_STORE_REAL32, GT_TYPE_NONE, 4, false)]
            = SdfValueTypeNames->Float4;
        m_typeLookup[KeyType(GT_STORE_REAL32, GT_TYPE_NONE, 4, true)]
            = SdfValueTypeNames->Float4Array;
        m_typeLookup[KeyType(GT_STORE_REAL32, GT_TYPE_QUATERNION, 4, false)]
            = SdfValueTypeNames->Float4;
        m_typeLookup[KeyType(GT_STORE_REAL32, GT_TYPE_QUATERNION, 4, true)]
            = SdfValueTypeNames->Float4Array;
        // String
        m_typeLookup[KeyType(GT_STORE_STRING, GT_TYPE_NONE, -1, false)]
            = SdfValueTypeNames->String;
        m_typeLookup[KeyType(GT_STORE_STRING, GT_TYPE_NONE, -1, true)]
            = SdfValueTypeNames->StringArray;
        // Half
        m_typeLookup[KeyType(GT_STORE_REAL16, GT_TYPE_NONE, -1, false)]
            = SdfValueTypeNames->Half;
        m_typeLookup[KeyType(GT_STORE_REAL16, GT_TYPE_NONE, -1, true)]
            = SdfValueTypeNames->HalfArray;
        // Vec3h
        m_typeLookup[KeyType(GT_STORE_REAL16, GT_TYPE_NONE, 3, false)]
            = SdfValueTypeNames->Half3;
        m_typeLookup[KeyType(GT_STORE_REAL16, GT_TYPE_NONE, 3, true)]
            = SdfValueTypeNames->Half3Array;
        // Vec4h
        m_typeLookup[KeyType(GT_STORE_REAL16, GT_TYPE_NONE, 4, false)]
            = SdfValueTypeNames->Half4;
        m_typeLookup[KeyType(GT_STORE_REAL16, GT_TYPE_NONE, 4, true)]
            = SdfValueTypeNames->Half4Array;
        m_typeLookup[KeyType(GT_STORE_REAL16, GT_TYPE_QUATERNION, 4, false)]
            = SdfValueTypeNames->Half4;
        m_typeLookup[KeyType(GT_STORE_REAL16, GT_TYPE_QUATERNION, 4, true)]
            = SdfValueTypeNames->Half4Array;

    }

    SdfValueTypeName operator()(const GT_DataArrayHandle& gtData, bool isArray) const
    {
        GT_Size tupleSize = gtData->getTupleSize();
        // Types may be specialized for 3- and 4-vectors.
        // -1 means "any size"
        if(tupleSize != 2 && tupleSize != 3 && tupleSize != 4) {
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
};
//#############################################################################

//#############################################################################
// struct TypeConvertTraits
//#############################################################################
template <typename T_USD> struct TypeConvertTraits;

template <> struct TypeConvertTraits<int>
{
    enum {GT_Storage = GT_STORE_INT32};
    typedef int Type;
    typedef VtIntArray ArrayType;
    
    static bool fillValue(Type& usdValue, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->entries() < 1) return false;
        usdValue = gtData->getI32(0);

        return true;
    }

    static bool fillArray(ArrayType& usdArray, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        const int numElements = gtData->entries() * gtData->getTupleSize();
        GT_DataArrayHandle buffer;
        const int* flatArray = gtData->getI32Array(buffer);
        usdArray.resize(numElements);
        for(int i=0; i<numElements; ++i)
            usdArray[i] = flatArray[i];
        
        return true;
    }
};


// XXX This needs to be updated once USD supports int64
template <> struct TypeConvertTraits<int64_t>
{
    enum {GT_Storage = GT_STORE_INT64};
    typedef int64_t Type;
    typedef VtIntArray ArrayType;
    
    static bool fillValue(Type& usdValue, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->entries() < 1) return false;
        usdValue = static_cast<int>(gtData->getI64(0));

        return true;
    }

    static bool fillArray(ArrayType& usdArray, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        const int numElements = gtData->entries() * gtData->getTupleSize();
        GT_DataArrayHandle buffer;
        const int64* flatArray = gtData->getI64Array(buffer);
        usdArray.resize(numElements);
        for(int i=0; i<numElements; ++i)
            usdArray[i] = static_cast<int>(flatArray[i]);
        
        return true;
    }
};

template <> struct TypeConvertTraits<GfHalf>
{
    enum {GT_Storage = GT_STORE_REAL16};
    typedef GfHalf Type;
    typedef VtHalfArray ArrayType;
    
    static bool fillValue(Type& usdValue, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->entries() < 1) return false;
        usdValue = gtData->getF16(0);

        return true;
    }

    static bool fillArray(ArrayType& usdArray, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        const int numElements = gtData->entries() * gtData->getTupleSize();
        GT_DataArrayHandle buffer;
        const fpreal16* flatArray = gtData->getF16Array(buffer);
        usdArray.resize(numElements);
        for(int i=0; i<numElements; ++i)
            usdArray[i] = flatArray[i];
        
        return true;
    }
};

template <> struct TypeConvertTraits<GfVec3h>
{
    enum {GT_Storage = GT_STORE_REAL16};
    typedef GfVec3h Type;
    typedef VtVec3hArray ArrayType;
    
    static bool fillValue(Type& usdValue, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->entries() < 1) return false;
        usdValue.Set(static_cast<GfHalf>(gtData->getF32(0,0)),
                     static_cast<GfHalf>(gtData->getF32(0,1)),
                     static_cast<GfHalf>(gtData->getF32(0,2)));

        return true;
    }

    static bool fillArray(ArrayType& usdArray, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->getTupleSize() != 3) return false;
        const int numElements = gtData->entries();
        GT_DataArrayHandle buffer;
        const fpreal16* flatArray = gtData->getF16Array(buffer);
        usdArray.resize(numElements);
        for(int i=0; i<numElements; ++i)
            usdArray[i].Set(static_cast<GfHalf>(flatArray[i*3]),
                            static_cast<GfHalf>(flatArray[i*3+1]),
                            static_cast<GfHalf>(flatArray[i*3+2]));
        
        return true;
    }
};


template <> struct TypeConvertTraits<GfVec4h>
{
    enum {GT_Storage = GT_STORE_REAL16};
    typedef GfVec4h Type;
    typedef VtVec4hArray ArrayType;
    
    static bool fillValue(Type& usdValue, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->entries() < 1) return false;
        usdValue.Set(static_cast<GfHalf>(gtData->getF32(0,0)),
                     static_cast<GfHalf>(gtData->getF32(0,1)),
                     static_cast<GfHalf>(gtData->getF32(0,2)),
                     static_cast<GfHalf>(gtData->getF32(0,3)));

        return true;
    }

    static bool fillArray(ArrayType& usdArray, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->getTupleSize() != 4) return false;
        const int numElements = gtData->entries();
        GT_DataArrayHandle buffer;
        const fpreal16* flatArray = gtData->getF16Array(buffer);
        usdArray.resize(numElements);
        for(int i=0; i<numElements; ++i)
            usdArray[i].Set(static_cast<GfHalf>(flatArray[i*4]),
                            static_cast<GfHalf>(flatArray[i*4+1]),
                            static_cast<GfHalf>(flatArray[i*4+2]),
                            static_cast<GfHalf>(flatArray[i*4+3]));
        
        return true;
    }
};

template <> struct TypeConvertTraits<float>
{
    enum {GT_Storage = GT_STORE_REAL32};
    typedef float Type;
    typedef VtFloatArray ArrayType;
    
    static bool fillValue(Type& usdValue, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->entries() < 1) return false;
        usdValue = gtData->getF32(0);

        return true;
    }

    static bool fillArray(ArrayType& usdArray, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        const int numElements = gtData->entries() * gtData->getTupleSize();
        GT_DataArrayHandle buffer;
        const float* flatArray = gtData->getF32Array(buffer);
        usdArray.resize(numElements);
        for(int i=0; i<numElements; ++i)
            usdArray[i] = flatArray[i];
        
        return true;
    }
};

template <> struct TypeConvertTraits<double>
{
    enum {GT_Storage = GT_STORE_REAL64};
    typedef double Type;
    typedef VtDoubleArray ArrayType;
    
    static bool fillValue(Type& usdValue, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->entries() < 1) return false;
        usdValue = gtData->getF64(0);

        return true;
    }

    static bool fillArray(ArrayType& usdArray, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage) {
            cerr << "storage type does not match" << endl;
            return false;
        }
        const int numElements = gtData->entries() * gtData->getTupleSize();
        GT_DataArrayHandle buffer;
        const double* flatArray = gtData->getF64Array(buffer);
        usdArray.resize(numElements);
        for(int i=0; i<numElements; ++i)
            usdArray[i] = flatArray[i];
        
        return true;
    }
};

template <> struct TypeConvertTraits<bool>
{
    enum {GT_Storage = GT_STORE_UINT8};
    typedef int Type;
    typedef VtBoolArray ArrayType;
    
    static bool fillValue(Type& usdValue, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->entries() < 1) return false;
        usdValue = gtData->getU8(0);

        return true;
    }

    static bool fillArray(ArrayType& usdArray, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        const int numElements = gtData->entries() * gtData->getTupleSize();
        GT_DataArrayHandle buffer;
        const uint8* flatArray = gtData->getU8Array(buffer);
        usdArray.resize(numElements);
        for(int i=0; i<numElements; ++i)
            usdArray[i] = flatArray[i];
        
        return true;
    }
};


template <> struct TypeConvertTraits<GfVec3i>
{
    enum {GT_Storage = GT_STORE_INT32};
    typedef GfVec3i Type;
    typedef VtVec3iArray ArrayType;
    
    static bool fillValue(Type& usdValue, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->entries() < 1) return false;
        usdValue.Set(gtData->getI32(0,0),
                     gtData->getI32(0,1),
                     gtData->getI32(0,2));

        return true;
    }

    static bool fillArray(ArrayType& usdArray, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->getTupleSize() != 3) return false;
        const int numElements = gtData->entries();
        GT_DataArrayHandle buffer;
        const int* flatArray = gtData->getI32Array(buffer);
        usdArray.resize(numElements);
        for(int i=0; i<numElements; ++i)
            usdArray[i].Set(flatArray[i*3],
                            flatArray[i*3+1],
                            flatArray[i*3+2]);
        
        return true;
    }
};


template <> struct TypeConvertTraits<GfVec4i>
{
    enum {GT_Storage = GT_STORE_INT32};
    typedef GfVec4i Type;
    typedef VtVec4iArray ArrayType;
    
    static bool fillValue(Type& usdValue, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->entries() < 1) return false;
        usdValue.Set(gtData->getI32(0,0),
                     gtData->getI32(0,1),
                     gtData->getI32(0,2),
                     gtData->getI32(0,3));

        return true;
    }

    static bool fillArray(ArrayType& usdArray, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->getTupleSize() != 4) return false;
        const int numElements = gtData->entries();
        GT_DataArrayHandle buffer;
        const int* flatArray = gtData->getI32Array(buffer);
        usdArray.resize(numElements);
        for(int i=0; i<numElements; ++i)
            usdArray[i].Set(flatArray[i*3],
                            flatArray[i*3+1],
                            flatArray[i*3+2],
                            flatArray[i*3+3]);
        
        return true;
    }
};

template <> struct TypeConvertTraits<GfVec2f>
{
    enum {GT_Storage = GT_STORE_REAL32};
    typedef GfVec2f Type;
    typedef VtVec2fArray ArrayType;
    
    static bool fillValue(Type& usdValue, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->entries() < 1) return false;
        usdValue.Set(gtData->getF32(0,0),
                     gtData->getF32(0,1));

        return true;
    }

    static bool fillArray(ArrayType& usdArray, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->getTupleSize() != 2) return false;
        const int numElements = gtData->entries();
        GT_DataArrayHandle buffer;
        const float* flatArray = gtData->getF32Array(buffer);
        usdArray.resize(numElements);
        for(int i=0; i<numElements; ++i)
            usdArray[i].Set(flatArray[i*2],
                            flatArray[i*2+1]);
        
        return true;
    }
};

template <> struct TypeConvertTraits<GfVec3f>
{
    enum {GT_Storage = GT_STORE_REAL32};
    typedef GfVec3f Type;
    typedef VtVec3fArray ArrayType;
    
    static bool fillValue(Type& usdValue, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->entries() < 1) return false;
        usdValue.Set(gtData->getF32(0,0),
                     gtData->getF32(0,1),
                     gtData->getF32(0,2));

        return true;
    }

    static bool fillArray(ArrayType& usdArray, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->getTupleSize() != 3) return false;
        const int numElements = gtData->entries();
        GT_DataArrayHandle buffer;
        const float* flatArray = gtData->getF32Array(buffer);
        usdArray.resize(numElements);
        for(int i=0; i<numElements; ++i)
            usdArray[i].Set(flatArray[i*3],
                            flatArray[i*3+1],
                            flatArray[i*3+2]);
        
        return true;
    }
};


template <> struct TypeConvertTraits<GfVec4f>
{
    enum {GT_Storage = GT_STORE_REAL32};
    typedef GfVec4f Type;
    typedef VtVec4fArray ArrayType;
    
    static bool fillValue(Type& usdValue, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->entries() < 1) return false;
        usdValue.Set(gtData->getF32(0,0),
                     gtData->getF32(0,1),
                     gtData->getF32(0,2),
                     gtData->getF32(0,3));

        return true;
    }

    static bool fillArray(ArrayType& usdArray, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->getTupleSize() != 4) return false;
        const int numElements = gtData->entries();
        GT_DataArrayHandle buffer;
        const float* flatArray = gtData->getF32Array(buffer);
        usdArray.resize(numElements);
        for(int i=0; i<numElements; ++i)
            usdArray[i].Set(flatArray[i*4],
                            flatArray[i*4+1],
                            flatArray[i*4+2],
                            flatArray[i*4+3]);
        
        return true;
    }
};

template <> struct TypeConvertTraits<GfQuath>
{
    enum {GT_Storage = GT_STORE_REAL32};
    typedef GfQuath Type;
    typedef VtQuathArray ArrayType;
    
    static bool fillValue(Type& usdValue, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->entries() < 1) return false;

        // Houdini quaternions are i,j,k,w, while Gf is w,i,j,k
        usdValue.SetReal(static_cast<GfHalf>(gtData->getF32(0,3)));
        usdValue.SetImaginary(static_cast<GfHalf>(gtData->getF32(0,0)),
                              static_cast<GfHalf>(gtData->getF32(0,1)),
                              static_cast<GfHalf>(gtData->getF32(0,2)));

        return true;
    }

    static bool fillArray(ArrayType& usdArray, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->getTupleSize() != 4) return false;
        const int numElements = gtData->entries();
        GT_DataArrayHandle buffer;
        const float* flatArray = gtData->getF32Array(buffer);
        usdArray.resize(numElements);
        for(int i=0; i<numElements; ++i)
        {
            // Houdini quaternions are i,j,k,w, while Gf is w,i,j,k
            usdArray[i].SetReal(static_cast<GfHalf>(flatArray[i*4+3]));
            usdArray[i].SetImaginary(static_cast<GfHalf>(flatArray[i*4]),
                                     static_cast<GfHalf>(flatArray[i*4+1]),
                                     static_cast<GfHalf>(flatArray[i*4+2]));
        }
        
        return true;
    }
};

template <> struct TypeConvertTraits<GfMatrix4d>
{
    enum {GT_Storage = GT_STORE_REAL64};
    typedef GfMatrix4d Type;
    typedef VtMatrix4dArray ArrayType;
    
    static bool fillValue(Type& usdValue, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->entries() < 1) return false;
        GT_DataArrayHandle buffer;
        const double* flatArray = gtData->getF64Array(buffer);
        memcpy(usdValue[0], flatArray, sizeof(double)*4);
        memcpy(usdValue[1], flatArray+4, sizeof(double)*4);
        memcpy(usdValue[2], flatArray+8, sizeof(double)*4);
        memcpy(usdValue[3], flatArray+12, sizeof(double)*4);

        return true;
    }

    static bool fillArray(ArrayType& usdArray, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->getTupleSize() != 16) return false;
        const int numElements = gtData->entries();
        GT_DataArrayHandle buffer;
        usdArray.resize(numElements);
        // TODO this might not be correct
        for(int i=0; i<numElements; ++i) {
            gtData->fillArray(usdArray[i][0], i*16,    1, 16);
            gtData->fillArray(usdArray[i][1], i*16+4,  1, 16);
            gtData->fillArray(usdArray[i][2], i*16+8,  1, 16);
            gtData->fillArray(usdArray[i][3], i*16+12, 1, 16);
        }
        
        return true;
    }
};


template <> struct TypeConvertTraits<std::string>
{
    enum {GT_Storage = GT_STORE_STRING};
    typedef std::string Type;
    typedef VtStringArray ArrayType;
    
    static bool fillValue(Type& usdValue, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        if(gtData->entries() < 1) return false;
        GT_String str = gtData->getS(0); 
        if(str == NULL) return false;

        usdValue = str;
        return true;
    }

    static bool fillArray(ArrayType& usdArray, const GT_DataArrayHandle& gtData)
    {
        if((int)gtData->getStorage() != (int)GT_Storage)  return false;
        // XXX tuples of strings not supported
        const int numElements = gtData->entries();
        GT_DataArrayHandle buffer;
        usdArray.resize(numElements);
        gtData->fillStrings(usdArray.data());
        
        return true;
    }
};

//#############################################################################

bool
setExtentSample(UsdGeomBoundable& usdPrim,
                const GT_PrimitiveHandle& gtPrim,
                UsdTimeCode time=UsdTimeCode::Default())
{
    UT_BoundingBox houBoundsArray[1];
    houBoundsArray[0].initBounds();
    gtPrim->enlargeBounds(houBoundsArray, 1);

    // XXX do we want extent to take width into account?
    //gtPrim->enlargeRenderBounds(houBoundsArray, 1);
    VtVec3fArray extent(2);
    extent[0] = GfVec3f(houBoundsArray[0].xmin(),
                        houBoundsArray[0].ymin(),
                        houBoundsArray[0].zmin());
    extent[1] = GfVec3f(houBoundsArray[0].xmax(),
                        houBoundsArray[0].ymax(),
                        houBoundsArray[0].zmax());

    return usdPrim.GetExtentAttr().Set(extent, time);
}

template <typename T> bool
setPvSample(UsdGeomImageable&           usdPrim,
            const TfToken&              name,
            const GT_DataArrayHandle&   gtData,
            const TfToken&              interpolationIn,
            UsdTimeCode                 time)
{
    using Traits = TypeConvertTraits<T>;
    static const GtDataToUsdTypename usdTypename;

    TfToken interpolation = interpolationIn;
    //bool isArrayType = (interpolation != UsdGeomTokens->constant);
    SdfValueTypeName typeName = usdTypename( gtData, true );
    if( !typeName ) {
        TF_WARN( "Can't find type name for primvar %s:%s", 
                usdPrim.GetPrim().GetPath().GetText(),
                name.GetText() );
        return false;
    }
    UsdGeomPrimvar existingPrimvar = usdPrim.GetPrimvar( name );
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
    if(primvar.GetTypeName().IsArray()) {

        typename Traits::ArrayType usdValues;
        if(!Traits::fillArray(usdValues, gtData))
            return false;
        return primvar.Set(usdValues, time);
    } else {
        typename Traits::Type usdValue;
        if(!Traits::fillValue(usdValue, gtData))
            return false;
        return primvar.Set(usdValue, time);
    }

    // should never get here
    return false;
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


bool GusdGT_Utils::
setUsdAttribute(UsdAttribute& destAttr,
                const GT_DataArrayHandle& sourceAttr,
                UsdTimeCode time)
{
    if(!sourceAttr || !destAttr) return false;

    const TfToken usdType = destAttr.GetTypeName().GetAsToken();

    if (SdfValueTypeNames->Point3fArray  == usdType
     || SdfValueTypeNames->Normal3fArray == usdType
     || SdfValueTypeNames->Vector3fArray == usdType
     || SdfValueTypeNames->Float3Array == usdType) {

        TypeConvertTraits<GfVec3f>::ArrayType usdArray;
        if(!TypeConvertTraits<GfVec3f>::fillArray(usdArray, sourceAttr)) {
            return false;
        }
        return destAttr.Set(usdArray, time);
    }
    else if(SdfValueTypeNames->Quath == usdType) {
        TypeConvertTraits<GfQuath>::Type usdValue;
        if(!TypeConvertTraits<GfQuath>::fillValue(usdValue, sourceAttr)) {
            return false;
        }
        return destAttr.Set(usdValue, time);
    }
    else if(SdfValueTypeNames->QuathArray == usdType) {
        TypeConvertTraits<GfQuath>::ArrayType usdArray;
        if(!TypeConvertTraits<GfQuath>::fillArray(usdArray, sourceAttr)) {
            return false;
        }
        return destAttr.Set(usdArray, time);
    }
    else if(SdfValueTypeNames->Float4Array == usdType) {
        TypeConvertTraits<GfVec4f>::ArrayType usdArray;
        if(!TypeConvertTraits<GfVec4f>::fillArray(usdArray, sourceAttr)) {
            return false;
        }
        return destAttr.Set(usdArray, time);
    }
    else if(SdfValueTypeNames->FloatArray == usdType) {        
        TypeConvertTraits<float>::ArrayType usdArray;
        if(!TypeConvertTraits<float>::fillArray(usdArray, sourceAttr)) {
            return false;
        }
        return destAttr.Set(usdArray, time);
    }
    else if(SdfValueTypeNames->Float == usdType) {     
        TypeConvertTraits<float>::Type usdValue;
        if(!TypeConvertTraits<float>::fillValue(usdValue, sourceAttr)) {
            return false;
        }
        return destAttr.Set(usdValue, time);
    }
    else if(SdfValueTypeNames->DoubleArray == usdType) {        
        TypeConvertTraits<double>::ArrayType usdArray;
        if(!TypeConvertTraits<double>::fillArray(usdArray, sourceAttr)) {
            return false;
        }
        return destAttr.Set(usdArray, time);
    }
    else if(SdfValueTypeNames->Double == usdType) {     
        TypeConvertTraits<double>::Type usdValue;
        if(!TypeConvertTraits<double>::fillValue(usdValue, sourceAttr)) {
            return false;
        }
        return destAttr.Set(usdValue, time);
    }
    else if(SdfValueTypeNames->BoolArray == usdType) {     
        TypeConvertTraits<bool>::ArrayType usdArray;
        if(!TypeConvertTraits<bool>::fillArray(usdArray, sourceAttr)) {
            return false;
        }
        return destAttr.Set(usdArray, time);
    }
    else if(SdfValueTypeNames->IntArray == usdType) {

        if(GT_STORE_INT64 == sourceAttr->getStorage()) {
            TypeConvertTraits<int64_t>::ArrayType usdArray;
            if(!TypeConvertTraits<int64_t>::fillArray(usdArray, sourceAttr)) {
                return false;
            }
            return destAttr.Set(usdArray, time);
        }
        else if(GT_STORE_INT32 == sourceAttr->getStorage()) {
            TypeConvertTraits<int>::ArrayType usdArray;
            if(!TypeConvertTraits<int>::fillArray(usdArray, sourceAttr)) {
                return false;
            }
            return destAttr.Set(usdArray, time);
        }
    }
    else if(SdfValueTypeNames->HalfArray == usdType) {        
        TypeConvertTraits<GfHalf>::ArrayType usdArray;
        if(!TypeConvertTraits<GfHalf>::fillArray(usdArray, sourceAttr)) {
            return false;
        }
        return destAttr.Set(usdArray, time);
    }
    else if(SdfValueTypeNames->Half == usdType) {     
        TypeConvertTraits<GfHalf>::Type usdValue;
        if(!TypeConvertTraits<GfHalf>::fillValue(usdValue, sourceAttr)) {
            return false;
        }
        return destAttr.Set(usdValue, time);
    }
    cerr << "setUsdAttribute: type not implemented: " << usdType << endl;

    return false;
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


void
GusdGT_Utils::setPrimvarSample( 
    UsdGeomImageable& usdPrim, 
    const TfToken &name, 
    const GT_DataArrayHandle& data, 
    const TfToken& interpolation,
    UsdTimeCode time )
{
    const GT_Storage gtStorage = data->getStorage();
    const GT_Size gtTupleSize = data->getTupleSize();

    DBG(cerr << "GusdGT_Utils::setPrimvarSample: " << name << ", " << GTstorage( gtStorage ) << ", " << gtTupleSize << ", " << interpolation << endl);
    
    if(gtStorage == GT_STORE_REAL32) {
        if (gtTupleSize == 3) {
            setPvSample<GfVec3f>(usdPrim,
                                 name,
                                 data,
                                 interpolation,
                                 time);
        }
        else if(gtTupleSize == 4) {
            setPvSample<GfVec4f>(usdPrim,
                                 name,
                                 data,
                                 interpolation,
                                 time);
        }
        else if(gtTupleSize==2) {
            setPvSample<GfVec2f>(usdPrim,
                                 name,
                                 data,
                                 interpolation,
                                 time);
        }
        else { 
            setPvSample<float>(usdPrim,
                               name,
                               data,
                               interpolation,
                               time);
        }
    } 
    else if(gtStorage == GT_STORE_INT32) {
        if(gtTupleSize == 3) {
            setPvSample<GfVec3i>(usdPrim,
                                 name,
                                 data,
                                 interpolation,
                                 time);
        }
        else if(gtTupleSize == 4) {
            setPvSample<GfVec4i>(usdPrim,
                                 name,
                                 data,
                                 interpolation,
                                 time);
        }
        else {
            setPvSample<int>(usdPrim,
                             name,
                             data,
                             interpolation,
                             time);
        }

    }
    else if(gtStorage == GT_STORE_INT64) {
        setPvSample<int64_t>(usdPrim,
                             name,
                             data,
                             interpolation,
                             time);

    }
    else if(gtStorage == GT_STORE_STRING) {
        setPvSample<std::string>(usdPrim,
                                 name,
                                 data,
                                 interpolation,
                                 time);
    }
    else if(gtStorage == GT_STORE_REAL16) {
        if(gtTupleSize == 3) {
            setPvSample<GfVec3h>(usdPrim,
                                 name,
                                 data,
                                 interpolation,
                                 time);
        }
        else if(gtTupleSize == 4) {
            setPvSample<GfVec4h>(usdPrim,
                                 name,
                                 data,
                                 interpolation,
                                 time);
        }
        else {
            setPvSample<GfHalf>(usdPrim,
                                name,
                                data,
                                interpolation,
                                time);
        }
    }
    else {
        TF_WARN( "Unsupported primvar type: %s, %s, tupleSize = %zd", 
                name.GetText(), GTstorage( gtStorage ), gtTupleSize );
    }
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

    if( entries <= 1 )
        return true;

    if( storage == GT_STORE_UINT8 ) {

        GT_DataArrayHandle buffer;
        return isDataConst<uint8>(
                    data->getU8Array( buffer ),
                    entries,
                    tupleSize );

    }  
    else if( storage == GT_STORE_INT32 ) {

        GT_DataArrayHandle buffer;
        return isDataConst<int32>(
                    data->getI32Array( buffer ),
                    entries,
                    tupleSize );

    } 
    else if( storage == GT_STORE_INT64 ) {

        GT_DataArrayHandle buffer;
        return isDataConst<int64>(
                    data->getI64Array( buffer ),
                    entries,
                    tupleSize );

    } 
    else if( storage == GT_STORE_REAL16 ) {

        GT_DataArrayHandle buffer;
        return isDataConst<fpreal16>(
                    data->getF16Array( buffer ),
                    entries,
                    tupleSize );

    }  
    else if( storage == GT_STORE_REAL32 ) {

        GT_DataArrayHandle buffer;
        return isDataConst<fpreal32>(
                    data->getF32Array( buffer ),
                    entries,
                    tupleSize );

    }  
    else if( storage == GT_STORE_REAL64 ) {

        GT_DataArrayHandle buffer;
        return isDataConst<fpreal64>(
                    data->getF64Array( buffer ),
                    entries,
                    tupleSize );

    }
    else if( storage == GT_STORE_STRING ) {

        if( data->getStringIndexCount() >= 0 ) {

            // If this is an indexed string array, we can just compare the indexes.
            // One would think that getIndexedStrings would return in indexes, 
            // but it doesn't. If we look at the header file for GT_DAIndexedString,
            // we can deduce that getI32Array gets you the indexes.
            GT_DataArrayHandle buffer;
            const int32* indicies = data->getI32Array( buffer );
            if( indicies ) {
                int32 first = indicies[0];
                for( GT_Size i = 1; i < entries; ++i ) {
                    if( indicies[i] != first ) {
                        return false;
                    }
                }
                return true;
            }
        }

        UT_StringArray strings;
        data->getStrings( strings );

        // beware of arrays of strings, I don't how to compare these.
        if( strings.entries() == 0 ) {
            return false;
        }

        const UT_StringHolder &first = strings(0);
        for( GT_Size i = 1, end = std::min( entries, strings.entries() ); i < end; ++i ) {
            if( strings(i) != first ) {
                return false;
            }
        }
        return true;
    }
    TF_WARN( "Unsupported primvar type: %s, tupleSize = %zd", 
                GTstorage( storage ), tupleSize );
    return false;
}

void GusdGT_Utils::
setCustomAttributesFromGTPrim(
    UsdGeomImageable &usdGeomPrim,
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
setTransformFromGTArray(UsdGeomXformable& usdGeom,
                        const GT_DataArrayHandle& xform,
                        const TransformLevel transformLevel,
                        UsdTimeCode time)
{
    if(!usdGeom || !xform) return false;

    bool resetsXformStack=false;
    std::vector<UsdGeomXformOp> xformOps = usdGeom.GetOrderedXformOps(
        &resetsXformStack);
    if(xformOps.size() <= transformLevel) return false;

    typedef TypeConvertTraits<GfMatrix4d> Traits;
    typename Traits::Type mat4;
    Traits::fillValue(mat4, xform);

    //return xformOps[transformLevel].Set(mat4, time);
    return xformOps[transformLevel].Set(mat4, time);
}

GfMatrix4d GusdGT_Utils::
getMatrixFromGTArray(const GT_DataArrayHandle& xform)
{
    typedef TypeConvertTraits<GfMatrix4d> Traits;
    typename Traits::Type mat4;
    Traits::fillValue(mat4, xform);    
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
