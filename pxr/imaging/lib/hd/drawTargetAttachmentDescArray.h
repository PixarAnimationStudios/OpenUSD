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
#ifndef HD_DRAW_TARGET_ATTACHMENT_DESC_ARRAY_H
#define HD_DRAW_TARGET_ATTACHMENT_DESC_ARRAY_H

#include "pxr/imaging/hd/drawTargetAttachmentDesc.h"

#include <vector>

/// \class HdDrawTargetAttachmentDescArray
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
class HdDrawTargetAttachmentDescArray
{
public:
    /// Default constructor for container purposes.
    HdDrawTargetAttachmentDescArray();

    HdDrawTargetAttachmentDescArray(size_t attachmentCount);
    virtual ~HdDrawTargetAttachmentDescArray() = default;

    HdDrawTargetAttachmentDescArray(const HdDrawTargetAttachmentDescArray &copy);
    HdDrawTargetAttachmentDescArray &operator =(const HdDrawTargetAttachmentDescArray &copy);

    /// Pushes a new attachment onto the end of the list of attachments.
    void AddAttachment(const std::string &name,
                       HdFormat           format,
                       const VtValue      &clearColor);

    size_t GetNumAttachments() const;
    const HdDrawTargetAttachmentDesc &GetAttachment(size_t idx) const;


    // VtValue requirements
    size_t GetHash() const;
    void   Dump(std::ostream &out) const;
    bool operator==(const HdDrawTargetAttachmentDescArray &other) const;
    bool operator!=(const HdDrawTargetAttachmentDescArray &other) const;

private:
    typedef std::vector<HdDrawTargetAttachmentDesc> _AttachmentDescArray;

    _AttachmentDescArray _attachments;
};

size_t hash_value(const HdDrawTargetAttachmentDescArray &attachments);
std::ostream &operator <<(std::ostream &out, const HdDrawTargetAttachmentDescArray &pv);

#endif  // HD_DRAW_TARGET_ATTACHMENT_DESC_ARRAY_H
