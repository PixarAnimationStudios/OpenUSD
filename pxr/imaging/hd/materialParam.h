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
#ifndef PXR_IMAGING_HD_MATERIAL_PARAM_H
#define PXR_IMAGING_HD_MATERIAL_PARAM_H

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
        ParamTypeTexture,
        // This is a parameter that is connected to a field reader.
        ParamTypeField,
        // Accesses 3d texture with potential transform and fallback under
        // different name
        ParamTypeFieldRedirect,
        // Additional primvar needed by material. One that is not connected to
        // a input parameter (ParamTypePrimvar).
        ParamTypeAdditionalPrimvar
    };

    HD_API
    HdMaterialParam();

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

    HD_API
    HdTupleType GetTupleType() const;

    bool IsField() const {
        return paramType == ParamTypeField;
    }
    bool IsTexture() const {
        return paramType == ParamTypeTexture;
    }
    bool IsPrimvar() const {
        return paramType == ParamTypePrimvar;
    }
    bool IsFallback() const {
        return paramType == ParamTypeFallback;
    }
    bool IsFieldRedirect() const {
        return paramType == ParamTypeFieldRedirect;
    }
    bool IsAdditionalPrimvar() const {
        return paramType == ParamTypeAdditionalPrimvar;
    }

    ParamType paramType;
    TfToken name;
    VtValue fallbackValue;
    SdfPath connection;
    TfTokenVector samplerCoords;
    HdTextureType textureType;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_MATERIAL_PARAM_H
