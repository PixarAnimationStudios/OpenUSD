//
// Copyright 2019 Pixar
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
#ifndef PXR_IMAGING_HD_ST_MATERIAL_PARAM_H
#define PXR_IMAGING_HD_ST_MATERIAL_PARAM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/tf/token.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

using HdSt_MaterialParamVector = std::vector<class HdSt_MaterialParam>;

// XXX: Docs
class HdSt_MaterialParam final {
public:
    typedef size_t ID;

    // Indicates the kind of material parameter.
    enum ParamType {
        // This is a shader specified fallback value that is
        // not connected to either a primvar or texture.
        ParamTypeFallback,
        // This is a parameter that is connected to a texture.
        ParamTypeTexture,
        // Creates an accessor HdGet_name() that either reads a
        // primvar with a potentially different name (given in
        // samplerCoords) if it exists or uses the fallback value.
        // It corresponds to a primvar reader shading node.
        ParamTypePrimvarRedirect,
        // Creates an accessor HdGet_name(vec3) that either reads
        // from a field texture with a potentially different name (given
        // in samplerCoords) if it exists or uses the fallback value.
        // It corresponds to a field reader shading node.
        ParamTypeFieldRedirect,
        // Additional primvar needed by material. One that is not connected to
        // a input parameter (ParamTypePrimvar).
        ParamTypeAdditionalPrimvar,
        // This is a parameter that is connected to a transform2d node
        ParamTypeTransform2d
    };

    HDST_API
    HdSt_MaterialParam();

    HDST_API
    HdSt_MaterialParam(ParamType paramType,
                    TfToken const& name, 
                    VtValue const& fallbackValue,
                    TfTokenVector const& samplerCoords=TfTokenVector(),
                    HdTextureType textureType=HdTextureType::Uv,
                    std::string const& swizzle=std::string(),
                    bool const isPremultiplied=false);

    // No d'tor so that we pick up the implicitly declared default
    // move c'tor.

    /// Computes a hash for all parameters using structural information
    /// (name, texture type, primvar names) but not the fallback value.
    HDST_API
    static ID ComputeHash(HdSt_MaterialParamVector const &shaders);

    HDST_API
    HdTupleType GetTupleType() const;

    bool IsTexture() const {
        return paramType == ParamTypeTexture;
    }
    bool IsPrimvarRedirect() const {
        return paramType == ParamTypePrimvarRedirect;
    }
    bool IsFieldRedirect() const {
        return paramType == ParamTypeFieldRedirect;
    }
    bool IsFallback() const {
        return paramType == ParamTypeFallback;
    }
    bool IsAdditionalPrimvar() const {
        return paramType == ParamTypeAdditionalPrimvar;
    }
    bool IsTransform2d() const {
        return paramType == ParamTypeTransform2d;
    }

    ParamType paramType;
    TfToken name;
    VtValue fallbackValue;
    TfTokenVector samplerCoords;
    HdTextureType textureType;
    std::string swizzle;
    bool isPremultiplied;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_ST_MATERIAL_PARAM_H
