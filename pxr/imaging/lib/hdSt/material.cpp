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

#include "pxr/imaging/hdSt/material.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/imaging/hdSt/surfaceShader.h"
#include "pxr/imaging/hdSt/textureResource.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/glf/contextCaps.h"
#include "pxr/imaging/glf/textureHandle.h"
#include "pxr/imaging/glf/textureRegistry.h"
#include "pxr/imaging/glf/uvTextureStorage.h"

#include "pxr/base/tf/staticTokens.h"

#include <boost/pointer_cast.hpp>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (limitSurfaceEvaluation)
);

// A bindless GL sampler buffer.
// This identifies a texture as a 64-bit handle, passed to GLSL as "uvec2".
// See https://www.khronos.org/opengl/wiki/Bindless_Texture
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
    virtual HdTupleType GetTupleType() const {
        return {HdTypeUInt32Vec2, 1};
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
    virtual void GetBufferSpecs(HdBufferSpecVector *specs) const {
        specs->emplace_back(_name, GetTupleType());
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

HdStMaterial::HdStMaterial(SdfPath const &id)
 : HdMaterial(id)
 , _surfaceShader(new HdStSurfaceShader)
 , _hasPtex(false)
 , _hasLimitSurfaceEvaluation(false)
{
}

HdStMaterial::~HdStMaterial()
{
}

/* virtual */
void
HdStMaterial::Sync(HdSceneDelegate *sceneDelegate,
                 HdRenderParam   *renderParam,
                 HdDirtyBits     *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TF_UNUSED(renderParam);

    HdResourceRegistrySharedPtr const &resourceRegistry = 
        sceneDelegate->GetRenderIndex().GetResourceRegistry();
    HdDirtyBits bits = *dirtyBits;

    bool needsRprimMaterialStateUpdate = false;

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

        bool hasLimitSurfaceEvaluation =
            _GetHasLimitSurfaceEvaluation(GetMaterialMetadata(sceneDelegate));

        if (_hasLimitSurfaceEvaluation != hasLimitSurfaceEvaluation) {
            _hasLimitSurfaceEvaluation = hasLimitSurfaceEvaluation;
            needsRprimMaterialStateUpdate = true;
        }

    }

    if(bits & DirtyParams) {
        HdBufferSourceVector sources;
        HdStShaderCode::TextureDescriptorVector textures;
        const HdMaterialParamVector &params = GetMaterialParams(sceneDelegate);
        _surfaceShader->SetParams(params);

        // Release any fallback texture resources
        _fallbackTextureResources.clear();

        bool hasPtex = false;
        for (HdMaterialParam const & param: params) {
            if (param.IsPrimvar()) {
                // skip -- maybe not necessary, but more memory efficient
                continue;
            } else if (param.IsFallback()) {
                VtValue paramVt = GetMaterialParamValue(sceneDelegate,
                                                        param.GetName());
                HdBufferSourceSharedPtr source(
                             new HdVtBufferSource(param.GetName(), paramVt));

                sources.push_back(source);
            } else if (param.IsTexture()) {

                HdStTextureResourceSharedPtr texResource =
                    _GetTextureResource(sceneDelegate, param);

                if (!texResource) {
                    // we were unable to get the requested resource or
                    // fallback resource so skip this param
                    // (Error already posted).
                    continue;
                }

                bool bindless = GlfContextCaps::GetInstance()
                                                        .bindlessTextureEnabled;
                // register bindless handle

                HdStShaderCode::TextureDescriptor tex;
                tex.name = param.GetName();

                if (texResource->IsPtex()) {
                    hasPtex = true;
                    tex.type =
                        HdStShaderCode::TextureDescriptor::TEXTURE_PTEX_TEXEL;
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
                       TfToken(param.GetName().GetString() + "_layout");
                    tex.type =
                       HdStShaderCode::TextureDescriptor::TEXTURE_PTEX_LAYOUT;
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
                    tex.type = HdStShaderCode::TextureDescriptor::TEXTURE_2D;
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

        if (_hasPtex != hasPtex) {
            _hasPtex = hasPtex;
            needsRprimMaterialStateUpdate = true;
        }
    }

    if (needsRprimMaterialStateUpdate) {
        // XXX Forcing rprims to have a dirty material id to re-evaluate
        // their material state as we don't know which rprims are bound to
        // this one.
        HdChangeTracker& changeTracker =
                         sceneDelegate->GetRenderIndex().GetChangeTracker();
        changeTracker.MarkAllRprimsDirty(HdChangeTracker::DirtyMaterialId);
    }

    *dirtyBits = Clean;
}

HdStTextureResourceSharedPtr
HdStMaterial::_GetTextureResource(
        HdSceneDelegate *sceneDelegate,
        HdMaterialParam const &param)
{
    HdResourceRegistrySharedPtr const &resourceRegistry = 
        sceneDelegate->GetRenderIndex().GetResourceRegistry();

    // Create a fallback texture resource when the texture doesn't
    // have a valid connection path or if the scene delegate returns
    // and invalidate ID
    bool useFallback = param.GetConnection().IsEmpty();

    HdTextureResource::ID texID = HdTextureResource::ID(-1);

    if (!param.GetConnection().IsEmpty()) {
         texID = GetTextureResourceID(sceneDelegate, param.GetConnection());
    }
    useFallback |= (texID == HdTextureResource::ID(-1));

    // XXX todo handle fallback Ptex textures
    HdStTextureResourceSharedPtr texResource;
    if (useFallback) {
        GlfUVTextureStorageRefPtr texPtr = 
            GlfUVTextureStorage::New(1,1, param.GetFallbackValue());
        GlfTextureHandleRefPtr texture =
            GlfTextureRegistry::GetInstance().GetTextureHandle(texPtr);
        texResource.reset(
            new HdStSimpleTextureResource(texture,
                                          false,
                                          HdWrapClamp,
                                          HdWrapClamp,
                                          HdMinFilterNearest,
                                          HdMagFilterNearest,
                                          0));
        _fallbackTextureResources.push_back(texResource);
    } else {
        HdInstance<HdTextureResource::ID,
                   HdTextureResourceSharedPtr> texInstance;

        bool textureResourceFound = false;
        std::unique_lock<std::mutex> regLock =
            resourceRegistry->FindTextureResource
            (texID, &texInstance, &textureResourceFound);
        // A bad asset can cause the texture resource to not 
        // be found. Hence, issue a warning and continue onto the 
        // next param.
        if (!textureResourceFound) {
            TF_WARN("No texture resource found with path %s",
                param.GetConnection().GetText());
            return HdStTextureResourceSharedPtr();
        }

        texResource =
            boost::dynamic_pointer_cast<HdStTextureResource>
            (texInstance.GetValue());
        if (!TF_VERIFY(texResource,
                "Incorrect texture resource with path %s",
                param.GetConnection().GetText())) {
            return HdStTextureResourceSharedPtr();
        }
    }

    return texResource;
}

bool
HdStMaterial::_GetHasLimitSurfaceEvaluation(VtDictionary const & metadata) const
{
    VtValue value = TfMapLookupByValue(metadata,
                                       _tokens->limitSurfaceEvaluation,
                                       VtValue());
    return value.IsHolding<bool>() && value.Get<bool>();
}

// virtual
HdDirtyBits
HdStMaterial::GetInitialDirtyBitsMask() const
{
    return AllDirty;
}


//virtual
void
HdStMaterial::Reload()
{
    _surfaceShader->Reload();
}

HdStShaderCodeSharedPtr
HdStMaterial::GetShaderCode() const
{
    return boost::static_pointer_cast<HdStShaderCode>(_surfaceShader);
}

void
HdStMaterial::SetSurfaceShader(HdStSurfaceShaderSharedPtr &shaderCode)
{
    _surfaceShader = shaderCode;
}

PXR_NAMESPACE_CLOSE_SCOPE
