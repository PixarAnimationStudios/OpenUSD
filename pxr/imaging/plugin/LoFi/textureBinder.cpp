//
// Copyright 2020 benmalartre
//
// Unlicensed 
//
#include "pxr/pxr.h"
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/plugin/LoFi/binding.h"
#include "pxr/imaging/plugin/LoFi/textureBinder.h"
#include "pxr/imaging/plugin/LoFi/ptexTextureObject.h"
#include "pxr/imaging/plugin/LoFi/samplerObject.h"
#include "pxr/imaging/plugin/LoFi/textureHandle.h"
#include "pxr/imaging/plugin/LoFi/textureObject.h"
#include "pxr/imaging/plugin/LoFi/udimTextureObject.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/hgiGL/texture.h"
#include "pxr/imaging/hgiGL/sampler.h"

PXR_NAMESPACE_OPEN_SCOPE

static const HdTupleType _bindlessHandleTupleType{ HdTypeUInt32Vec2, 1 };

static
TfToken
_Concat(const TfToken &a, const TfToken &b)
{
    return TfToken(a.GetString() + b.GetString());
}

void
LoFiTextureBinder::GetBufferSpecs(
    const NamedTextureHandleVector &textures,
    const bool useBindlessHandles,
    HdBufferSpecVector * const specs)
{
    for (const NamedTextureHandle & texture : textures) {
        switch (texture.type) {
        case HdTextureType::Uv:
            if (useBindlessHandles) {
                specs->emplace_back(
                    texture.name,
                    _bindlessHandleTupleType);
            } else {
                specs->emplace_back(
                    _Concat(
                        texture.name,
                        LoFiBindingSuffixTokens->valid),
                    HdTupleType{HdTypeBool, 1});
            }
            break;
        case HdTextureType::Field:
            if (useBindlessHandles) {
                specs->emplace_back(
                    texture.name,
                    _bindlessHandleTupleType);
            } else {
                specs->emplace_back(
                    _Concat(
                        texture.name,
                        LoFiBindingSuffixTokens->valid),
                    HdTupleType{HdTypeBool, 1});
            }
            specs->emplace_back(
                _Concat(
                    texture.name,
                    LoFiBindingSuffixTokens->samplingTransform),
                HdTupleType{HdTypeDoubleMat4, 1});
            break;
        case HdTextureType::Ptex:
            if (useBindlessHandles) {
                specs->emplace_back(
                    texture.name,
                    _bindlessHandleTupleType);
                specs->emplace_back(
                    _Concat(
                        texture.name,
                        LoFiBindingSuffixTokens->layout),
                    _bindlessHandleTupleType);
            }
            break;
        case HdTextureType::Udim:
            if (useBindlessHandles) {
                specs->emplace_back(
                    texture.name,
                    _bindlessHandleTupleType);
                specs->emplace_back(
                    _Concat(
                        texture.name,
                        LoFiBindingSuffixTokens->layout),
                    _bindlessHandleTupleType);
            }
            break;
        }
    }
}

namespace {

// A bindless GL sampler buffer.
// This identifies a texture as a 64-bit handle, passed to GLSL as "uvec2".
// See https://www.khronos.org/opengl/wiki/Bindless_Texture
class LoFiBindlessSamplerBufferSource : public HdBufferSource {
public:
    LoFiBindlessSamplerBufferSource(TfToken const &name,
                                    const GLuint64EXT value)
    : HdBufferSource()
    , _name(name)
    , _value(value)
    {
    }

    ~LoFiBindlessSamplerBufferSource() override = default;

    TfToken const &GetName() const override {
        return _name;
    }
    void const* GetData() const override {
        return &_value;
    }
    HdTupleType GetTupleType() const override {
        return _bindlessHandleTupleType;
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
    const GLuint64EXT _value;
};

class _ComputeBufferSourcesFunctor {
public:
    static void Compute(
        TfToken const &name,
        LoFiUvTextureObject const &texture,
        LoFiUvSamplerObject const &sampler,
        const bool useBindlessHandles,
        HdBufferSourceSharedPtrVector * const sources)
    {
        if (useBindlessHandles) {
            sources->push_back(
                std::make_shared<LoFiBindlessSamplerBufferSource>(
                    name,
                    sampler.GetGLTextureSamplerHandle()));
        } else {
            sources->push_back(
                std::make_shared<HdVtBufferSource>(
                    _Concat(
                        name,
                        LoFiBindingSuffixTokens->valid),
                    VtValue(texture.IsValid())));
        }
    }

    static void Compute(
        TfToken const &name,
        LoFiFieldTextureObject const &texture,
        LoFiFieldSamplerObject const &sampler,
        const bool useBindlessHandles,
        HdBufferSourceSharedPtrVector * const sources)
    {
        sources->push_back(
            std::make_shared<HdVtBufferSource>(
                _Concat(
                    name,
                    LoFiBindingSuffixTokens->samplingTransform),
                VtValue(texture.GetSamplingTransform())));

        if (useBindlessHandles) {
            sources->push_back(
                std::make_shared<LoFiBindlessSamplerBufferSource>(
                    name,
                    sampler.GetGLTextureSamplerHandle()));
        } else {
            sources->push_back(
                std::make_shared<HdVtBufferSource>(
                    _Concat(
                        name,
                        LoFiBindingSuffixTokens->valid),
                    VtValue(texture.IsValid())));
        }
    }

