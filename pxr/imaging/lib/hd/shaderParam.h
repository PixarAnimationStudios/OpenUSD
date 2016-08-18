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
#ifndef HD_SHADER_PARAM_H
#define HD_SHADER_PARAM_H

#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/tf/token.h"

#include <string>
#include <vector>

typedef std::vector<class HdShaderParam> HdShaderParamVector;

bool HdEnabledUV();

// XXX: Docs
class HdShaderParam {
public:
    typedef size_t ID;

    HDLIB_API
    HdShaderParam(TfToken const& name, 
                  VtValue const& fallbackValue,
                  SdfPath const& connection=SdfPath(),
                  TfTokenVector const& samplerCoords=TfTokenVector(),
                  bool isPtex = false);

    HDLIB_API
    ~HdShaderParam();

    /// Computes a hash for all shader parameters. This hash also includes 
    /// ShaderParam connections (texture, primvar, etc).
    HDLIB_API
    static ID ComputeHash(HdShaderParamVector const &shaders);

    TfToken const& GetName() const { return _name; }

    // i.e. GL_FLOAT, etc.
    HDLIB_API
    int GetGLElementType() const;

    // i.e. GL_FLOAT_MAT4, etc.
    HDLIB_API
    int GetGLComponentType() const;

    HDLIB_API
    TfToken GetGLTypeName() const;

    VtValue const& GetFallbackValue() const { return _fallbackValue; }

    SdfPath const& GetConnection() const { return _connection; }

    bool IsTexture() const {
        return not IsFallback() and _connection.IsAbsolutePath();
    }
    bool IsPrimvar() const {
        return not IsFallback() and not IsTexture();
    }
    bool IsFallback() const {
        return _connection.IsEmpty();
    }

    // XXX: we don't want this, we need a better way of supplying this answer.
    HDLIB_API
    bool IsPtex() const;

    HDLIB_API
    TfTokenVector const& GetSamplerCoordinates() const;

private:
    TfToken _name;
    VtValue _fallbackValue;
    SdfPath _connection;
    TfTokenVector _samplerCoords;
    bool _isPtex;
};

#endif //HD_SHADER_PARAM_H
