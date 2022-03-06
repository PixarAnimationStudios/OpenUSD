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
#include "pxr/imaging/hdSt/cullingShaderKey.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


namespace pxrImagingHdStCullingShaderKey {

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((baseGLSLFX,       "frustumCull.glslfx"))
    ((instancing,       "Instancing.Transform"))
    ((counting,         "ViewFrustumCull.Counting"))
    ((noCounting,       "ViewFrustumCull.NoCounting"))
    ((tinyCull,         "ViewFrustumCull.TinyCull"))
    ((noTinyCull,       "ViewFrustumCull.NoTinyCull"))
    ((isVisible,        "ViewFrustumCull.IsVisible"))
    ((mainInstancingVS, "ViewFrustumCull.VertexInstancing"))
    ((mainVS,           "ViewFrustumCull.Vertex"))
);

} // pxrImagingHdStCullingShaderKey

HdSt_CullingShaderKey::HdSt_CullingShaderKey(
    bool instancing, bool tinyCull, bool counting)
    : glslfx(pxrImagingHdStCullingShaderKey::_tokens->baseGLSLFX)
{

    VS[0] = pxrImagingHdStCullingShaderKey::_tokens->instancing;
    VS[1] = counting ? pxrImagingHdStCullingShaderKey::_tokens->counting : pxrImagingHdStCullingShaderKey::_tokens->noCounting;
    VS[2] = tinyCull ? pxrImagingHdStCullingShaderKey::_tokens->tinyCull : pxrImagingHdStCullingShaderKey::_tokens->noTinyCull;
    VS[3] = pxrImagingHdStCullingShaderKey::_tokens->isVisible;
    VS[4] = instancing ? pxrImagingHdStCullingShaderKey::_tokens->mainInstancingVS : pxrImagingHdStCullingShaderKey::_tokens->mainVS;
    VS[5] = TfToken();
}

HdSt_CullingShaderKey::~HdSt_CullingShaderKey()
{
}

PXR_NAMESPACE_CLOSE_SCOPE

