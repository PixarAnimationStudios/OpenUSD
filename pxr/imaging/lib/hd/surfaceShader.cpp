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

#include "pxr/imaging/hd/surfaceShader.h"

#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/resource.h"
#include "pxr/imaging/hd/resourceBinder.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

class Hd_BindlessSamplerBufferSource : public HdBufferSource {
public:
    Hd_BindlessSamplerBufferSource(TfToken const &name, GLenum type, size_t value)
        : _name(name), _type(type), _value(value) {
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
        // note: we use sampler enums to express bindless pointer (somewhat unusual)
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


HdSurfaceShader::HdSurfaceShader(SdfPath const& id)
 : _id(id)
{
}

HdSurfaceShader::~HdSurfaceShader()
{
}

void
HdSurfaceShader::Sync(HdSceneDelegate *sceneDelegate)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(sceneDelegate != nullptr)) {
        return;  
    }

    SdfPath const& id = GetID();
    HdResourceRegistry *resourceRegistry = &HdResourceRegistry::GetInstance();
    HdChangeTracker& changeTracker = 
                            sceneDelegate->GetRenderIndex().GetChangeTracker();
    HdChangeTracker::DirtyBits bits = changeTracker.GetShaderDirtyBits(id);

    if(bits & HdChangeTracker::DirtySurfaceShader) {
        _fragmentSource = sceneDelegate->GetSurfaceShaderSource(id);
        _geometrySource = sceneDelegate->GetDisplacementShaderSource(id);

        // XXX Forcing collections to be dirty to reload everything
        //     Something more efficient can be done here
        changeTracker.MarkAllCollectionsDirty();
    }

    if(bits & HdChangeTracker::DirtyParams) {
        HdBufferSourceVector sources;
        TextureDescriptorVector textures;
        _params = sceneDelegate->GetSurfaceShaderParams(id);
        TF_FOR_ALL(paramIt, _params) {
            if (paramIt->IsPrimvar()) {
                // skip -- maybe not necessary, but more memory efficient
                continue;
            } else if (paramIt->IsFallback()) {
                VtValue paramVt =
                        sceneDelegate->GetSurfaceShaderParamValue(id,
                                                            paramIt->GetName());
                HdBufferSourceSharedPtr source(
                             new HdVtBufferSource(paramIt->GetName(), paramVt));

                sources.push_back(source);
            } else if (paramIt->IsTexture()) {
                bool bindless = HdRenderContextCaps::GetInstance()
                                                        .bindlessTextureEnabled;
                // register bindless handle

                HdTextureResource::ID texID =
                  sceneDelegate->GetTextureResourceID(paramIt->GetConnection());

                HdTextureResourceSharedPtr texResource;
                {
                    HdInstance<HdTextureResource::ID, HdTextureResourceSharedPtr> 
                        texInstance;

                    bool textureResourceFound = false;
                    std::unique_lock<std::mutex> regLock =
                        resourceRegistry->FindTextureResource
                        (texID, &texInstance, &textureResourceFound);
                    if (!TF_VERIFY(textureResourceFound, 
                            "No texture resource found with path %s",
                            paramIt->GetConnection().GetText())) {
                        continue;
                    }

                    texResource = texInstance.GetValue();
                    if (!TF_VERIFY(texResource, 
                            "Incorrect texture resource with path %s",
                            paramIt->GetConnection().GetText())) {
                        continue;
                    }
                }

                TextureDescriptor tex;
                tex.name = paramIt->GetName();

                if (texResource->IsPtex()) {
                    tex.type = TextureDescriptor::TEXTURE_PTEX_TEXEL;
                    tex.handle = bindless ? texResource->GetTexelsTextureHandle()
                                          : texResource->GetTexelsTextureId();
                    textures.push_back(tex);

                    if (bindless) {
                        HdBufferSourceSharedPtr source(new Hd_BindlessSamplerBufferSource(
                                                           tex.name,
                                                           GL_SAMPLER_2D_ARRAY,
                                                           tex.handle));
                        sources.push_back(source);
                    }

                    // layout

                    tex.name = TfToken(std::string(paramIt->GetName().GetText()) + "_layout");
                    tex.type = TextureDescriptor::TEXTURE_PTEX_LAYOUT;
                    tex.handle = bindless ? texResource->GetLayoutTextureHandle()
                                          : texResource->GetLayoutTextureId();
                    textures.push_back(tex);

                    if (bindless) {
                        HdBufferSourceSharedPtr source(new Hd_BindlessSamplerBufferSource(
                                                           tex.name,
                                                           GL_INT_SAMPLER_BUFFER,
                                                           tex.handle));
                        sources.push_back(source);
                    }
                } else {
                    tex.type = TextureDescriptor::TEXTURE_2D;
                    tex.handle = bindless ? texResource->GetTexelsTextureHandle()
                                          : texResource->GetTexelsTextureId();
                    tex.sampler =  texResource->GetTexelsSamplerId();
                    textures.push_back(tex);

                    if (bindless) {
                        HdBufferSourceSharedPtr source(new Hd_BindlessSamplerBufferSource(
                                                           tex.name,
                                                           GL_SAMPLER_2D,
                                                           tex.handle));
                        sources.push_back(source);
                    }
                }
            }
        }

        _textureDescriptors = textures;

        // return before allocation if it's empty.                                   
        if (sources.empty())                                                         
            return;

        // Allocate a new uniform buffer if not exists.
        if (!_paramArray) {
            // establish a buffer range
            HdBufferSpecVector bufferSpecs;
            TF_FOR_ALL(srcIt, sources) {
                (*srcIt)->AddBufferSpecs(&bufferSpecs);
            }
            
            HdBufferArrayRangeSharedPtr range =
                            resourceRegistry->AllocateShaderStorageBufferArrayRange(
                                        HdTokens->surfaceShaderParams, bufferSpecs);
            if (!TF_VERIFY(range->IsValid()))
                return;
            _paramArray = range;
        }

        if (!(TF_VERIFY(_paramArray->IsValid())))
            return;

        resourceRegistry->AddSources(_paramArray, sources);
    }
}

