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
#ifndef HD_DRAW_TARGET_ATTACHMENT_DESC_H
#define HD_DRAW_TARGET_ATTACHMENT_DESC_H

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/enums.h"

#include "pxr/base/vt/value.h"

#include <string>

/// Represents an render to texture render pass.
/// Note:  This is a temporary api to aid transition to hydra.
/// and subject to major changes.
class HdDrawTargetAttachmentDesc final
{
public:
    /// default constructor - only for use by containers.
    HdDrawTargetAttachmentDesc();

    /// Construct a draw target attachment description
    HdDrawTargetAttachmentDesc(const std::string &name,
                               HdFormat           format,
                               const VtValue      &clearColor);
    ~HdDrawTargetAttachmentDesc() = default;

    // Copy for container support.
    HdDrawTargetAttachmentDesc(const HdDrawTargetAttachmentDesc &copy);
    HdDrawTargetAttachmentDesc &operator =(const HdDrawTargetAttachmentDesc &copy);

    const std::string &GetName()       const { return _name; }
    HdFormat           GetFormat()     const { return _format; }
    const VtValue     &GetClearColor() const { return _clearColor; }

    // VtValue requirements
    size_t GetHash() const;
    void   Dump(std::ostream &out) const;
    bool operator==(const HdDrawTargetAttachmentDesc &other) const;
    bool operator!=(const HdDrawTargetAttachmentDesc &other) const;

private:
    std::string _name;
    HdFormat    _format;
    VtValue     _clearColor;

};

size_t hash_value(HdDrawTargetAttachmentDesc const &attachment);
std::ostream &operator <<(std::ostream &out, const HdDrawTargetAttachmentDesc &pv);

#endif  // HD_DRAW_TARGET_ATTACHMENT_DESC_H
