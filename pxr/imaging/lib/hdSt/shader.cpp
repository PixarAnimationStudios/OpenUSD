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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hdSt/shader.h"
#include "pxr/imaging/hdSt/surfaceShader.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/textureResource.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/shaderCode.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include <boost/pointer_cast.hpp>

PXR_NAMESPACE_OPEN_SCOPE

class HdSt_BindlessSamplerBufferSource : public HdBufferSource {
public:
    HdSt_BindlessSamplerBufferSource(TfToken const &name,
                                     GLenum type,
                                     size_t value)
     : HdBufferSource()
     , _name(name)
     , _type(type)
     , _value(value)
    {
        if (_value == 0) {
            TF_CODING_ERROR("Invalid texture handle: %s: %ld\n",
                            name.GetText(), value);
        }
    }

    virtual TfToken const &GetName() const {
        return _name;
    }
    virtual void const* GetData() const {
        return &_value;
    }
    virtual int GetGLComponentDataType() const {
        // note: we use sampler enums to express bindless pointer
        // (somewhat unusual)
        return _type;
    }
    virtual int GetGLElementDataType() const {
        return GL_UNSIGNED_INT64_ARB;
    }
    virtual int GetNumElements() const {
        return 1;
    }
    virtual short GetNumComponents() const {
        return 1;
    }
    virtual void AddBufferSpecs(HdBufferSpecVector *specs) const {
        specs->push_back(HdBufferSpec(_name, _type, 1));
    }
    virtual bool Resolve() {
        if (!_TryLock()) return false;
        _SetResolved();
        return true;
    }

protected:
    virtual bool _CheckValid() const {
        return true;
    }

private:
    TfToken _name;
    GLenum _type;
    size_t _value;
};

HdStShader::HdStShader(SdfPath const &id)
 : HdShader(id)
 , _surfaceShader(new HdStSurfaceShader)
{
}

HdStShader::~HdStShader()
{
}

