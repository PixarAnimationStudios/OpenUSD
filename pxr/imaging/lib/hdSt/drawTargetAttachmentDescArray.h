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
#ifndef HDST_DRAW_TARGET_ATTACHMENT_DESC_ARRAY_H
#define HDST_DRAW_TARGET_ATTACHMENT_DESC_ARRAY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/drawTargetAttachmentDesc.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class HdStDrawTargetAttachmentDescArray
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
class HdStDrawTargetAttachmentDescArray
{
public:
    /// Default constructor for container purposes.
    HDST_API
    HdStDrawTargetAttachmentDescArray();

    HDST_API
    HdStDrawTargetAttachmentDescArray(size_t attachmentCount);
    virtual ~HdStDrawTargetAttachmentDescArray() = default;

    HDST_API
    HdStDrawTargetAttachmentDescArray(const HdStDrawTargetAttachmentDescArray &copy);
    HDST_API
    HdStDrawTargetAttachmentDescArray &operator =(const HdStDrawTargetAttachmentDescArray &copy);

    /// Pushes a new attachment onto the end of the list of attachments.
    HDST_API
    void AddAttachment(const std::string &name,
                       HdFormat           format,
                       const VtValue      &clearColor,
                       HdWrap             wrapS,
                       HdWrap             wrapT,
                       HdMinFilter        minFilter,
                       HdMagFilter        magFilter);


    HDST_API
    size_t GetNumAttachments() const;
    HDST_API
    const HdStDrawTargetAttachmentDesc &GetAttachment(size_t idx) const;

    /// Sampler State for Depth attachment
    HDST_API
    void SetDepthSampler(HdWrap      depthWrapS,
                         HdWrap      depthWrapT,
                         HdMinFilter depthMinFilter,
                         HdMagFilter depthMagFilter);


    HdWrap      GetDepthWrapS()     const { return _depthWrapS;     }
    HdWrap      GetDepthWrapT()     const { return _depthWrapT;     }
    HdMinFilter GetDepthMinFilter() const { return _depthMinFilter; }
    HdMagFilter GetDepthMagFilter() const { return _depthMagFilter; }


    // VtValue requirements
    HDST_API
    size_t GetHash() const;
    HDST_API
    void   Dump(std::ostream &out) const;
    HDST_API
    bool operator==(const HdStDrawTargetAttachmentDescArray &other) const;
    HDST_API
    bool operator!=(const HdStDrawTargetAttachmentDescArray &other) const;

private:
    typedef std::vector<HdStDrawTargetAttachmentDesc> _AttachmentDescArray;

    _AttachmentDescArray _attachments;

    // Sampler State for Depth attachment
    HdWrap      _depthWrapS;
    HdWrap      _depthWrapT;
    HdMinFilter _depthMinFilter;
    HdMagFilter _depthMagFilter;
};

HDST_API
size_t hash_value(const HdStDrawTargetAttachmentDescArray &attachments);
HDST_API
std::ostream &operator <<(std::ostream &out,
                          const HdStDrawTargetAttachmentDescArray &pv);


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDST_DRAW_TARGET_ATTACHMENT_DESC_ARRAY_H
