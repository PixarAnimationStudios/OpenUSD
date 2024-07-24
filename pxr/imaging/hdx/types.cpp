//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/types.h"

#include "pxr/base/tf/iterator.h"

#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE


// hdxShaderInputs implementation
bool
operator==(const HdxShaderInputs& lhs, const HdxShaderInputs& rhs)
{
    return  lhs.parameters == rhs.parameters && 
            lhs.textures == rhs.textures     && 
            lhs.textureFallbackValues == rhs.textureFallbackValues && 
            lhs.attributes == rhs.attributes;
}

bool
operator!=(const HdxShaderInputs& lhs, const HdxShaderInputs& rhs)
{
    return !(lhs == rhs);
}

std::ostream&
operator<<(std::ostream& out, const HdxShaderInputs& pv)
{
    out << pv.parameters << " "
        << pv.textures << " "
        << pv.textureFallbackValues << " ";

    for (const TfToken &attribute : pv.attributes) {
        out << attribute;
    }
    return out;
}

const HioFormat FORMAT_DESC[] =
{
    // HioFormat            
    HioFormatUNorm8,        // UNorm8
    HioFormatUNorm8Vec2,    // UNorm8Vec2
    // HioFormatUNormVec3,     // Unsupported by HgiFormat
    HioFormatUNorm8Vec4,    // UNorm8Vec4

    HioFormatSNorm8,        // SNorm8
    HioFormatSNorm8Vec2,    // SNorm8Vec2
    // HioFormatSNorm8Vec3,    // Unsupported by HgiFormat
    HioFormatSNorm8Vec4,    // SNorm8Vec4

    HioFormatFloat16,       // Float16
    HioFormatFloat16Vec2,   // Float16Vec2
    HioFormatFloat16Vec3,   // Float16Vec3
    HioFormatFloat16Vec4,   // Float16Vec4

    HioFormatFloat32,       // Float32
    HioFormatFloat32Vec2,   // Float32Vec2
    HioFormatFloat32Vec3,   // Float32Vec3
    HioFormatFloat32Vec4,   // Float32Vec4

    HioFormatInt16,         // Int16
    HioFormatInt16Vec2,     // Int16Vec2
    HioFormatInt16Vec3,     // Int16Vec3
    HioFormatInt16Vec4,     // Int16Vec4

    HioFormatUInt16,        // UInt16
    HioFormatUInt16Vec2,    // UInt16Vec2
    HioFormatUInt16Vec3,    // UInt16Vec3
    HioFormatUInt16Vec4,    // UInt16Vec4

    HioFormatInt32,         // Int32
    HioFormatInt32Vec2,     // Int32Vec2
    HioFormatInt32Vec3,     // Int32Vec3
    HioFormatInt32Vec4,     // Int32Vec4

    // HioFormatUNorm8Vec3srgb,     // Unsupported by HgiFormat
    HioFormatUNorm8Vec4srgb,     // UNorm8Vec4sRGB,

    HioFormatBC6FloatVec3,         // BC6FloatVec3
    HioFormatBC6UFloatVec3,        // BC6UFloatVec3
    HioFormatBC7UNorm8Vec4,        // BC7UNorm8Vec4
    HioFormatBC7UNorm8Vec4srgb,    // BC7UNorm8Vec4srgb
    HioFormatBC1UNorm8Vec4,        // BC1UNorm8Vec4
    HioFormatBC3UNorm8Vec4,        // BC3UNorm8Vec4

    HioFormatFloat32, // HdFormatFloat32UInt8

    HioFormatInt32,   // PackedInt1010102

};

// A few random format validations to make sure out Hio table stays aligned
// with the HgiFormat table.
constexpr bool _CompileTimeValidateHgiFormatTable() {
    return (TfArraySize(FORMAT_DESC) == HgiFormatCount &&
            HgiFormatUNorm8 == 0 &&
            HgiFormatFloat16Vec4 == 9 &&
            HgiFormatFloat32Vec4 == 13 &&
            HgiFormatUInt16Vec4 == 21 &&
            HgiFormatInt32Vec4 == 25 &&
            HgiFormatUNorm8Vec4srgb == 26 &&
            HgiFormatBC3UNorm8Vec4 == 32) ? true : false;
}

static_assert(_CompileTimeValidateHgiFormatTable(), 
              "_FormatDesc array out of sync with HgiFormat enum");

HioFormat HdxGetHioFormat(HgiFormat hgiFormat)
{
    return FORMAT_DESC[hgiFormat];
}



PXR_NAMESPACE_CLOSE_SCOPE

