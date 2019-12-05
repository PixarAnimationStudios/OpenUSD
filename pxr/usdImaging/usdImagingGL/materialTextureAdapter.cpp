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
#include "pxr/usdImaging/usdImagingGL/materialTextureAdapter.h"
#include "pxr/usdImaging/usdImagingGL/textureUtils.h"
#include "pxr/imaging/glf/image.h"

#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/sdr/registry.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (inputs)
);


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingGLMaterialTextureAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingGLMaterialTextureAdapter::~UsdImagingGLMaterialTextureAdapter() 
{
}

HdTextureResourceSharedPtr
UsdImagingGLMaterialTextureAdapter::GetTextureResource(
    UsdPrim const& usdPrim,
    SdfPath const &id,
    UsdTimeCode time) const
{
    // The usdPrim we receive is the Material prim, since that is the prim we
    // inserted the SPrim and primInfo for. However, the texture is authored on
    // the texture prim, so we get the texture prim.
    UsdPrim texturePrim = _GetPrim(id);

    // Our texture loader wants the attribute that contains the file path.
    // So we change:
    //    /Materials/Woody/BootMaterial/Tex
    // into:
    //    /Materials/Woody/BootMaterial/Tex.inputs:file
    //
    // In order to do this correctly we need to consult Sdr to get the exact
    // name of the property that will provide the file path.

    SdfPath texAttr = id;
    UsdShadeShader shadeNode(texturePrim);

    if (shadeNode) {
        TfToken shaderId;
        shadeNode.GetShaderId(&shaderId);

        SdrRegistry& shaderReg = SdrRegistry::GetInstance();
        if (SdrShaderNodeConstPtr sdrNode = 
            shaderReg.GetShaderNodeByIdentifier(shaderId)) {
            NdrTokenVec const& assetIdentifierPropertyNames = 
                sdrNode->GetAssetIdentifierInputNames();
            if (assetIdentifierPropertyNames.size() == 1) {
                TfToken const& fileProp = assetIdentifierPropertyNames[0];
                texAttr = texAttr.AppendProperty(
                    TfToken(SdfPath::JoinIdentifier(
                       _tokens->inputs, fileProp)));
            }
        }
    }

    return UsdImagingGL_GetTextureResource(texturePrim, texAttr, time);
}

PXR_NAMESPACE_CLOSE_SCOPE
