//
// Copyright 2017 Pixar
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
#ifndef HDST_DRAW_TARGET_TEXTURE_RESOURCE_H
#define HDST_DRAW_TARGET_TEXTURE_RESOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/textureResource.h"
#include "pxr/imaging/glf/drawTarget.h"

PXR_NAMESPACE_OPEN_SCOPE


class HdSt_DrawTargetTextureResource final : public HdStTextureResource {
public:
    HdSt_DrawTargetTextureResource();
    virtual ~HdSt_DrawTargetTextureResource();

    void SetAttachment(const GlfDrawTarget::AttachmentRefPtr &attachment);
    void SetSampler(HdWrap wrapS, HdWrap wrapT,
                    HdMinFilter minFilter, HdMagFilter magFilter);

    //
    // HdTextureResource API
    //
    virtual HdTextureType GetTextureType() const override;
    virtual size_t GetMemoryUsed() override;

    //
    // HdStTextureResource API
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
    HdSt_DrawTargetTextureResource(
        const HdSt_DrawTargetTextureResource &) = delete;
    HdSt_DrawTargetTextureResource &operator =(
        const HdSt_DrawTargetTextureResource &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDST_DRAW_TARGET_TEXTURE_RESOURCE_H