void
HdSurfaceShader::_SetSource(TfToken const &shaderStageKey, std::string const &source)
{
    if (shaderStageKey == HdShaderTokens->fragmentShader) {
        _fragmentSource = source;
    } else if (shaderStageKey == HdShaderTokens->geometryShader) {
        _geometrySource = source;
    }
}

// -------------------------------------------------------------------------- //
// HdShader Virtual Interface                                                 //
// -------------------------------------------------------------------------- //

/*virtual*/
std::string
HdSurfaceShader::GetSource(TfToken const &shaderStageKey) const
{
    if (shaderStageKey == HdShaderTokens->fragmentShader) {
        return _fragmentSource;
    } else if (shaderStageKey == HdShaderTokens->geometryShader) {
        return _geometrySource;
    }

    return std::string();
}
/*virtual*/
HdShaderParamVector const&
HdSurfaceShader::GetParams() const
{
    return _params;
}
/*virtual*/
HdBufferArrayRangeSharedPtr const&
HdSurfaceShader::GetShaderData() const
{
    return _paramArray;
}
/*virtual*/
HdShader::TextureDescriptorVector 
HdSurfaceShader::GetTextures() const
{
    return _textureDescriptors;
}
/*virtual*/
void
HdSurfaceShader::BindResources(Hd_ResourceBinder const &binder, int program)
{
    // XXX: there's an issue where other shaders try to use textures.
    int samplerUnit = binder.GetNumReservedTextureUnits();
    TF_FOR_ALL(it, _textureDescriptors) {
        HdBinding binding = binder.GetBinding(it->name);
        // XXX: put this into resource binder.
        if (binding.GetType() == HdBinding::TEXTURE_2D) {
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_2D, (GLuint)it->handle);
            glBindSampler(samplerUnit, it->sampler);
            
            glProgramUniform1i(program, binding.GetLocation(), samplerUnit);
            samplerUnit++;
        } else if (binding.GetType() == HdBinding::TEXTURE_PTEX_TEXEL) {
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_2D_ARRAY, (GLuint)it->handle);

            glProgramUniform1i(program, binding.GetLocation(), samplerUnit);
            samplerUnit++;
        } else if (binding.GetType() == HdBinding::TEXTURE_PTEX_LAYOUT) {
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_BUFFER, (GLuint)it->handle);

            glProgramUniform1i(program, binding.GetLocation(), samplerUnit);
            samplerUnit++;
        }
    }
    glActiveTexture(GL_TEXTURE0);
    binder.BindShaderResources(this);
}
/*virtual*/
void
HdSurfaceShader::UnbindResources(Hd_ResourceBinder const &binder, int program)
{
    binder.UnbindShaderResources(this);

    int samplerUnit = binder.GetNumReservedTextureUnits();
    TF_FOR_ALL(it, _textureDescriptors) {
        HdBinding binding = binder.GetBinding(it->name);
        // XXX: put this into resource binder.
        if (binding.GetType() == HdBinding::TEXTURE_2D) {
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindSampler(samplerUnit, 0);
            samplerUnit++;
        } else if (binding.GetType() == HdBinding::TEXTURE_PTEX_TEXEL) {
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
            samplerUnit++;
        } else if (binding.GetType() == HdBinding::TEXTURE_PTEX_LAYOUT) {
            glActiveTexture(GL_TEXTURE0 + samplerUnit);
            glBindTexture(GL_TEXTURE_BUFFER, 0);
            samplerUnit++;
        }
    }
    glActiveTexture(GL_TEXTURE0);

}
/*virtual*/
void
HdSurfaceShader::AddBindings(HdBindingRequestVector *customBindings)
{
}

/*virtual*/
HdShader::ID
HdSurfaceShader::ComputeHash() const
{
    size_t hash = 0;
    
    TF_FOR_ALL(it, _params) {
        if (it->IsFallback())
            boost::hash_combine(hash, it->GetName().Hash());
    }
    boost::hash_combine(hash, 
        ArchHash(_fragmentSource.c_str(), _fragmentSource.size()));
    boost::hash_combine(hash, 
        ArchHash(_geometrySource.c_str(), _geometrySource.size()));
    return hash;
}

/*static*/
bool
HdSurfaceShader::CanAggregate(HdShaderSharedPtr const &shaderA,
                              HdShaderSharedPtr const &shaderB)
{
    bool bindlessTexture = HdRenderContextCaps::GetInstance()
                                                .bindlessTextureEnabled;

    // See if the shaders are same or not. If the bindless texture option
    // is enabled, the shaders can be aggregated for those differences are
    // only texture addresses.
    if (bindlessTexture) {
        if (shaderA->ComputeHash() != shaderB->ComputeHash()) {
            return false;
        }
    } else {
        // XXX: still wrong. it breaks batches for the shaders with same
        // signature.
        if (shaderA != shaderB) {
            return false;
        }
    }
    return true;
}
