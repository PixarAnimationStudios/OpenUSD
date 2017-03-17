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
#ifndef HDX_DRAW_TARGET_ATTACHMENT_DESC_H
#define HDX_DRAW_TARGET_ATTACHMENT_DESC_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/enums.h"

#include "pxr/base/vt/value.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE


/// \class HdxDrawTargetAttachmentDesc
///
/// Represents an render to texture render pass.
///
/// \note This is a temporary API to aid transition to hydra, and is subject
/// to major changes.
///
class HdxDrawTargetAttachmentDesc final
{
public:
    /// default constructor - only for use by containers.
    HDX_API
    HdxDrawTargetAttachmentDesc();

    /// Construct a draw target attachment description
    HDX_API
    HdxDrawTargetAttachmentDesc(const std::string &name,
                                HdFormat           format,
                                const VtValue     &clearColor,
                                HdWrap             wrapS,
                                HdWrap             wrapT,
                                HdMinFilter        minFilter,
                                HdMagFilter        magFilter);
    ~HdxDrawTargetAttachmentDesc() = default;

    // Copy for container support.
    HDX_API
    HdxDrawTargetAttachmentDesc(const HdxDrawTargetAttachmentDesc &copy);
    HDX_API
    HdxDrawTargetAttachmentDesc &operator =(const HdxDrawTargetAttachmentDesc &copy);

    const std::string &GetName()       const { return _name; }
    HdFormat           GetFormat()     const { return _format; }
    const VtValue     &GetClearColor() const { return _clearColor; }
    HdWrap             GetWrapS()      const { return _wrapS; }
    HdWrap             GetWrapT()      const { return _wrapS; }
    HdMinFilter        GetMinFilter()  const { return _minFilter; }
    HdMagFilter        GetMagFilter()  const { return _magFilter; }

    // VtValue requirements
    HDX_API
    size_t GetHash() const;
    HDX_API
    void   Dump(std::ostream &out) const;
    HDX_API
    bool operator==(const HdxDrawTargetAttachmentDesc &other) const;
    HDX_API
    bool operator!=(const HdxDrawTargetAttachmentDesc &other) const;

private:
    std::string _name;
    HdFormat    _format;
    VtValue     _clearColor;
    HdWrap      _wrapS;
    HdWrap      _wrapT;
    HdMinFilter _minFilter;
    HdMagFilter _magFilter;
};

HDX_API
size_t hash_value(HdxDrawTargetAttachmentDesc const &attachment);
HDX_API
std::ostream &operator <<(std::ostream &out, const HdxDrawTargetAttachmentDesc &pv);


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDX_DRAW_TARGET_ATTACHMENT_DESC_H
