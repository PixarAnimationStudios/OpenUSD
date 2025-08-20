//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/textureBinder.h"
#include "pxr/imaging/hdSt/ptexTextureObject.h"
#include "pxr/imaging/hdSt/resourceBinder.h"
#include "pxr/imaging/hdSt/samplerObject.h"
#include "pxr/imaging/hdSt/textureHandle.h"
#include "pxr/imaging/hdSt/textureObject.h"
#include "pxr/imaging/hdSt/udimTextureObject.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/vt/array.h"

PXR_NAMESPACE_OPEN_SCOPE

static
TfToken
_Concat(const TfToken &a, const TfToken &b)
{
    return TfToken(a.GetString() + b.GetString());
}

void
HdSt_TextureBinder::GetBufferSpecs(
    const NamedTextureHandleVector &textures,
    HdBufferSpecVector * const specs,
    bool doublesSupported)
{
    const bool useBindlessHandles = textures.empty() ? false :
        textures[0].handles[0]->UseBindlessHandles();

    for (const NamedTextureHandle & texture : textures) {
        switch (texture.type) {
        case HdStTextureType::Uv:
            if (useBindlessHandles) {
                specs->emplace_back(
                    texture.name,
                    HdTupleType{ HdTypeUInt32Vec2, texture.handles.size() });
            }
            specs->emplace_back(
                _Concat(
                    texture.name,
                    HdSt_ResourceBindingSuffixTokens->valid),
                HdTupleType{HdTypeBool, 1});
            break;
        case HdStTextureType::Field:
            if (useBindlessHandles) {
                specs->emplace_back(
                    texture.name,
                    HdTupleType{ HdTypeUInt32Vec2, texture.handles.size() });
            }
            specs->emplace_back(
                _Concat(
                    texture.name,
                    HdSt_ResourceBindingSuffixTokens->valid),
                HdTupleType{HdTypeBool, 1});
            specs->emplace_back(
                _Concat(
                    texture.name,
                    HdSt_ResourceBindingSuffixTokens->samplingTransform),
                HdTupleType{ (doublesSupported ?
                    HdTypeDoubleMat4 : HdTypeFloatMat4), 1});
            break;
        case HdStTextureType::Ptex:
            if (useBindlessHandles) {
                specs->emplace_back(
                    texture.name,
                    HdTupleType{ HdTypeUInt32Vec2, texture.handles.size() });
                specs->emplace_back(
                    _Concat(
                        texture.name,
                        HdSt_ResourceBindingSuffixTokens->layout),
                        HdTupleType{ HdTypeUInt32Vec2, texture.handles.size() });
            }
            specs->emplace_back(
                _Concat(
                    texture.name,
                    HdSt_ResourceBindingSuffixTokens->valid),
                HdTupleType{HdTypeBool, 1});
            break;
        case HdStTextureType::Udim:
            if (useBindlessHandles) {
                specs->emplace_back(
                    texture.name,
                    HdTupleType{ HdTypeUInt32Vec2, texture.handles.size() });
                specs->emplace_back(
                    _Concat(
                        texture.name,
                        HdSt_ResourceBindingSuffixTokens->layout),
                        HdTupleType{ HdTypeUInt32Vec2, texture.handles.size() });
            }
            specs->emplace_back(
                _Concat(
                    texture.name,
                    HdSt_ResourceBindingSuffixTokens->valid),
                HdTupleType{HdTypeBool, 1});
            break;
        }
    }
}

namespace {

// A bindless GL sampler buffer.
// This identifies a texture as a 64-bit handle, passed to GLSL as "uvec2".
// See https://www.khronos.org/opengl/wiki/Bindless_Texture
class HdSt_BindlessSamplerBufferSource : public HdBufferSource {
public:
    HdSt_BindlessSamplerBufferSource(TfToken const &name,
                                     const VtArray<uint64_t>& value)
    : HdBufferSource()
    , _name(name)
    , _value(value)
    {
    }

    ~HdSt_BindlessSamplerBufferSource() override = default;