/* virtual */
void
HdStShader::Sync(HdSceneDelegate *sceneDelegate,
                 HdRenderParam   *renderParam,
                 HdDirtyBits     *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TF_UNUSED(renderParam);

    HdResourceRegistrySharedPtr const &resourceRegistry = 
        sceneDelegate->GetRenderIndex().GetResourceRegistry();
    HdDirtyBits bits = *dirtyBits;

    if(bits & DirtySurfaceShader) {
        const std::string &fragmentSource =
                GetSurfaceShaderSource(sceneDelegate);

        _surfaceShader->SetFragmentSource(fragmentSource);

        const std::string &geometrySource = 
                GetDisplacementShaderSource(sceneDelegate);

        _surfaceShader->SetGeometrySource(geometrySource);
        
        // XXX Forcing collections to be dirty to reload everything
        //     Something more efficient can be done here
        HdChangeTracker& changeTracker =
                             sceneDelegate->GetRenderIndex().GetChangeTracker();
        changeTracker.MarkAllCollectionsDirty();
    }

    if(bits & DirtyParams) {
        HdBufferSourceVector sources;
        HdShaderCode::TextureDescriptorVector textures;
        const HdShaderParamVector &params =
                                          GetSurfaceShaderParams(sceneDelegate);
        _surfaceShader->SetParams(params);

        TF_FOR_ALL(paramIt, params) {
            if (paramIt->IsPrimvar()) {
                // skip -- maybe not necessary, but more memory efficient
                continue;
            } else if (paramIt->IsFallback()) {
                VtValue paramVt = GetSurfaceShaderParamValue(sceneDelegate,
                                                            paramIt->GetName());
                HdBufferSourceSharedPtr source(
                             new HdVtBufferSource(paramIt->GetName(), paramVt));

                sources.push_back(source);
            } else if (paramIt->IsTexture()) {
                bool bindless = HdRenderContextCaps::GetInstance()
                                                        .bindlessTextureEnabled;
                // register bindless handle

                HdTextureResource::ID texID =
                                 GetTextureResourceID(sceneDelegate,
                                                      paramIt->GetConnection());

                HdStTextureResourceSharedPtr texResource;
                {
                    HdInstance<HdTextureResource::ID,
                               HdTextureResourceSharedPtr> texInstance;

                    bool textureResourceFound = false;
                    std::unique_lock<std::mutex> regLock =
                        resourceRegistry->FindTextureResource
                        (texID, &texInstance, &textureResourceFound);
                    if (!TF_VERIFY(textureResourceFound,
                            "No texture resource found with path %s",
                            paramIt->GetConnection().GetText())) {
                        continue;
                    }

                    texResource =
                        boost::dynamic_pointer_cast<HdStTextureResource>
                        (texInstance.GetValue());
                    if (!TF_VERIFY(texResource,
                            "Incorrect texture resource with path %s",
                            paramIt->GetConnection().GetText())) {
                        continue;
                    }
                }

                HdShaderCode::TextureDescriptor tex;
                tex.name = paramIt->GetName();

                if (texResource->IsPtex()) {
                    tex.type =
                            HdShaderCode::TextureDescriptor::TEXTURE_PTEX_TEXEL;
                    tex.handle =
                                bindless ? texResource->GetTexelsTextureHandle()
                                         : texResource->GetTexelsTextureId();
                    textures.push_back(tex);

                    if (bindless) {
                        HdBufferSourceSharedPtr source(
                                new HdSt_BindlessSamplerBufferSource(
                                                           tex.name,
                                                           GL_SAMPLER_2D_ARRAY,
                                                           tex.handle));
                        sources.push_back(source);
                    }

                    // layout

                    tex.name =
                            TfToken(paramIt->GetName().GetString() + "_layout");
                    tex.type =
                           HdShaderCode::TextureDescriptor::TEXTURE_PTEX_LAYOUT;
                    tex.handle =
                                bindless ? texResource->GetLayoutTextureHandle()
                                         : texResource->GetLayoutTextureId();
                    textures.push_back(tex);

                    if (bindless) {
                        HdBufferSourceSharedPtr source(
                                new HdSt_BindlessSamplerBufferSource(
                                                          tex.name,
                                                          GL_INT_SAMPLER_BUFFER,
                                                          tex.handle));
                        sources.push_back(source);
                    }
                } else {
                    tex.type = HdShaderCode::TextureDescriptor::TEXTURE_2D;
                    tex.handle =
                                bindless ? texResource->GetTexelsTextureHandle()
                                         : texResource->GetTexelsTextureId();
                    tex.sampler =  texResource->GetTexelsSamplerId();
                    textures.push_back(tex);

                    if (bindless) {
                        HdBufferSourceSharedPtr source(
                                new HdSt_BindlessSamplerBufferSource(
                                                           tex.name,
                                                           GL_SAMPLER_2D,
                                                           tex.handle));
                        sources.push_back(source);
                    }
                }
            }
        }

        _surfaceShader->SetTextureDescriptors(textures);
        _surfaceShader->SetBufferSources(sources, resourceRegistry);
    }

    *dirtyBits = Clean;
}

// virtual
VtValue
HdStShader::Get(TfToken const &token) const
{
    TF_CODING_ERROR("Unused Function");
    return VtValue();
}

// virtual
HdDirtyBits
HdStShader::GetInitialDirtyBitsMask() const
{
    return AllDirty;
}


//virtual
void
HdStShader::Reload()
{
    _surfaceShader->Reload();
}

// virtual
HdShaderCodeSharedPtr
HdStShader::GetShaderCode() const
{
    return boost::static_pointer_cast<HdShaderCode>(_surfaceShader);
}

void
HdStShader::SetSurfaceShader(HdStSurfaceShaderSharedPtr &shaderCode)
{
    _surfaceShader = shaderCode;
}

PXR_NAMESPACE_CLOSE_SCOPE
