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
#ifndef PXR_IMAGING_HD_ST_SHADER_KEY_H
#define PXR_IMAGING_HD_ST_SHADER_KEY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/geometricShader.h" // XXX: for PrimitiveType
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE


// Abstract interface for geometric shader keys that may be used to
// construct a geometric shader.
struct HdSt_ShaderKey {
    typedef size_t ID;

    HDST_API
    virtual ~HdSt_ShaderKey();

    // The hash computed identifies each geometric shader instance, and is used
    // for deduplication in the resource registry.
    HDST_API
    ID ComputeHash() const;

    // Stitches the glslfx filename and the shader stage mixin names into
    // a string for consumption by HioGlslfx.
    HDST_API
    std::string GetGlslfxString() const;

    // -------------------------------------------------------------------------
    // Virtual interface
    // -------------------------------------------------------------------------

    // Return the name of the glslfx file that houses the entry point mixins
    // that define the main() function for the relevant shader stages.
    // The expectation is that this file includes the glslfx files that
    // define any functions it uses.
    virtual TfToken const &GetGlslfxFilename() const = 0;

    // Each shader stage specifies the various mixins to stitch together
    // via their token names. The Get* flavor of methods return the first
    // token in the array.
    HDST_API
    virtual TfToken const *GetVS() const;
    HDST_API
    virtual TfToken const *GetTCS() const;
    HDST_API
    virtual TfToken const *GetTES() const;
    HDST_API
    virtual TfToken const *GetGS() const;
    HDST_API
    virtual TfToken const *GetFS() const; 

    // An implementation detail of code gen, which generates slightly
    // different code for the VS stage for the frustum culling pass.
    HDST_API
    virtual bool IsFrustumCullingPass() const;
    
    // Returns the geometric shader primitive type that is used in code gen
    // and to figure out the primitive mode during draw submission.
    virtual HdSt_GeometricShader::PrimitiveType GetPrimitiveType() const = 0; 
    
    // Implementation details of the geometric shader that sets hardware
    // pipeline state (cull face, polygon mode, line width) or queues upload of
    // data (cullstyle) to the GPU.
    HDST_API
    virtual HdCullStyle GetCullStyle() const;
    HDST_API
    virtual bool UseHardwareFaceCulling() const;
    HDST_API
    virtual bool HasMirroredTransform() const;
    HDST_API
    virtual bool IsDoubleSided() const;
    HDST_API
    virtual HdPolygonMode GetPolygonMode() const;
    HDST_API
    virtual float GetLineWidth() const;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_SHADER_KEY_H