    TfToken const &GetName() const override {
        return _name;
    }
    void const* GetData() const override {
        return _value.data();
    }
    HdTupleType GetTupleType() const override {
        return { HdTypeUInt32Vec2, _value.size() };
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
    const VtArray<uint64_t> _value;
};

class _ComputeBufferSourcesFunctor {
public:
    static void Compute(
        TfToken const &name,
        std::vector<const HdStUvTextureObject*> const &textures,
        std::vector<const HdStUvSamplerObject*> const &samplers,
        HdBufferSourceSharedPtrVector * const sources,
        bool useBindlessHandles,
        bool doublesSupported)
    {
        if (useBindlessHandles) {
            VtArray<uint64_t> bindlessHandles;
            for (size_t i = 0; i < textures.size(); i++) {
                bindlessHandles.push_back(
                    HdSt_ResourceBinder::GetSamplerBindlessHandle(
                        samplers[i]->GetSampler(), textures[i]->GetTexture()));
            }
            sources->push_back(
                std::make_shared<HdSt_BindlessSamplerBufferSource>(
                    name,
                    bindlessHandles));
        }
       
        sources->push_back(
            std::make_shared<HdVtBufferSource>(
                _Concat(
                    name,
                    HdSt_ResourceBindingSuffixTokens->valid),
                VtValue(textures[0]->IsValid())));
    }

    static void Compute(
        TfToken const &name,
        std::vector<const HdStFieldTextureObject*> const &textures,
        std::vector<const HdStFieldSamplerObject*> const &samplers,
        HdBufferSourceSharedPtrVector * const sources,
        bool useBindlessHandles,
        bool doublesSupported)
    {
        if (useBindlessHandles) {
            VtArray<uint64_t> bindlessHandles;
            for (size_t i = 0; i < textures.size(); i++) {
                bindlessHandles.push_back(
                    HdSt_ResourceBinder::GetSamplerBindlessHandle(
                        samplers[i]->GetSampler(), textures[i]->GetTexture()));
            }
            sources->push_back(
                std::make_shared<HdSt_BindlessSamplerBufferSource>(
                    name,
                    bindlessHandles));
        }
        sources->push_back(
            std::make_shared<HdVtBufferSource>(
                _Concat(
                    name,
                    HdSt_ResourceBindingSuffixTokens->valid),
                VtValue(textures[0]->IsValid())));
        sources->push_back(
            std::make_shared<HdVtBufferSource>(
                _Concat(
                    name,
                    HdSt_ResourceBindingSuffixTokens->samplingTransform),
                VtValue(textures[0]->GetSamplingTransform()),
                1,
                doublesSupported));
    }

    static void Compute(
        TfToken const &name,
        std::vector<const HdStPtexTextureObject*> const &textures,
        std::vector<const HdStPtexSamplerObject*> const &samplers,
        HdBufferSourceSharedPtrVector * const sources,
        bool useBindlessHandles,
        bool doublesSupported)
    {
        if (useBindlessHandles) {
            VtArray<uint64_t> bindlessHandles;
            VtArray<uint64_t> bindlessLayoutHandles;
            for (size_t i = 0; i < textures.size(); i++) {
                bindlessHandles.push_back(
                    HdSt_ResourceBinder::GetSamplerBindlessHandle(
                        samplers[i]->GetTexelsSampler(),
                        textures[i]->GetTexelTexture()));
                bindlessLayoutHandles.push_back(
                    HdSt_ResourceBinder::GetTextureBindlessHandle(
                        textures[i]->GetLayoutTexture()));
            }
            sources->push_back(
                std::make_shared<HdSt_BindlessSamplerBufferSource>(
                    name,
                    bindlessHandles));
            sources->push_back(
                std::make_shared<HdSt_BindlessSamplerBufferSource>(
                    _Concat(
                        name,
                        HdSt_ResourceBindingSuffixTokens->layout),
                    bindlessLayoutHandles));
        }
        sources->push_back(
            std::make_shared<HdVtBufferSource>(
                _Concat(
                    name,
                    HdSt_ResourceBindingSuffixTokens->valid),
                VtValue(textures[0]->IsValid())));
    }

