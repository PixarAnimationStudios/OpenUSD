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
#ifndef HDX_DRAW_TARGET_ATTACHMENT_DESC_ARRAY_H
#define HDX_DRAW_TARGET_ATTACHMENT_DESC_ARRAY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/drawTargetAttachmentDesc.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class HdxDrawTargetAttachmentDescArray
///
/// Describes all the color buffer attachments for a draw target.
/// The array should not contain a depth buffer - that is managed
/// separately.
///
/// \note This is a temporary API to aid transition to hydra, and is subject
/// to major changes.
///
/// class is derivable for TfAny support.
///
class HdxDrawTargetAttachmentDescArray
{
public:
    /// Default constructor for container purposes.
    HDX_API
    HdxDrawTargetAttachmentDescArray();

    HDX_API
    HdxDrawTargetAttachmentDescArray(size_t attachmentCount);
    virtual ~HdxDrawTargetAttachmentDescArray() = default;

    HDX_API
    HdxDrawTargetAttachmentDescArray(const HdxDrawTargetAttachmentDescArray &copy);
    HDX_API
    HdxDrawTargetAttachmentDescArray &operator =(const HdxDrawTargetAttachmentDescArray &copy);

    /// Pushes a new attachment onto the end of the list of attachments.
    HDX_API
    void AddAttachment(const std::string &name,
                       HdFormat           format,
                       const VtValue      &clearColor,
                       HdWrap             wrapS,
                       HdWrap             wrapT,
                       HdMinFilter        minFilter,
                       HdMagFilter        magFilter);


    HDX_API
    size_t GetNumAttachments() const;
    HDX_API
    const HdxDrawTargetAttachmentDesc &GetAttachment(size_t idx) const;

    /// Sampler State for Depth attachment
    HDX_API
    void SetDepthSampler(HdWrap      depthWrapS,
                         HdWrap      depthWrapT,
                         HdMinFilter depthMinFilter,
                         HdMagFilter depthMagFilter);


    HdWrap      GetDepthWrapS()     const { return _depthWrapS;     }
    HdWrap      GetDepthWrapT()     const { return _depthWrapT;     }
    HdMinFilter GetDepthMinFilter() const { return _depthMinFilter; }
    HdMagFilter GetDepthMagFilter() const { return _depthMagFilter; }


    // VtValue requirements
    HDX_API
    size_t GetHash() const;
    HDX_API
    void   Dump(std::ostream &out) const;
    HDX_API
    bool operator==(const HdxDrawTargetAttachmentDescArray &other) const;
    HDX_API
    bool operator!=(const HdxDrawTargetAttachmentDescArray &other) const;

private:
    typedef std::vector<HdxDrawTargetAttachmentDesc> _AttachmentDescArray;

    _AttachmentDescArray _attachments;

    // Sampler State for Depth attachment
    HdWrap      _depthWrapS;
    HdWrap      _depthWrapT;
    HdMinFilter _depthMinFilter;
    HdMagFilter _depthMagFilter;
};

HDX_API
size_t hash_value(const HdxDrawTargetAttachmentDescArray &attachments);
HDX_API
std::ostream &operator <<(std::ostream &out, const HdxDrawTargetAttachmentDescArray &pv);


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDX_DRAW_TARGET_ATTACHMENT_DESC_ARRAY_H
