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
#ifndef HDST_MATERIAL_BUFFER_SOURCE_AND_TEXTURE_HELPER_H
#define HDST_MATERIAL_BUFFER_SOURCE_AND_TEXTURE_HELPER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/materialParam.h"
#include "pxr/imaging/hdSt/shaderCode.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;

struct HdSt_MaterialBufferSourceAndTextureHelper
{
    void ProcessPrimvarMaterialParam(
        HdMaterialParam const &param);
    void ProcessFallbackMaterialParam(
        HdMaterialParam const &param,
        HdSceneDelegate *sceneDelegate,
        SdfPath const &materialId);
    void ProcessTextureMaterialParam(
        HdMaterialParam const &param,
        HdStTextureResourceHandleSharedPtr const &handle,
        bool * isPtex = nullptr);

    HdBufferSourceVector sources;
    HdStShaderCode::TextureDescriptorVector textures;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
