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
#ifndef HD_MATERIAL_PARAM_H
#define HD_MATERIAL_PARAM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/tf/token.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


typedef std::vector<class HdMaterialParam> HdMaterialParamVector;

// XXX: Docs
class HdMaterialParam {
public:
    typedef size_t ID;

    // Indicates the kind of material parameter.
    enum ParamType {
        // This is a shader specified fallback value that is
        // not connected to either a primvar or texture.
        ParamTypeFallback,
        // This is a parameter that is connected to a primvar.
        ParamTypePrimvar,
        // This is a parameter that is connected to a texture.
        ParamTypeTexture
    };

    HD_API
    HdMaterialParam(ParamType paramType,
                    TfToken const& name, 
                    VtValue const& fallbackValue,
                    SdfPath const& connection=SdfPath(),
                    TfTokenVector const& samplerCoords=TfTokenVector(),
                    HdTextureType textureType = HdTextureType::Uv);

    HD_API
    ~HdMaterialParam();

    /// Computes a hash for all parameters. This hash also includes 
    /// parameter connections (texture, primvar, etc).
    HD_API
    static ID ComputeHash(HdMaterialParamVector const &shaders);

    TfToken const& GetName() const { return _name; }

    ParamType GetParamType() const { return _paramType; }

    HD_API
    HdTupleType GetTupleType() const;

    VtValue const& GetFallbackValue() const { return _fallbackValue; }

    SdfPath const& GetConnection() const { return _connection; }

    bool IsTexture() const {
        return GetParamType() == ParamTypeTexture;
    }
    bool IsPrimvar() const {
        return GetParamType() == ParamTypePrimvar;
    }
    bool IsFallback() const {
        return GetParamType() == ParamTypeFallback;
    }

    HdTextureType GetTextureType() const {
        return _textureType;
    }

    HD_API
    TfTokenVector const& GetSamplerCoordinates() const;

private:
    ParamType _paramType;
    TfToken _name;
    VtValue _fallbackValue;
    SdfPath _connection;
    TfTokenVector _samplerCoords;
    HdTextureType _textureType;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_MATERIAL_PARAM_H
