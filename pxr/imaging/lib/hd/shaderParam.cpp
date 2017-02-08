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
#include "pxr/imaging/hd/shaderParam.h"

#include "pxr/imaging/glf/glew.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2d.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"

#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"

#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/envSetting.h"

#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((_bool, "bool"))
    ((_float, "float"))
    (vec2)
    (vec3)
    (vec4)
    (mat4)
    ((_double, "double"))
    (dvec2)
    (dvec3)
    (dvec4)
    (dmat4)
    ((_int, "int"))
    (ivec2)
    (ivec3)
    (ivec4)
);

HdShaderParam::HdShaderParam(TfToken const& name, 
                             VtValue const& fallbackValue,
                             SdfPath const& connection,
                             TfTokenVector const& samplerCoords,
                             bool isPtex)
    : _name(name)
    , _fallbackValue(fallbackValue)
    , _connection(connection)
    , _samplerCoords(samplerCoords)
    , _isPtex(isPtex)
{
    /*NOTHING*/
}

HdShaderParam::~HdShaderParam()
{
    /*NOTHING*/
}

/* static */
size_t
HdShaderParam::ComputeHash(HdShaderParamVector const &params)
{
    size_t hash = 0;
    TF_FOR_ALL(paramIt, params) {
        boost::hash_combine(hash, paramIt->GetName().Hash());
        boost::hash_combine(hash, paramIt->GetConnection().GetHash());
        TF_FOR_ALL(coordIt, paramIt->GetSamplerCoordinates()) {
            boost::hash_combine(hash, coordIt->Hash());
        }
        boost::hash_combine(hash, paramIt->IsPtex());
    }
    return hash;
}



// XXX: Copied from VtBufferSource
// -------------------------------------------------------------------------- //
// Convert runtime element type into GL component and element type enums.
struct _GLDataType {
    _GLDataType(int componentType, int elementType)
        : componentType(componentType)
        , elementType(elementType) { }
    int componentType;
    int elementType;
};

static
_GLDataType _GetGLType(VtValue const& value)
{
    if (value.IsHolding<char>())
        return _GLDataType(GL_BYTE, GL_BYTE);
    else if (value.IsHolding<short>())
        return _GLDataType(GL_SHORT, GL_SHORT);
    else if (value.IsHolding<unsigned short>())
        return _GLDataType(GL_UNSIGNED_SHORT, GL_UNSIGNED_SHORT);
    else if (value.IsHolding<int>())
        return _GLDataType(GL_INT, GL_INT);
    else if (value.IsHolding<GfVec2i>())
        return _GLDataType(GL_INT, GL_INT_VEC2);
    else if (value.IsHolding<GfVec3i>())
        return _GLDataType(GL_INT, GL_INT_VEC3);
    else if (value.IsHolding<GfVec4i>())
        return _GLDataType(GL_INT, GL_INT_VEC4);
    else if (value.IsHolding<unsigned int>())
        return _GLDataType(GL_UNSIGNED_INT, GL_UNSIGNED_INT);
    else if (value.IsHolding<float>())
        return _GLDataType(GL_FLOAT, GL_FLOAT);
    else if (value.IsHolding<GfVec2f>())
        return _GLDataType(GL_FLOAT, GL_FLOAT_VEC2);
    else if (value.IsHolding<GfVec3f>())
        return _GLDataType(GL_FLOAT, GL_FLOAT_VEC3);
    else if (value.IsHolding<GfVec4f>())
        return _GLDataType(GL_FLOAT, GL_FLOAT_VEC4);
    else if (value.IsHolding<double>())
        return _GLDataType(GL_DOUBLE, GL_DOUBLE);
    else if (value.IsHolding<GfVec2d>())
        return _GLDataType(GL_DOUBLE, GL_DOUBLE_VEC2);
    else if (value.IsHolding<GfVec3d>())
        return _GLDataType(GL_DOUBLE, GL_DOUBLE_VEC3);
    else if (value.IsHolding<GfVec4d>())
        return _GLDataType(GL_DOUBLE, GL_DOUBLE_VEC4);
    else if (value.IsHolding<GfMatrix4f>())
        return _GLDataType(GL_FLOAT, GL_FLOAT_MAT4);
    else if (value.IsHolding<GfMatrix4d>())
        return _GLDataType(GL_DOUBLE, GL_DOUBLE_MAT4);
    else if (value.IsHolding<bool>())
        return _GLDataType(GL_BOOL, GL_BOOL);
    else
        TF_CODING_ERROR("Unknown type held by VtValue in ShaderParam");

    return _GLDataType(0, 0);
}

static
TfToken _GetGLTypeName(GLenum elementType)
{
    if (elementType == GL_FLOAT) {
        return _tokens->_float;
    } else if (elementType == GL_FLOAT_VEC2) {
        return _tokens->vec2;
    } else if (elementType == GL_FLOAT_VEC3) {
        return _tokens->vec3;
    } else if (elementType == GL_FLOAT_VEC4) {
        return _tokens->vec4;
    } else if (elementType == GL_DOUBLE) {
        return _tokens->_double;
    } else if (elementType == GL_DOUBLE_VEC2) {
        return _tokens->dvec2;
    } else if (elementType == GL_DOUBLE_VEC3) {
        return _tokens->dvec3;
    } else if (elementType == GL_DOUBLE_VEC4) {
        return _tokens->dvec4;
    } else if (elementType == GL_FLOAT_MAT4) {
        return _tokens->mat4;
    } else if (elementType == GL_DOUBLE_MAT4) {
        return _tokens->dmat4;
    } else if (elementType == GL_INT) {
        return _tokens->_int;
    } else if (elementType == GL_INT_VEC2) {
        return _tokens->ivec2;
    } else if (elementType == GL_INT_VEC3) {
        return _tokens->ivec3;
    } else if (elementType == GL_INT_VEC4) {
        return _tokens->ivec4;
    } else if (elementType == GL_BOOL) {
        return _tokens->_bool;
    } else {
        TF_CODING_ERROR("unsupported type: 0x%x", elementType);
        return TfToken();
    }
}

// -------------------------------------------------------------------------- //

int
HdShaderParam::GetGLElementType() const
{
    return _GetGLType(GetFallbackValue()).elementType;
}

int
HdShaderParam::GetGLComponentType() const
{
    return _GetGLType(GetFallbackValue()).componentType;
}

TfToken
HdShaderParam::GetGLTypeName() const
{
    return _GetGLTypeName(GetGLElementType());
}

TfTokenVector const&
HdShaderParam::GetSamplerCoordinates() const
{
    // NOTE: could discover from texture connection.
    return _samplerCoords;
}

bool
HdShaderParam::IsPtex() const
{
    return _isPtex;
}

PXR_NAMESPACE_CLOSE_SCOPE

