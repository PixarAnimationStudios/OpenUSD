//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_TYPES_H
#define PXR_IMAGING_HD_TYPES_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/base/vt/value.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE

/// \enum HdWrap
///
/// Enumerates wrapping attributes type values.
///
/// <ul>
///     <li>\b HdWrapClamp               Clamp coordinate to range [1/(2N),1-1/(2N)] where N is the size of the texture in the direction of clamping</li>
///     <li>\b HdWrapRepeat              Creates a repeating pattern</li>
///     <li>\b HdWrapBlack               Clamp coordinate to range [-1/(2N),1+1/(2N)] where N is the size of the texture in the direction of clamping</li>
///     <li>\b HdWrapMirror              Creates a mirrored repeating pattern.</li>
///     <li>\b HdWrapNoOpinion           No opinion. The data texture can define its own wrap mode that we can use instead. Fallback to HdWrapBlack</li>
///     <li>\b HdWrapLegacyNoOpinionFallbackRepeat  (deprecated) Similar to HdWrapNoOpinon but fallback to HdWrapRepeat</li>
///     <li>\b HdWrapUseMetadata         (deprecated) Alias for HdWrapNoOpinion</li>
///     <li>\b HdWrapLegacy              (deprecated) Alias for HdWrapLegacyNoOpinionFallbackRepeat</li>
/// </ul>
///
enum HdWrap 
{
    HdWrapClamp,
    HdWrapRepeat,
    HdWrapBlack,
    HdWrapMirror,

    HdWrapNoOpinion,
    HdWrapLegacyNoOpinionFallbackRepeat, // deprecated

    HdWrapUseMetadata = HdWrapNoOpinion, // deprecated alias
    HdWrapLegacy = HdWrapLegacyNoOpinionFallbackRepeat // deprecated alias
};

/// \enum HdMinFilter
///
/// Enumerates minFilter attribute type values.
///
/// <ul>
///     <li>\b HdMinFilterNearest                Nearest to center of the pixel</li>
///     <li>\b HdMinFilterLinear                 Weighted average od the four texture elements closest to the pixel</li>
///     <li>\b HdMinFilterNearestMipmapNearest   Nearest to center of the pixel from the nearest mipmaps</li>
///     <li>\b HdMinFilterLinearMipmapNeares     Weighted average using texture elements from the nearest mipmaps</li>
///     <li>\b HdMinFilterNearestMipmapLinear    Weighted average of the nearest pixels from the two nearest mipmaps</li>
///     <li>\b HdMinFilterLinearMipmapLinear     WeightedAverage of the weighted averages from the nearest mipmaps</li>
/// </ul>
///
enum HdMinFilter 
{
    HdMinFilterNearest,
    HdMinFilterLinear,
    HdMinFilterNearestMipmapNearest,
    HdMinFilterLinearMipmapNearest,
    HdMinFilterNearestMipmapLinear,
    HdMinFilterLinearMipmapLinear,
};

/// \enum HdMagFilter
///
/// Enumerates magFilter attribute type values.
///
/// <ul>
///     <li>HdFilterNearest       Nearest to center of the pixel</li>
///     <li>HdFilterLinear        Weighted average of the four texture elements closest to the pixel</li>
/// </ul>
///
enum HdMagFilter 
{
    HdMagFilterNearest,
    HdMagFilterLinear,
};

/// \enum HdBorderColor
///
/// Border color to use for clamped texture values.
///
/// <ul>
///     <li>HdBorderColorTransparentBlack</li>
///     <li>HdBorderColorOpaqueBlack</li>
///     <li>HdBorderColorOpaqueWhite</li>
/// </ul>
///
enum HdBorderColor 
{
    HdBorderColorTransparentBlack,
    HdBorderColorOpaqueBlack,
    HdBorderColorOpaqueWhite,
};

/// \class HdSamplerParameters
///
/// Collection of standard parameters such as wrap modes to sample a texture.
///
class HdSamplerParameters {
public:
    HdWrap wrapS;
    HdWrap wrapT;
    HdWrap wrapR;
    HdMinFilter minFilter;
    HdMagFilter magFilter;
    HdBorderColor borderColor;
    bool enableCompare;
    HdCompareFunction compareFunction;
    uint32_t maxAnisotropy;

