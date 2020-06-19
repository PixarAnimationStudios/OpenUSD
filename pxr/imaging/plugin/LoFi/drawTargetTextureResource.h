//
// Copyright 2020 benmalartre
//
// Unlicensed
// 
#ifndef PXR_IMAGING_PLUGIN_LOFI_DRAW_TARGET_TEXTURE_RESOURCE_H
#define PXR_IMAGING_PLUGIN_LOFI_DRAW_TARGET_TEXTURE_RESOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/textureResource.h"
#include "pxr/imaging/glf/drawTarget.h"

PXR_NAMESPACE_OPEN_SCOPE


class LoFiDrawTargetTextureResource final : public LoFiTextureResource {
public:
    LoFiDrawTargetTextureResource();
    virtual ~LoFiDrawTargetTextureResource();

    void SetAttachment(const GlfDrawTarget::AttachmentRefPtr &attachment);
    void SetSampler(HdWrap wrapS, HdWrap wrapT,
                    HdMinFilter minFilter, HdMagFilter magFilter);

    //
    // HdTextureResource API
    //
    virtual HdTextureType GetTextureType() const override;
    virtual size_t GetMemoryUsed() override;

    //
    // LoFiTextureResource API
    //
    virtual GLuint GetTexelsTextureId() override;
    virtual GLuint GetTexelsSamplerId() override;
    virtual uint64_t GetTexelsTextureHandle() override;
    virtual GLuint GetLayoutTextureId() override;
    virtual uint64_t GetLayoutTextureHandle() override;

private:
    GlfDrawTarget::AttachmentRefPtr  _attachment;
    GLuint                           _sampler;
    GfVec4f                          _borderColor;
    float                            _maxAnisotropy;

    // No copying
    LoFiDrawTargetTextureResource(
        const LoFiDrawTargetTextureResource &) = delete;
    LoFiDrawTargetTextureResource &operator =(
        const LoFiDrawTargetTextureResource &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_DRAW_TARGET_TEXTURE_RESOURCE_H
