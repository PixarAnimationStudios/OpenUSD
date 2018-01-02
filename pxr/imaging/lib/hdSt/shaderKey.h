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
#ifndef HDST_SHADER_KEY_H
#define HDST_SHADER_KEY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE


// This is a static utility class to interpret prim specific shaderKeys.
//
struct HdStShaderKey {
    typedef size_t ID;

    // This hash is being used to distinguish GeometricShader instance,
    // so that in order to break batches and interleave GL rasterization
    // state changes appropriately.
    // Note that the GLSL programs still can be shared across GeometricShader
    // instances, when they are identical except the GL states, as long as
    // Hd_GeometricShader::ComputeHash() provides consistent hash values.
    //
    // XXX: we may want to rename Hd_GeometricShader::ComputeHash to
    //      ComputeProgramHash or something to avoid this confusion.
    //
    template <typename KEY>
    static ID ComputeHash(KEY const &key) {
        return ComputeHash(key.GetGlslfxFile(),
                           key.GetVS(),
                           key.GetTCS(),
                           key.GetTES(),
                           key.GetGS(),
                           key.GetFS(),
                           static_cast<int16_t> (key.GetPrimitiveType()), // class enum type
                           key.GetCullStyle(),
                           key.GetPolygonMode(),
                           key.IsCullingPass(),
                           key.IsFaceVarying());
    }

    HDST_API
    static ID ComputeHash(TfToken const &glslfxFile,
                          TfToken const *VS,
                          TfToken const *TCS,
                          TfToken const *TES,
                          TfToken const *GS,
                          TfToken const *FS,
                          int16_t primType,
                          HdCullStyle cullStyle,
                          HdPolygonMode polygonMode,
                          bool isCullingPass,
                          bool IsFaceVarying);

    template <typename KEY>
    static std::string GetGLSLFXString(KEY const &key) {
        return GetGLSLFXString(key.GetGlslfxFile(),
                               key.GetVS(),
                               key.GetTCS(),
                               key.GetTES(),
                               key.GetGS(),
                               key.GetFS());
    }

    HDST_API
    static std::string GetGLSLFXString(TfToken const &glslfxFile,
                                       TfToken const *VS,
                                       TfToken const *TCS,
                                       TfToken const *TES,
                                       TfToken const *GS,
                                       TfToken const *FS);

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDST_SHADER_KEY_H