    static void Compute(
        TfToken const &name,
        LoFiPtexTextureObject const &texture,
        LoFiPtexSamplerObject const &sampler,
        const bool useBindlessHandles,
        HdBufferSourceSharedPtrVector * const sources)
    {
        if (!useBindlessHandles) {
            return;
        }

        sources->push_back(
            std::make_shared<LoFiBindlessSamplerBufferSource>(
                name,
                sampler.GetTexelsGLTextureHandle()));

        sources->push_back(
            std::make_shared<LoFiBindlessSamplerBufferSource>(
                _Concat(
                    name,
                    LoFiBindingSuffixTokens->layout),
                sampler.GetLayoutGLTextureHandle()));
    }

    static void Compute(
        TfToken const &name,
        LoFiUdimTextureObject const &texture,
        LoFiUdimSamplerObject const &sampler,
        const bool useBindlessHandles,
        HdBufferSourceSharedPtrVector * const sources)
    {
        if (!useBindlessHandles) {
            return;
        }

        sources->push_back(
            std::make_shared<LoFiBindlessSamplerBufferSource>(
                name,
                sampler.GetTexelsGLTextureHandle()));

        sources->push_back(
            std::make_shared<LoFiBindlessSamplerBufferSource>(
                _Concat(
                    name,
                    LoFiBindingSuffixTokens->layout),
                sampler.GetLayoutGLTextureHandle()));
    }
};

void
_BindTexture(const GLenum target,
             HgiTextureHandle const &textureHandle,
             HgiSamplerHandle const &samplerHandle,
             const TfToken &name,
             LoFiBinder const &binder,
             const bool bind)
{
    const LoFiBinding& binding = binder.GetTextureBinding(name);
    const int samplerUnit = binding.location;

    glActiveTexture(GL_TEXTURE0 + samplerUnit);

    const HgiTexture * const tex = textureHandle.Get();
    const HgiGLTexture * const glTex =
        dynamic_cast<const HgiGLTexture*>(tex);

    if (tex && !glTex) {
        TF_CODING_ERROR("LoFi texture binder only supports OpenGL");
    }

    const GLuint texName =
        (bind && glTex) ? glTex->GetTextureId() : 0;
    glBindTexture(target, texName);

    const HgiSampler * const sampler = samplerHandle.Get();
    const HgiGLSampler * const glSampler =
        dynamic_cast<const HgiGLSampler*>(sampler);

    if (sampler && !glSampler) {
        TF_CODING_ERROR("LoFi texture binder only supports OpenGL");
    }

    const GLuint samplerName =
        (bind && glSampler) ? glSampler->GetSamplerId() : 0;
    glBindSampler(samplerUnit, samplerName);
}

class _BindFunctor {
public:
    static void Compute(
        TfToken const &name,
        LoFiUvTextureObject const &texture,
        LoFiUvSamplerObject const &sampler,
        LoFiBinder const &binder,
        const bool bind)
    {
        _BindTexture(
            GL_TEXTURE_2D,
            texture.GetTexture(),
            sampler.GetSampler(),
            name,
            binder,
            bind);
    }

    static void Compute(
        TfToken const &name,
        LoFiFieldTextureObject const &texture,
        LoFiFieldSamplerObject const &sampler,
        LoFiBinder const &binder,
        const bool bind)
    {
        _BindTexture(
            GL_TEXTURE_3D,
            texture.GetTexture(),
            sampler.GetSampler(),
            name,
            binder,
            bind);
    }

    static void Compute(
        TfToken const &name,
        LoFiPtexTextureObject const &texture,
        LoFiPtexSamplerObject const &sampler,
        LoFiBinder const &binder,
        const bool bind)
    {
        const LoFiBinding& texelBinding = binder.GetTextureBinding(name);
        const int texelSamplerUnit = texelBinding.location;

        glActiveTexture(GL_TEXTURE0 + texelSamplerUnit);
        glBindTexture(GL_TEXTURE_2D_ARRAY,
                      bind ? texture.GetTexelTexture()->GetRawResource() : 0);

        HgiSampler * const texelSampler = sampler.GetTexelsSampler().Get();

        const HgiGLSampler * const glSampler =
            bind ? dynamic_cast<HgiGLSampler*>(texelSampler) : nullptr;

        if (glSampler) {
            glBindSampler(texelSamplerUnit, (GLuint)glSampler->GetSamplerId());
        } else {
            glBindSampler(texelSamplerUnit, 0);
        }

        const LoFiBinding& layoutBinding = binder.GetTextureBinding(
            _Concat(name, LoFiBindingSuffixTokens->layout));
        const int layoutSamplerUnit = layoutBinding.location;

        glActiveTexture(GL_TEXTURE0 + layoutSamplerUnit);
        glBindTexture(GL_TEXTURE_1D_ARRAY,
                      bind ? texture.GetLayoutTexture()->GetRawResource() : 0);
    }