    static void Compute(
        TfToken const &name,
        std::vector<const HdStUdimTextureObject*> const &textures,
        std::vector<const HdStUdimSamplerObject*> const &samplers,
        HdBufferSourceSharedPtrVector * const sources,
        bool useBindlessHandles,
        bool doublesSupported)
    {
        if (useBindlessHandles) {
            VtArray<uint64_t> bindlessHandles;
            VtArray<uint64_t> bindlessLayoutHandles;
            for (size_t i = 0; i < textures.size(); i++) {
                bindlessHandles.push_back(
                    HdSt_ResourceBinder::GetSamplerBindlessHandle(
                        samplers[i]->GetTexelsSampler(),
                        textures[i]->GetTexelTexture()));
                bindlessLayoutHandles.push_back(
                    HdSt_ResourceBinder::GetTextureBindlessHandle(
                        textures[i]->GetLayoutTexture()));
            }
            sources->push_back(
                std::make_shared<HdSt_BindlessSamplerBufferSource>(
                    name,
                    bindlessHandles));
            sources->push_back(
                std::make_shared<HdSt_BindlessSamplerBufferSource>(
                    _Concat(
                        name,
                        HdSt_ResourceBindingSuffixTokens->layout),
                    bindlessLayoutHandles));
        }
        sources->push_back(
            std::make_shared<HdVtBufferSource>(
                _Concat(
                    name,
                    HdSt_ResourceBindingSuffixTokens->valid),
                VtValue(textures[0]->IsValid())));
    }
};

class _BindFunctor {
public:
    static void Compute(
        TfToken const &name,
        std::vector<const HdStUvTextureObject*> const &textures,
        std::vector<const HdStUvSamplerObject*> const &samplers,
        HdSt_ResourceBinder const &binder,
        const bool bind)
    {
        std::vector<HgiTextureHandle> textureHandles;
        std::vector<HgiSamplerHandle> samplerHandles;
        for (size_t i = 0; i < textures.size(); i++) {
            textureHandles.push_back(textures[i]->GetTexture());
            samplerHandles.push_back(samplers[i]->GetSampler());
        }
        binder.BindTextures(
                name,
                samplerHandles,
                textureHandles,
                bind);
    }

    static void Compute(
        TfToken const &name,
        std::vector<const HdStFieldTextureObject*> const &textures,
        std::vector<const HdStFieldSamplerObject*> const &samplers,
        HdSt_ResourceBinder const &binder,
        const bool bind)
    {
        std::vector<HgiTextureHandle> textureHandles;
        std::vector<HgiSamplerHandle> samplerHandles;
        for (size_t i = 0; i < textures.size(); i++) {
            textureHandles.push_back(textures[i]->GetTexture());
            samplerHandles.push_back(samplers[i]->GetSampler());
        }
        binder.BindTextures(
                name,
                samplerHandles,
                textureHandles,
                bind);
    }

    static void Compute(
        TfToken const &name,
        std::vector<const HdStPtexTextureObject*> const &textures,
        std::vector<const HdStPtexSamplerObject*> const &samplers,
        HdSt_ResourceBinder const &binder,
        const bool bind)
    {
        std::vector<HgiTextureHandle> textureHandles;
        std::vector<HgiTextureHandle> layoutTextureHandles;
        std::vector<HgiSamplerHandle> samplerHandles;
        std::vector<HgiSamplerHandle> layoutSamplerHandles;
        for (size_t i = 0; i < textures.size(); i++) {
            textureHandles.push_back(textures[i]->GetTexelTexture());
            layoutTextureHandles.push_back(textures[i]->GetLayoutTexture());
            samplerHandles.push_back(samplers[i]->GetTexelsSampler());
            layoutSamplerHandles.push_back(samplers[i]->GetLayoutSampler());
        }
        binder.BindTexturesWithLayout(
            name,
            samplerHandles,
            textureHandles,
            layoutSamplerHandles,
            layoutTextureHandles,
            bind);
    }

