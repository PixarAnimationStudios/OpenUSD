//
// Copyright 2018 Pixar
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
#include "pxr/pxr.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec2h.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec3h.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/base/gf/vec4h.h"

#include <unordered_map>
#include <typeinfo>
#include <typeindex>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(HdTypeInvalid);
    TF_ADD_ENUM_NAME(HdTypeBool);
    TF_ADD_ENUM_NAME(HdTypeUInt8);
    TF_ADD_ENUM_NAME(HdTypeUInt16);
    TF_ADD_ENUM_NAME(HdTypeInt8);
    TF_ADD_ENUM_NAME(HdTypeInt16);
    TF_ADD_ENUM_NAME(HdTypeInt32);
    TF_ADD_ENUM_NAME(HdTypeInt32Vec2);
    TF_ADD_ENUM_NAME(HdTypeInt32Vec3);
    TF_ADD_ENUM_NAME(HdTypeInt32Vec4);
    TF_ADD_ENUM_NAME(HdTypeUInt32);
    TF_ADD_ENUM_NAME(HdTypeUInt32Vec2);
    TF_ADD_ENUM_NAME(HdTypeUInt32Vec3);
    TF_ADD_ENUM_NAME(HdTypeUInt32Vec4);
    TF_ADD_ENUM_NAME(HdTypeFloat);
    TF_ADD_ENUM_NAME(HdTypeFloatVec2);
    TF_ADD_ENUM_NAME(HdTypeFloatVec3);
    TF_ADD_ENUM_NAME(HdTypeFloatVec4);
    TF_ADD_ENUM_NAME(HdTypeFloatMat3);
    TF_ADD_ENUM_NAME(HdTypeFloatMat4);
    TF_ADD_ENUM_NAME(HdTypeHalfFloat);
    TF_ADD_ENUM_NAME(HdTypeHalfFloatVec2);
    TF_ADD_ENUM_NAME(HdTypeHalfFloatVec3);
    TF_ADD_ENUM_NAME(HdTypeHalfFloatVec4);
    TF_ADD_ENUM_NAME(HdTypeDouble);
    TF_ADD_ENUM_NAME(HdTypeDoubleVec2);
    TF_ADD_ENUM_NAME(HdTypeDoubleVec3);
    TF_ADD_ENUM_NAME(HdTypeDoubleVec4);
    TF_ADD_ENUM_NAME(HdTypeDoubleMat3);
    TF_ADD_ENUM_NAME(HdTypeDoubleMat4);
    TF_ADD_ENUM_NAME(HdTypeInt32_2_10_10_10_REV);

    TF_ADD_ENUM_NAME(HdFormatInvalid);
    TF_ADD_ENUM_NAME(HdFormatUNorm8);
    TF_ADD_ENUM_NAME(HdFormatUNorm8Vec2);
    TF_ADD_ENUM_NAME(HdFormatUNorm8Vec3);
    TF_ADD_ENUM_NAME(HdFormatUNorm8Vec4);
    TF_ADD_ENUM_NAME(HdFormatSNorm8);
    TF_ADD_ENUM_NAME(HdFormatSNorm8Vec2);
    TF_ADD_ENUM_NAME(HdFormatSNorm8Vec3);
    TF_ADD_ENUM_NAME(HdFormatSNorm8Vec4);
    TF_ADD_ENUM_NAME(HdFormatFloat16);
    TF_ADD_ENUM_NAME(HdFormatFloat16Vec2);
    TF_ADD_ENUM_NAME(HdFormatFloat16Vec3);
    TF_ADD_ENUM_NAME(HdFormatFloat16Vec4);
    TF_ADD_ENUM_NAME(HdFormatFloat32);
    TF_ADD_ENUM_NAME(HdFormatFloat32Vec2);
    TF_ADD_ENUM_NAME(HdFormatFloat32Vec3);
    TF_ADD_ENUM_NAME(HdFormatFloat32Vec4);
    TF_ADD_ENUM_NAME(HdFormatInt32);
    TF_ADD_ENUM_NAME(HdFormatInt32Vec2);
    TF_ADD_ENUM_NAME(HdFormatInt32Vec3);
    TF_ADD_ENUM_NAME(HdFormatInt32Vec4);
}


