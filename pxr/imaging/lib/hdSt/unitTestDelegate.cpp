//
// Copyright 2018 Pixar
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
#include "pxr/imaging/hdSt/unitTestDelegate.h"

#include "pxr/imaging/hd/basisCurves.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/points.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/texture.h"
#include "pxr/imaging/hdSt/textureResource.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/rotation.h"

#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/baseTexture.h"
#include "pxr/imaging/glf/textureRegistry.h"
#include "pxr/imaging/glf/ptexTexture.h"

PXR_NAMESPACE_OPEN_SCOPE

template <typename T>
static VtArray<T>
_BuildArray(T values[], int numValues)
{
    VtArray<T> result(numValues);
    std::copy(values, values+numValues, result.begin());
    return result;
}

HdSt_UnitTestDelegate::HdSt_UnitTestDelegate(HdRenderIndex *parentIndex,
                                         SdfPath const& delegateID)
  : HdUnitTestDelegate(parentIndex, delegateID)
{
}

HdSt_UnitTestDelegate::~HdSt_UnitTestDelegate()
{
}

void
HdSt_UnitTestDelegate::AddTexture(SdfPath const& id, 
                                  GlfTextureRefPtr const& texture)
{
    HdRenderIndex& index = GetRenderIndex();
    index.InsertBprim(HdPrimTypeTokens->texture, this, id);
    _textures[id] = _Texture(texture);
}

/*virtual*/
HdTextureResourceSharedPtr
HdSt_UnitTestDelegate::GetTextureResource(SdfPath const& textureId)
{
    GlfTextureHandleRefPtr texture =
        GlfTextureRegistry::GetInstance().GetTextureHandle(_textures[textureId].texture);

    // Simple way to detect if the glf texture is ptex or not
    HdTextureType textureType = HdTextureType::Uv;
#ifdef PXR_PTEX_SUPPORT_ENABLED
    GlfPtexTextureRefPtr pTex = 
        TfDynamic_cast<GlfPtexTextureRefPtr>(_textures[textureId].texture);
    if (pTex) {
        textureType = HdTextureType::Ptex;
    }
#endif

    return HdTextureResourceSharedPtr(
        new HdStSimpleTextureResource(texture,
                                      textureType,
                                      HdWrapUseMetadata,
                                      HdWrapUseMetadata,
                                      HdMinFilterNearestMipmapLinear,
                                      HdMagFilterLinear,
                                      0));
}

PXR_NAMESPACE_CLOSE_SCOPE