    HD_API
    HdSamplerParameters();   

    HD_API
    HdSamplerParameters(HdWrap wrapS, HdWrap wrapT, HdWrap wrapR, 
        HdMinFilter minFilter, HdMagFilter magFilter,
        HdBorderColor borderColor=HdBorderColorTransparentBlack,
        bool enableCompare=false, 
        HdCompareFunction compareFunction=HdCmpFuncNever,
        uint32_t maxAnisotropy=16);

    HD_API 
    bool operator==(const HdSamplerParameters &other) const;

    HD_API
    bool operator!=(const HdSamplerParameters &other) const;
};

///
/// Type representing a set of dirty bits.
///
typedef uint32_t HdDirtyBits;

// GL Spec 2.3.5.2 (signed case, eq 2.4)
inline int HdConvertFloatToFixed(float v, int b)
{
    return int(
        std::round(
            std::min(std::max(v, -1.0f), 1.0f) * (float(1 << (b-1)) - 1.0f)));
}

// GL Spec 2.3.5.1 (signed case, eq 2.2)
inline float HdConvertFixedToFloat(int v, int b)
{
    return float(
        std::max(-1.0f,
            (v / (float(1 << (b-1)) - 1.0f))));
}

///
/// HdVec4f_2_10_10_10_REV is a compact representation of a GfVec4f.
/// It uses 10 bits for x, y, and z, and 2 bits for w.
///
/// XXX We expect this type to move again as we continue work on
/// refactoring the GL dependencies.
/// 
struct HdVec4f_2_10_10_10_REV
{
    HdVec4f_2_10_10_10_REV() { }

    template <typename Vec3Type>
    HdVec4f_2_10_10_10_REV(Vec3Type const &value) {
        x = HdConvertFloatToFixed(value[0], 10);
        y = HdConvertFloatToFixed(value[1], 10);
        z = HdConvertFloatToFixed(value[2], 10);
        w = 0;
    }

    HdVec4f_2_10_10_10_REV(int const value) {
        HdVec4f_2_10_10_10_REV const* other =
            reinterpret_cast<HdVec4f_2_10_10_10_REV const*>(&value);
        x = other->x;
        y = other->y;
        z = other->z;
        w = other->w;
    }

    template <typename Vec3Type>
    Vec3Type GetAsVec() const {
        return Vec3Type(HdConvertFixedToFloat(x, 10),
                        HdConvertFixedToFloat(y, 10),
                        HdConvertFixedToFloat(z, 10));
    }

    int GetAsInt() const {
        int const* asInt = reinterpret_cast<int const*>(this);
        return *asInt;
    }

    bool operator==(const HdVec4f_2_10_10_10_REV &other) const {
        return (other.w == w && 
                other.z == z && 
                other.y == y && 
                other.x == x);
    }
    bool operator!=(const HdVec4f_2_10_10_10_REV &other) const {
        return !(*this == other);
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

    HdTypeCount
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

// Support TfHash.
template <class HashState>
void
TfHashAppend(HashState &h, HdTupleType const &tt)
{
    h.Append(tt.type, tt.count);
}

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

    // Int16 - a 2-byte signed integer
    HdFormatInt16,
    HdFormatInt16Vec2,
    HdFormatInt16Vec3,
    HdFormatInt16Vec4,

    // UInt16 - a 2-byte unsigned integer
    HdFormatUInt16,
    HdFormatUInt16Vec2,
    HdFormatUInt16Vec3,
    HdFormatUInt16Vec4,

    // Int32 - a 4-byte signed integer
    HdFormatInt32,
    HdFormatInt32Vec2,
    HdFormatInt32Vec3,
    HdFormatInt32Vec4,

    // Depth-stencil format
    HdFormatFloat32UInt8,

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

///
/// Type representing a depth-stencil value.
///
using HdDepthStencilType = std::pair<float, uint32_t>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_TYPES_H