template <class T>
static void const *_GetArrayData(VtValue const &v) {
    return v.UncheckedGet<VtArray<T>>().cdata();
}
template <class T>
static void const *_GetSingleData(VtValue const &v) {
    return &v.UncheckedGet<T>();
}
using GetDataFunc = void const *(*)(VtValue const &);
using ValueDataGetterMap = std::unordered_map<std::type_index, GetDataFunc>;
static inline ValueDataGetterMap _MakeValueDataGetterMap() {
#define ELEM(T)                                          \
    { typeid(T), &_GetSingleData<T> },                   \
    { typeid(VtArray<T>), &_GetArrayData<T> }

    return ValueDataGetterMap {
        ELEM(GfHalf),
        ELEM(GfMatrix3d),
        ELEM(GfMatrix3f),
        ELEM(GfMatrix4d),
        ELEM(GfMatrix4f),
        ELEM(GfVec2d),
        ELEM(GfVec2f),
        ELEM(GfVec2h),
        ELEM(GfVec2i),
        ELEM(GfVec3d),
        ELEM(GfVec3f),
        ELEM(GfVec3h),
        ELEM(GfVec3i),
        ELEM(GfVec4d),
        ELEM(GfVec4f),
        ELEM(GfVec4h),
        ELEM(GfVec4i),
        ELEM(HdVec4f_2_10_10_10_REV),
        ELEM(bool),
        ELEM(char),
        ELEM(double),
        ELEM(float),
        ELEM(int16_t),
        ELEM(int32_t),
        ELEM(uint16_t),
        ELEM(uint32_t),
        ELEM(unsigned char),
    };
#undef ELEM
}        

const void* HdGetValueData(const VtValue &value)
{
    static ValueDataGetterMap const getterMap = _MakeValueDataGetterMap();
    auto const iter = getterMap.find(value.GetTypeid());
    if (ARCH_UNLIKELY(iter == getterMap.end())) {
        return nullptr;
    }
    return iter->second(value);
}

using TupleTypeMap = std::unordered_map<std::type_index, HdType>;
static inline TupleTypeMap _MakeTupleTypeMap() {
    return TupleTypeMap {
        { typeid(GfHalf), HdTypeHalfFloat },
        { typeid(GfMatrix3d), HdTypeDoubleMat3 },
        { typeid(GfMatrix3f), HdTypeFloatMat3 },
        { typeid(GfMatrix4d), HdTypeDoubleMat4 },
        { typeid(GfMatrix4f), HdTypeFloatMat4 },
        { typeid(GfVec2d), HdTypeDoubleVec2 },
        { typeid(GfVec2f), HdTypeFloatVec2 },
        { typeid(GfVec2h), HdTypeHalfFloatVec2 },
        { typeid(GfVec2i), HdTypeInt32Vec2 },
        { typeid(GfVec3d), HdTypeDoubleVec3 },
        { typeid(GfVec3f), HdTypeFloatVec3 },
        { typeid(GfVec3h), HdTypeHalfFloatVec3 },
        { typeid(GfVec3i), HdTypeInt32Vec3 },
        { typeid(GfVec4d), HdTypeDoubleVec4 },
        { typeid(GfVec4f), HdTypeFloatVec4 },
        { typeid(GfVec4h), HdTypeHalfFloatVec4 },
        { typeid(GfVec4i), HdTypeInt32Vec4 },
        { typeid(HdVec4f_2_10_10_10_REV), HdTypeInt32_2_10_10_10_REV },
        { typeid(bool), HdTypeBool },
        { typeid(char), HdTypeInt8 },
        { typeid(double), HdTypeDouble },
        { typeid(float), HdTypeFloat },
        { typeid(int16_t), HdTypeInt16 },
        { typeid(int32_t), HdTypeInt32 },
        { typeid(uint16_t), HdTypeUInt16 },
        { typeid(uint32_t), HdTypeUInt32 },
        { typeid(unsigned char), HdTypeUInt8 },
    };
}