    static void Compute(
        TfToken const &name,
        std::vector<const HdStUdimTextureObject*> const &textures,
        std::vector<const HdStUdimSamplerObject*> const &samplers,
        HdSt_ResourceBinder const &binder,
        const bool bind)
    {
        std::vector<HgiTextureHandle> textureHandles;
        std::vector<HgiTextureHandle> layoutTextureHandles;
        std::vector<HgiSamplerHandle> samplerHandles;
        std::vector<HgiSamplerHandle> layoutSamplerHandles;
        for (size_t i = 0; i < textures.size(); i++) {
            textureHandles.push_back(textures[i]->GetTexelTexture());
            layoutTextureHandles.push_back(textures[i]->GetLayoutTexture());
            samplerHandles.push_back(samplers[i]->GetTexelsSampler());
            layoutSamplerHandles.push_back(samplers[i]->GetLayoutSampler());
        }
        binder.BindTexturesWithLayout(
            name,
            samplerHandles,
            textureHandles,
            layoutSamplerHandles,
            layoutTextureHandles,
            bind);
    }
};

class _BindingDescsFunctor {
public:
    static void Compute(
        TfToken const &name,
        std::vector<const HdStUvTextureObject*> const &textures,
        std::vector<const HdStUvSamplerObject*> const &samplers,
        HdSt_ResourceBinder const &binder,
        HgiResourceBindingsDesc * bindingsDesc)
    {
        std::vector<HgiTextureHandle> textureHandles;
        std::vector<HgiSamplerHandle> samplerHandles;
        for (size_t i = 0; i < textures.size(); i++) {
            textureHandles.push_back(textures[i]->GetTexture());
            samplerHandles.push_back(samplers[i]->GetSampler());
        }
        binder.GetTextureBindingDescs(
                bindingsDesc,
                name,
                samplerHandles,
                textureHandles);
    }

    static void Compute(
        TfToken const &name,
        std::vector<const HdStFieldTextureObject*> const &textures,
        std::vector<const HdStFieldSamplerObject*> const &samplers,
        HdSt_ResourceBinder const &binder,
        HgiResourceBindingsDesc * bindingsDesc)
    {
        std::vector<HgiTextureHandle> textureHandles;
        std::vector<HgiSamplerHandle> samplerHandles;
        for (size_t i = 0; i < textures.size(); i++) {
            textureHandles.push_back(textures[i]->GetTexture());
            samplerHandles.push_back(samplers[i]->GetSampler());
        }
        binder.GetTextureBindingDescs(
                bindingsDesc,
                name,
                samplerHandles,
                textureHandles);
    }

    static void Compute(
        TfToken const &name,
        std::vector<const HdStPtexTextureObject*> const &textures,
        std::vector<const HdStPtexSamplerObject*> const &samplers,
        HdSt_ResourceBinder const &binder,
        HgiResourceBindingsDesc * bindingsDesc)
    {
        std::vector<HgiTextureHandle> textureHandles;
        std::vector<HgiTextureHandle> layoutTextureHandles;
        std::vector<HgiSamplerHandle> samplerHandles;
        std::vector<HgiSamplerHandle> layoutSamplerHandles;
        for (size_t i = 0; i < textures.size(); i++) {
            textureHandles.push_back(textures[i]->GetTexelTexture());
            layoutTextureHandles.push_back(textures[i]->GetLayoutTexture());
            samplerHandles.push_back(samplers[i]->GetTexelsSampler());
            layoutSamplerHandles.push_back(samplers[i]->GetLayoutSampler());
        }
        binder.GetTextureWithLayoutBindingDescs(
                bindingsDesc,
                name,
                samplerHandles,
                textureHandles,
                layoutSamplerHandles,
                layoutTextureHandles);
    }