    static void Compute(
        TfToken const &name,
        LoFiUdimTextureObject const &texture,
        LoFiUdimSamplerObject const &sampler,
        LoFiBinder const &binder,
        const bool bind)
    {
        const LoFiBinding& texelBinding = binder.GetTextureBinding(name);
        const int texelSamplerUnit = texelBinding.location;

        glActiveTexture(GL_TEXTURE0 + texelSamplerUnit);
        glBindTexture(GL_TEXTURE_2D_ARRAY,
                      bind ? texture.GetTexelTexture()->GetRawResource() : 0);

        HgiSampler * const texelSampler = sampler.GetTexelsSampler().Get();

        const HgiGLSampler * const glSampler =
            bind ? dynamic_cast<HgiGLSampler*>(texelSampler) : nullptr;

        if (glSampler) {
            glBindSampler(texelSamplerUnit, (GLuint)glSampler->GetSamplerId());
        } else {
            glBindSampler(texelSamplerUnit, 0);
        }

        const LoFiBinding& layoutBinding = binder.GetTextureBinding(
            _Concat(name, LoFiBindingSuffixTokens->layout));
        const int layoutSamplerUnit = layoutBinding.location;

        glActiveTexture(GL_TEXTURE0 + layoutSamplerUnit);
        glBindTexture(GL_TEXTURE_1D,
                      bind ? texture.GetLayoutTexture()->GetRawResource() : 0);
    }
};

template<HdTextureType textureType, class Functor, typename ...Args>
void _CastAndCompute(
    LoFiShaderCode::NamedTextureHandle const &namedTextureHandle,
    Args&& ...args)
{
    // e.g. HdStUvTextureObject
    using TextureObject = LoFiTypedTextureObject<textureType>;
    // e.g. HdStUvSamplerObject
    using SamplerObject = LoFiTypedSamplerObject<textureType>;

    if (!namedTextureHandle.handle) {
        TF_CODING_ERROR("Invalid texture handle in texture binder.");
        return;
    }

    const TextureObject * const typedTexture =
        dynamic_cast<TextureObject *>(
            namedTextureHandle.handle->GetTextureObject().get());
    if (!typedTexture) {
        TF_CODING_ERROR("Bad texture object");
        return;
    }

    const SamplerObject * const typedSampler =
        dynamic_cast<SamplerObject *>(
            namedTextureHandle.handle->GetSamplerObject().get());
    if (!typedSampler) {
        TF_CODING_ERROR("Bad sampler object");
        return;
    }

    Functor::Compute(namedTextureHandle.name, *typedTexture, *typedSampler,
                     std::forward<Args>(args)...);
}

template<class Functor, typename ...Args>
void _Dispatch(
    LoFiShaderCode::NamedTextureHandle const &namedTextureHandle,
    Args&& ...args)
{
    switch (namedTextureHandle.type) {
    case HdTextureType::Uv:
        _CastAndCompute<HdTextureType::Uv, Functor>(
            namedTextureHandle, std::forward<Args>(args)...);
        break;
    case HdTextureType::Field:
        _CastAndCompute<HdTextureType::Field, Functor>(
            namedTextureHandle, std::forward<Args>(args)...);
        break;
    case HdTextureType::Ptex:
        _CastAndCompute<HdTextureType::Ptex, Functor>(
            namedTextureHandle, std::forward<Args>(args)...);
        break;
    case HdTextureType::Udim:
        _CastAndCompute<HdTextureType::Udim, Functor>(
            namedTextureHandle, std::forward<Args>(args)...);
        break;
    }
}

template<class Functor, typename ...Args>
void _Dispatch(
    LoFiShaderCode::NamedTextureHandleVector const &textures,
    Args &&... args)
{
    for (const LoFiShaderCode::NamedTextureHandle & texture : textures) {
        _Dispatch<Functor>(texture, std::forward<Args>(args)...);
    }
}

} // end anonymous namespace

void
LoFiTextureBinder::ComputeBufferSources(
    const NamedTextureHandleVector &textures,
    bool useBindlessHandles,
    HdBufferSourceSharedPtrVector * const sources)
{
    _Dispatch<_ComputeBufferSourcesFunctor>(
        textures, useBindlessHandles, sources);
}

void
LoFiTextureBinder::BindResources(
    LoFiBinder const &binder,
    const bool useBindlessHandles,
    const NamedTextureHandleVector &textures)
{
    if (useBindlessHandles) {
        return;
    }

    _Dispatch<_BindFunctor>(textures, binder, /* bind = */ true);
}

void
LoFiTextureBinder::UnbindResources(
    LoFiBinder const &binder,
    const bool useBindlessHandles,
    const NamedTextureHandleVector &textures)
{
    if (useBindlessHandles) {
        return;
    }

    _Dispatch<_BindFunctor>(textures, binder, /* bind = */ false);
}

PXR_NAMESPACE_CLOSE_SCOPE