HdTupleType HdGetValueTupleType(const VtValue &value)
{
    static const TupleTypeMap tupleTypeMap = _MakeTupleTypeMap();
    
    if (value.IsArrayValued()) {
        auto const iter = tupleTypeMap.find(value.GetElementTypeid());
        if (ARCH_UNLIKELY(iter == tupleTypeMap.end())) {
            return HdTupleType { HdTypeInvalid, 0 };
        }
        return HdTupleType { iter->second, value.GetArraySize() };
    }
    auto const iter = tupleTypeMap.find(value.GetTypeid());
    if (ARCH_UNLIKELY(iter == tupleTypeMap.end())) {
        return HdTupleType { HdTypeInvalid, 0 };
    }
    return HdTupleType { iter->second, 1 };
}

HdType HdGetComponentType(HdType t)
{
    switch (t) {
    case HdTypeUInt32Vec2:
    case HdTypeUInt32Vec3:
    case HdTypeUInt32Vec4:
        return HdTypeUInt32;
    case HdTypeInt32Vec2:
    case HdTypeInt32Vec3:
    case HdTypeInt32Vec4:
        return HdTypeInt32;
    case HdTypeFloatVec2:
    case HdTypeFloatVec3:
    case HdTypeFloatVec4:
    case HdTypeFloatMat3:
    case HdTypeFloatMat4:
        return HdTypeFloat;
    case HdTypeDoubleVec2:
    case HdTypeDoubleVec3:
    case HdTypeDoubleVec4:
    case HdTypeDoubleMat3:
    case HdTypeDoubleMat4:
        return HdTypeDouble;
    case HdTypeHalfFloatVec2:
    case HdTypeHalfFloatVec3:
    case HdTypeHalfFloatVec4:
        return HdTypeHalfFloat;
    default:
        return t;
    }
}

size_t HdGetComponentCount(HdType t)
{
    switch (t) {
    case HdTypeInt32Vec2:
    case HdTypeUInt32Vec2:
    case HdTypeFloatVec2:
    case HdTypeDoubleVec2:
    case HdTypeHalfFloatVec2:
        return 2;
    case HdTypeInt32Vec3:
    case HdTypeUInt32Vec3:
    case HdTypeFloatVec3:
    case HdTypeDoubleVec3:
    case HdTypeHalfFloatVec3:
        return 3;
    case HdTypeInt32Vec4:
    case HdTypeUInt32Vec4:
    case HdTypeFloatVec4:
    case HdTypeDoubleVec4:
    case HdTypeHalfFloatVec4:
        return 4;
    case HdTypeFloatMat3:
    case HdTypeDoubleMat3:
        return 3*3;
    case HdTypeFloatMat4:
    case HdTypeDoubleMat4:
        return 4*4;
    default:
        return 1;
    }
}

size_t HdDataSizeOfType(HdType t)
{
    switch (t) {
    case HdTypeInvalid:
    default:
        TF_CODING_ERROR("Cannot query size of invalid HdType");
        return 0;
    case HdTypeBool:
        // XXX: Currently, Hd represents bools as int32 sized values.
        // See HdVtBufferSource for explanation.  This should be moved
        // to the GL backend!
        return sizeof(int32_t);
    case HdTypeInt8:
        return sizeof(int8_t);
    case HdTypeUInt8:
        return sizeof(uint8_t);
    case HdTypeInt16:
        return sizeof(int16_t);
    case HdTypeUInt16:
        return sizeof(uint16_t);
    case HdTypeInt32:
        return sizeof(int32_t);
    case HdTypeInt32Vec2:
        return sizeof(int32_t)*2;
    case HdTypeInt32Vec3:
        return sizeof(int32_t)*3;
    case HdTypeInt32Vec4:
        return sizeof(int32_t)*4;
    case HdTypeUInt32:
        return sizeof(uint32_t);
    case HdTypeUInt32Vec2:
        return sizeof(uint32_t)*2;
    case HdTypeUInt32Vec3:
        return sizeof(uint32_t)*3;
    case HdTypeUInt32Vec4:
        return sizeof(uint32_t)*4;
    case HdTypeFloat:
        return sizeof(float);
    case HdTypeFloatVec2:
        return sizeof(float)*2;
    case HdTypeFloatVec3:
        return sizeof(float)*3;
    case HdTypeFloatVec4:
        return sizeof(float)*4;
    case HdTypeFloatMat3:
        return sizeof(float)*3*3;
    case HdTypeFloatMat4:
        return sizeof(float)*4*4;
    case HdTypeDouble:
        return sizeof(double);
    case HdTypeDoubleVec2:
        return sizeof(double)*2;
    case HdTypeDoubleVec3:
        return sizeof(double)*3;
    case HdTypeDoubleVec4:
        return sizeof(double)*4;
    case HdTypeDoubleMat3:
        return sizeof(double)*3*3;
    case HdTypeDoubleMat4:
        return sizeof(double)*4*4;
    case HdTypeHalfFloat:
        return sizeof(GfHalf);
    case HdTypeHalfFloatVec2:
        return sizeof(GfHalf)*2;
    case HdTypeHalfFloatVec3:
        return sizeof(GfHalf)*3;
    case HdTypeHalfFloatVec4:
        return sizeof(GfHalf)*4;
    case HdTypeInt32_2_10_10_10_REV:
        return sizeof(HdVec4f_2_10_10_10_REV);
    };
}