    static void Compute(
        TfToken const &name,
        std::vector<const HdStUdimTextureObject*> const &textures,
        std::vector<const HdStUdimSamplerObject*> const &samplers,
        HdSt_ResourceBinder const &binder,
        HgiResourceBindingsDesc * bindingsDesc)
    {
        std::vector<HgiTextureHandle> textureHandles;
        std::vector<HgiTextureHandle> layoutTextureHandles;
        std::vector<HgiSamplerHandle> samplerHandles;
        std::vector<HgiSamplerHandle> layoutSamplerHandles;
        for (size_t i = 0; i < textures.size(); i++) {
            textureHandles.push_back(textures[i]->GetTexelTexture());
            layoutTextureHandles.push_back(textures[i]->GetLayoutTexture());
            samplerHandles.push_back(samplers[i]->GetTexelsSampler());
            layoutSamplerHandles.push_back(samplers[i]->GetLayoutSampler());
        }
        binder.GetTextureWithLayoutBindingDescs(
                bindingsDesc,
                name,
                samplerHandles,
                textureHandles,
                layoutSamplerHandles,
                layoutTextureHandles);
    }
};

template<HdStTextureType textureType, class Functor, typename ...Args>
void _CastAndCompute(
    HdStShaderCode::NamedTextureHandle const &namedTextureHandle,
    Args&& ...args)
{
    // e.g. HdStUvTextureObject
    using TextureObject = HdStTypedTextureObject<textureType>;
    // e.g. HdStUvSamplerObject
    using SamplerObject = HdStTypedSamplerObject<textureType>;

    std::vector<const TextureObject*> textureObjects;
    std::vector<const SamplerObject*> samplerObjects;
    for (const HdStTextureHandleSharedPtr& tex : namedTextureHandle.handles) {
        if (!tex) {
            TF_CODING_ERROR("Invalid texture handle in texture binder.");
            return;
        }
        const TextureObject * typedTexture =
            dynamic_cast<TextureObject *>(
            tex->GetTextureObject().get());
        if (!typedTexture) {
            TF_CODING_ERROR("Bad texture object");
            return;
        }
        textureObjects.push_back(typedTexture);

        const SamplerObject * typedSampler =
            dynamic_cast<SamplerObject *>(
                tex->GetSamplerObject().get());
        if (!typedSampler) {
            TF_CODING_ERROR("Bad sampler object");
            return;
        }
        samplerObjects.push_back(typedSampler);
    }


    Functor::Compute(namedTextureHandle.name, textureObjects, samplerObjects,
                     std::forward<Args>(args)...);
}

template<class Functor, typename ...Args>
void _Dispatch(
    HdStShaderCode::NamedTextureHandle const &namedTextureHandle,
    Args&& ...args)
{
    switch (namedTextureHandle.type) {
    case HdStTextureType::Uv:
        _CastAndCompute<HdStTextureType::Uv, Functor>(
            namedTextureHandle, std::forward<Args>(args)...);
        break;
    case HdStTextureType::Field:
        _CastAndCompute<HdStTextureType::Field, Functor>(
            namedTextureHandle, std::forward<Args>(args)...);
        break;
    case HdStTextureType::Ptex:
        _CastAndCompute<HdStTextureType::Ptex, Functor>(
            namedTextureHandle, std::forward<Args>(args)...);
        break;
    case HdStTextureType::Udim:
        _CastAndCompute<HdStTextureType::Udim, Functor>(
            namedTextureHandle, std::forward<Args>(args)...);
        break;
    }
}

template<class Functor, typename ...Args>
void _Dispatch(
    HdStShaderCode::NamedTextureHandleVector const &textures,
    Args &&... args)
{
    for (const HdStShaderCode::NamedTextureHandle & texture : textures) {
        _Dispatch<Functor>(texture, std::forward<Args>(args)...);
    }
}

} // end anonymous namespace

void
HdSt_TextureBinder::ComputeBufferSources(
    const NamedTextureHandleVector &textures,
    HdBufferSourceSharedPtrVector * const sources,
    bool doublesSupported)
{
    const bool useBindlessHandles = textures.empty() ? false :
        textures[0].handles[0]->UseBindlessHandles();

    _Dispatch<_ComputeBufferSourcesFunctor>(textures, sources, 
        useBindlessHandles, doublesSupported);
}

void
HdSt_TextureBinder::BindResources(
    HdSt_ResourceBinder const &binder,
    const NamedTextureHandleVector &textures)
{
    _Dispatch<_BindFunctor>(textures, binder, /* bind = */ true);
}

void
HdSt_TextureBinder::UnbindResources(
    HdSt_ResourceBinder const &binder,
    const NamedTextureHandleVector &textures)
{
    _Dispatch<_BindFunctor>(textures, binder, /* bind = */ false);
}

void
HdSt_TextureBinder::GetBindingDescs(
        HdSt_ResourceBinder const &binder,
        HgiResourceBindingsDesc * bindingsDesc,
        const NamedTextureHandleVector &textures)
{
    _Dispatch<_BindingDescsFunctor>(textures, binder, bindingsDesc);
}

PXR_NAMESPACE_CLOSE_SCOPE
