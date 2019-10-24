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
#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/volumeShaderKey.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((baseGLSLFX,         "volume.glslfx"))

    // point id mixins (provide functions for picking system)
    ((pointIdFS,          "PointId.Fragment.Fallback"))

    // main for all the shader stages
    ((mainVS,             "Volume.Vertex"))
    ((mainFS,             "Volume.Fragment"))

    // instancing       
    ((instancing,         "Instancing.Transform"))
);

HdSt_VolumeShaderKey::HdSt_VolumeShaderKey()
    : glslfx(_tokens->baseGLSLFX),
      VS{ _tokens->instancing, _tokens->mainVS, TfToken() },
      FS{ _tokens->pointIdFS, _tokens->instancing,
          _tokens->mainFS, TfToken() }
{
}

HdSt_VolumeShaderKey::~HdSt_VolumeShaderKey() = default;

PXR_NAMESPACE_CLOSE_SCOPE

