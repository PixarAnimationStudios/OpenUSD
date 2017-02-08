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
#ifndef HDX_DRAW_TARGET_TEXTURE_RESOURCE_H
#define HDX_DRAW_TARGET_TEXTURE_RESOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/textureResource.h"
#include "pxr/imaging/glf/drawTarget.h"

PXR_NAMESPACE_OPEN_SCOPE


class Hdx_DrawTargetTextureResource final : public HdTextureResource {
public:
    Hdx_DrawTargetTextureResource();
    virtual ~Hdx_DrawTargetTextureResource();

    void SetAttachment(const GlfDrawTarget::AttachmentRefPtr &attachment);
    void SetSampler(HdWrap wrapS, HdWrap wrapT,
                    HdMinFilter minFilter, HdMagFilter magFilter);

    //
    // HdTextureResource API
    //
    virtual bool IsPtex() const override;

    virtual GLuint GetTexelsTextureId() override;
    virtual GLuint GetTexelsSamplerId() override;
    virtual GLuint64EXT GetTexelsTextureHandle() override;

    virtual GLuint GetLayoutTextureId() override;
    virtual GLuint64EXT GetLayoutTextureHandle() override;

    virtual size_t GetMemoryUsed() override;

private:
    GlfDrawTarget::AttachmentRefPtr  _attachment;
    GLuint                           _sampler;


    // No copying
    Hdx_DrawTargetTextureResource(const Hdx_DrawTargetTextureResource &)             = delete;
    Hdx_DrawTargetTextureResource &operator =(const Hdx_DrawTargetTextureResource &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDX_DRAW_TARGET_TEXTURE_RESOURCE_H
