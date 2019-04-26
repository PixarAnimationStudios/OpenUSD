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
#ifndef HD_TYPES_H
#define HD_TYPES_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/base/vt/value.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE

///
/// Type representing a set of dirty bits.
///
typedef uint32_t HdDirtyBits;

///
/// HdVec4f_2_10_10_10_REV is a compact representation of a GfVec4f.
/// It uses 10 bits for x, y, and z, and 2 bits for w.
///
/// XXX We expect this type to move again as we continue work on
/// refactoring the GL dependencies.
/// 
struct HdVec4f_2_10_10_10_REV {
    // we treat packed type as single-component values
    static const size_t dimension = 1;

    HdVec4f_2_10_10_10_REV() { }

    template <typename Vec3Type>
    HdVec4f_2_10_10_10_REV(Vec3Type const &value) {
        x = to10bits(value[0]);
        y = to10bits(value[1]);
        z = to10bits(value[2]);
        w = 0;
    }

    // ref. GL spec 2.3.5.2
    //   Conversion from floating point to normalized fixed point
    template <typename R>
    int to10bits(R v) {
        return int(
            std::round(
                std::min(std::max(v, static_cast<R>(-1)), static_cast<R>(1))
                *static_cast<R>(511)));
    }

    bool operator==(const HdVec4f_2_10_10_10_REV &other) const {
        return (other.w == w && 
                other.z == z && 
                other.y == y && 
                other.x == x);
    }

    int x : 10;
    int y : 10;
    int z : 10;
    int w : 2;
};

/// \enum HdType
///
/// HdType describes the type of an attribute value used in Hd.
///
/// HdType values have a specific machine representation and size.
/// See HdDataSizeOfType().
///
/// HdType specifies a scalar, vector, or matrix type.  Vector and
/// matrix types can be unpacked into the underlying "component"
/// type; see HdGetComponentType().
///
/// HdType is intended to span the common set of attribute types
/// used in shading languages such as GLSL.  However, it currently
/// does not include non-4x4 matrix types, nor struct types.
///
/// Fixed-size array types are represented by the related class
/// HdTupleType.  HdTupleType is used anywhere there is a
/// possibility of an array of values.
///
/// ## Value arrays and attribute buffers
///
/// Attribute data is often stored in linear buffers.  These buffers
/// have multiple dimensions and it is important to distinguish them:
///
/// - "Components" refer to the scalar components that comprise a vector
///   or matrix.  For example, a vec3 has 3 components, a mat4 has
///   16 components, and a float has a single component.
///
/// - "Elements" refer to external concepts that entries in a buffer
///   associate with.  Typically these are pieces of geometry,
///   such as faces or vertices.
///
/// - "Arrays" refer to the idea that each element may associate
///   with a fixed-size array of values.  For example, one approach
///   to motion blur might store a size-2 array of HdFloatMat4
///   values for each element of geometry, holding the transforms
///   at the beginning and ending of the camera shutter interval.
///
/// Combining these concepts in an example, a primvar buffer might hold
/// data for 10 vertices (the elements) with each vertex having a
/// 2 entries (an array) of 4x4 matrices (with 16 components each).
/// As a packed linear buffer, this would occupy 10*2*16==320 floats.
///
/// It is important to distinguish components from array entries,
/// and arrays from elements.  HdType and HdTupleType only
/// addresses components and arrays; elements are tracked by buffers.
/// See for example HdBufferSource::GetNumElements().
///
/// In other words, HdType and HdTupleType describe values.
/// Buffers describe elements and all other details regarding buffer
/// layout, such as offset/stride used to interleave attribute data.
///
/// For more background, see the OpenGL discussion on data types:
/// - https://www.khronos.org/opengl/wiki/OpenGL_Type
///
enum HdType
{
    HdTypeInvalid=-1,

    /// Corresponds to GL_BOOL
    HdTypeBool=0,
    HdTypeUInt8,
    HdTypeUInt16,
    HdTypeInt8,
    HdTypeInt16,

    /// Corresponds to GL_INT
    HdTypeInt32,
    /// A 2-component vector with Int32-valued components.
    HdTypeInt32Vec2,
    /// A 3-component vector with Int32-valued components.
    HdTypeInt32Vec3,
    /// A 4-component vector with Int32-valued components.
    HdTypeInt32Vec4,

    /// An unsigned 32-bit integer.  Corresponds to GL_UNSIGNED_INT.
    HdTypeUInt32,
    /// A 2-component vector with UInt32-valued components.
    HdTypeUInt32Vec2,
    /// A 3-component vector with UInt32-valued components.
    HdTypeUInt32Vec3,
    /// A 4-component vector with UInt32-valued components.
    HdTypeUInt32Vec4,

    /// Corresponds to GL_FLOAT
    HdTypeFloat,
    /// Corresponds to GL_FLOAT_VEC2
    HdTypeFloatVec2,
    /// Corresponds to GL_FLOAT_VEC3
    HdTypeFloatVec3,
    /// Corresponds to GL_FLOAT_VEC4
    HdTypeFloatVec4,
    /// Corresponds to GL_FLOAT_MAT3
    HdTypeFloatMat3,
    /// Corresponds to GL_FLOAT_MAT4
    HdTypeFloatMat4,