size_t HdDataSizeOfTupleType(HdTupleType tupleType)
{
    return HdDataSizeOfType(tupleType.type) * tupleType.count;
}

HdFormat HdGetComponentFormat(HdFormat f)
{
    switch(f) {
    case HdFormatUNorm8:
    case HdFormatUNorm8Vec2:
    case HdFormatUNorm8Vec3:
    case HdFormatUNorm8Vec4:
        return HdFormatUNorm8;
    case HdFormatSNorm8:
    case HdFormatSNorm8Vec2:
    case HdFormatSNorm8Vec3:
    case HdFormatSNorm8Vec4:
        return HdFormatSNorm8;
    case HdFormatFloat16:
    case HdFormatFloat16Vec2:
    case HdFormatFloat16Vec3:
    case HdFormatFloat16Vec4:
        return HdFormatFloat16;
    case HdFormatFloat32:
    case HdFormatFloat32Vec2:
    case HdFormatFloat32Vec3:
    case HdFormatFloat32Vec4:
        return HdFormatFloat32;
    case HdFormatInt32:
    case HdFormatInt32Vec2:
    case HdFormatInt32Vec3:
    case HdFormatInt32Vec4:
        return HdFormatInt32;
    default:
        return HdFormatInvalid;
    }
}

size_t HdGetComponentCount(HdFormat f)
{
    switch (f) {
    case HdFormatUNorm8Vec2:
    case HdFormatSNorm8Vec2:
    case HdFormatFloat16Vec2:
    case HdFormatFloat32Vec2:
    case HdFormatInt32Vec2:
        return 2;
    case HdFormatUNorm8Vec3:
    case HdFormatSNorm8Vec3:
    case HdFormatFloat16Vec3:
    case HdFormatFloat32Vec3:
    case HdFormatInt32Vec3:
        return 3;
    case HdFormatUNorm8Vec4:
    case HdFormatSNorm8Vec4:
    case HdFormatFloat16Vec4:
    case HdFormatFloat32Vec4:
    case HdFormatInt32Vec4:
        return 4;
    default:
        return 1;
    }
}

size_t HdDataSizeOfFormat(HdFormat f)
{
    switch(f) {
    case HdFormatUNorm8:
    case HdFormatSNorm8:
        return 1;
    case HdFormatUNorm8Vec2:
    case HdFormatSNorm8Vec2:
        return 2;
    case HdFormatUNorm8Vec3:
    case HdFormatSNorm8Vec3:
        return 3;
    case HdFormatUNorm8Vec4:
    case HdFormatSNorm8Vec4:
        return 4;
    case HdFormatFloat16:
        return 2;
    case HdFormatFloat16Vec2:
        return 4;
    case HdFormatFloat16Vec3:
        return 6;
    case HdFormatFloat16Vec4:
        return 8;
    case HdFormatFloat32:
    case HdFormatInt32:
        return 4;
    case HdFormatFloat32Vec2:
    case HdFormatInt32Vec2:
        return 8;
    case HdFormatFloat32Vec3:
    case HdFormatInt32Vec3:
        return 12;
    case HdFormatFloat32Vec4:
    case HdFormatInt32Vec4:
        return 16;
    default:
        return 0;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
