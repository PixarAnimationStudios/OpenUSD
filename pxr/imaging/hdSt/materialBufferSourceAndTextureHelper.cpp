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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hdSt/materialBufferSourceAndTextureHelper.h"

#include "pxr/imaging/hdSt/textureResource.h"
#include "pxr/imaging/hdSt/textureResourceHandle.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/glf/contextCaps.h"

PXR_NAMESPACE_OPEN_SCOPE

void
HdSt_MaterialBufferSourceAndTextureHelper::ProcessPrimvarMaterialParam(
    HdMaterialParam const &param)
{
    sources.push_back(
        boost::make_shared<HdVtBufferSource>(
            param.name, param.fallbackValue));
}

void
HdSt_MaterialBufferSourceAndTextureHelper::ProcessFallbackMaterialParam(
    HdMaterialParam const &param,
    VtValue const &fallbackValue)
{
    sources.push_back(
        boost::make_shared<HdVtBufferSource>(
            param.name, fallbackValue));
}

namespace {

// A bindless GL sampler buffer.
// This identifies a texture as a 64-bit handle, passed to GLSL as "uvec2".
// See https://www.khronos.org/opengl/wiki/Bindless_Texture
class HdSt_BindlessSamplerBufferSource : public HdBufferSource {
public:
    HdSt_BindlessSamplerBufferSource(TfToken const &name,
                                     size_t value)
    : HdBufferSource()
    , _name(name)
    , _value(value)
    {
        if (_value == 0) {
            TF_CODING_ERROR("Invalid texture handle: %s: %ld\n",
                            name.GetText(), value);
        }
    }

    ~HdSt_BindlessSamplerBufferSource() override = default;

    TfToken const &GetName() const override {
        return _name;
    }
    void const* GetData() const override {
        return &_value;
    }
    HdTupleType GetTupleType() const override {
        return { HdTypeUInt32Vec2, 1 };
    }
    size_t GetNumElements() const override {
        return 1;
    }
    void GetBufferSpecs(HdBufferSpecVector *specs) const override {
        specs->emplace_back(_name, GetTupleType());
    }
    bool Resolve() override {
        if (!_TryLock()) return false;
        _SetResolved();
        return true;
    }

protected:
    bool _CheckValid() const override {
        return true;
    }

private:
    const TfToken _name;
    const size_t _value;
};

}

void
HdSt_MaterialBufferSourceAndTextureHelper::ProcessTextureMaterialParam(
    HdMaterialParam const &param,
    HdStTextureResourceHandleSharedPtr const &handle,
    bool * isPtex)
{
    if (!(handle && handle->GetTextureResource())) {
        // we were unable to get the requested resource or
        // fallback resource so skip this param
        // (Error already posted).
        return;
    }
    
    HdStTextureResourceSharedPtr texResource =
        handle->GetTextureResource();

    bool bindless = GlfContextCaps::GetInstance()
        .bindlessTextureEnabled;

    HdStShaderCode::TextureDescriptor tex;
    tex.name = param.name;
    tex.handle = handle;

    const HdTextureType textureType = texResource->GetTextureType();
    if (textureType == HdTextureType::Ptex) {
        if (isPtex) {
            *isPtex = true;
        }
        tex.type =
            HdStShaderCode::TextureDescriptor::TEXTURE_PTEX_TEXEL;
        textures.push_back(tex);

        if (bindless) {
            sources.push_back(
                boost::make_shared<HdSt_BindlessSamplerBufferSource>(
                    tex.name,
                    texResource->GetTexelsTextureHandle()));
        }
        
        tex.name =
            TfToken(param.name.GetString() + "_layout");
        tex.type =
            HdStShaderCode::TextureDescriptor::TEXTURE_PTEX_LAYOUT;
        textures.push_back(tex);
        
        if (bindless) {
            sources.push_back(
                boost::make_shared<HdSt_BindlessSamplerBufferSource>(
                    tex.name,
                    texResource->GetLayoutTextureHandle()));
        }
    } else if (textureType == HdTextureType::Udim) {
        tex.type = HdStShaderCode::TextureDescriptor::TEXTURE_UDIM_ARRAY;
        textures.push_back(tex);
        
        if (bindless) {
            sources.push_back(
                boost::make_shared<HdSt_BindlessSamplerBufferSource>(
                    tex.name,
                    texResource->GetTexelsTextureHandle()));
        }
        
        tex.name =
            TfToken(param.name.GetString() + "_layout");
        tex.type =
            HdStShaderCode::TextureDescriptor::TEXTURE_UDIM_LAYOUT;
        textures.push_back(tex);
        
        if (bindless) {
            sources.push_back(
                boost::make_shared<HdSt_BindlessSamplerBufferSource>(
                    tex.name,
                    texResource->GetLayoutTextureHandle()));
        }
    } else if (textureType == HdTextureType::Uv) {
        tex.type = HdStShaderCode::TextureDescriptor::TEXTURE_2D;
        textures.push_back(tex);
        
        if (bindless) {
            sources.push_back(
                boost::make_shared<HdSt_BindlessSamplerBufferSource>(
                    tex.name,
                    texResource->GetTexelsTextureHandle()));
        }
    } else if (textureType == HdTextureType::Uvw) {
        tex.type = HdStShaderCode::TextureDescriptor::TEXTURE_3D;
        textures.push_back(tex);
        
        if (bindless) {
            sources.push_back(
                boost::make_shared<HdSt_BindlessSamplerBufferSource>(
                    tex.name,
                    texResource->GetTexelsTextureHandle()));
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