    /// Corresponds to GL_DOUBLE
    HdTypeDouble,
    /// Corresponds to GL_DOUBLE_VEC2
    HdTypeDoubleVec2,
    /// Corresponds to GL_DOUBLE_VEC3
    HdTypeDoubleVec3,
    /// Corresponds to GL_DOUBLE_VEC4
    HdTypeDoubleVec4,
    /// Corresponds to GL_DOUBLE_MAT3
    HdTypeDoubleMat3,
    /// Corresponds to GL_DOUBLE_MAT4
    HdTypeDoubleMat4,

    HdTypeHalfFloat,
    HdTypeHalfFloatVec2,
    HdTypeHalfFloatVec3,
    HdTypeHalfFloatVec4,

    /// Packed, reverse-order encoding of a 4-component vector into Int32.
    /// Corresponds to GL_INT_2_10_10_10_REV.
    /// \see HdVec4f_2_10_10_10_REV
    HdTypeInt32_2_10_10_10_REV,
};

/// HdTupleType represents zero, one, or more values of the same HdType.
/// It can be used to represent fixed-size array types, as well as single
/// values.  See HdType for more discussion about arrays.
struct HdTupleType {
    HdType type;
    size_t count;

    bool operator< (HdTupleType const& rhs) const {
        return (type < rhs.type) || (type == rhs.type && count < rhs.count);
    }
    bool operator== (HdTupleType const& rhs) const {
        return type == rhs.type && count == rhs.count;
    }
    bool operator!= (HdTupleType const& rhs) const {
        return !(*this == rhs);
    }
};

/// Returns a direct pointer to the data held by a VtValue.
/// Returns nullptr if the VtValue is empty or holds a type unknown to Hd.
HD_API
const void* HdGetValueData(const VtValue &);

/// Returns the HdTupleType that describes the given VtValue.
/// For scalar, vector, and matrix types, the count is 1.
/// For any VtArray type, the count is the number of array members.
HD_API
HdTupleType HdGetValueTupleType(const VtValue &);

/// Return the component type for the given value type.
/// For vectors and matrices, this is the scalar type of their components.
/// For scalars, this is the type itself.
/// As an example, the component type of HdTypeFloatMat4 is HdTypeFloat.
HD_API
HdType HdGetComponentType(HdType);

/// Return the count of components in the given value type.
/// For example, HdTypeFloatVec3 has 3 components.
HD_API
size_t HdGetComponentCount(HdType t);

/// Return the size, in bytes, of a single value of the given type.
HD_API
size_t HdDataSizeOfType(HdType);

/// Return the size, in bytes, of a value with HdTupleType.
HD_API
size_t HdDataSizeOfTupleType(HdTupleType);

/// \enum HdFormat
///
/// HdFormat describes the memory format of image buffers used in Hd.
/// It's similar to HdType but with more specific associated semantics.
///
/// The list of supported formats is modelled after Vulkan and DXGI, though
/// Hydra only supports a subset.  Endian-ness is explicitly not captured;
/// color data is assumed to always be RGBA.
///
/// For reference, see:
///   https://www.khronos.org/registry/vulkan/specs/1.1/html/vkspec.html#VkFormat
enum HdFormat
{
    HdFormatInvalid=-1,

    // UNorm8 - a 1-byte value representing a float between 0 and 1.
    // float value = (unorm / 255.0f);
    HdFormatUNorm8=0,
    HdFormatUNorm8Vec2,
    HdFormatUNorm8Vec3,
    HdFormatUNorm8Vec4,

    // SNorm8 - a 1-byte value representing a float between -1 and 1.
    // float value = max(snorm / 127.0f, -1.0f);
    HdFormatSNorm8,
    HdFormatSNorm8Vec2,
    HdFormatSNorm8Vec3,
    HdFormatSNorm8Vec4,

    // Float16 - a 2-byte IEEE half-precision float.
    HdFormatFloat16,
    HdFormatFloat16Vec2,
    HdFormatFloat16Vec3,
    HdFormatFloat16Vec4,

    // Float32 - a 4-byte IEEE float.
    HdFormatFloat32,
    HdFormatFloat32Vec2,
    HdFormatFloat32Vec3,
    HdFormatFloat32Vec4,

    // Int32 - a 4-byte signed integer
    HdFormatInt32,
    HdFormatInt32Vec2,
    HdFormatInt32Vec3,
    HdFormatInt32Vec4,

    HdFormatCount
};

/// Return the single-channel version of a given format.
HD_API
HdFormat HdGetComponentFormat(HdFormat f);

/// Return the count of components in the given format.
HD_API
size_t HdGetComponentCount(HdFormat f);

/// Return the size of a single element of the given format.
/// For block formats, this will return 0.
HD_API
size_t HdDataSizeOfFormat(HdFormat f);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_TYPES_H
