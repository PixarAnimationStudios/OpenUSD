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
#ifndef HDST_MESH_SHADER_KEY_H
#define HDST_MESH_SHADER_KEY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/garch/gl.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE


struct HdSt_MeshShaderKey
{
    HdSt_MeshShaderKey(GLenum primitiveMode,
                       bool lit,
                       bool smoothNormals,
                       bool doubleSided,
                       bool faceVarying,
                       bool blendWireframeColor,
                       HdCullStyle cullStyle,
                       HdMeshGeomStyle geomStyle);

    // Note: it looks like gcc 4.8 has a problem issuing
    // a wrong warning as "array subscript is above array bounds"
    // when the default destructor is automatically generated at callers.
    // Having an empty destructor explicitly within this linkage apparently
    // avoids the issue.
    ~HdSt_MeshShaderKey();

    TfToken const &GetGlslfxFile() const { return glslfx; }
    TfToken const *GetVS() const  { return VS; }
    TfToken const *GetTCS() const { return TCS; }
    TfToken const *GetTES() const { return TES; }
    TfToken const *GetGS() const  { return GS; }
    TfToken const *GetFS() const  { return FS; }
    bool IsCullingPass() const { return false; }
    GLenum GetPrimitiveMode() const { return primitiveMode; }
    int16_t GetPrimitiveIndexSize() const { return primitiveIndexSize; }
    HdCullStyle GetCullStyle() const { return cullStyle; }
    HdPolygonMode GetPolygonMode() const { return polygonMode; }
    GLenum primitiveMode;
    int16_t primitiveIndexSize;
    HdCullStyle cullStyle;
    HdPolygonMode polygonMode;
    TfToken glslfx;
    TfToken VS[4];
    TfToken TCS[3];
    TfToken TES[3];
    TfToken GS[5];
    TfToken FS[7];
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDST_MESH_SHADER_KEY_H
