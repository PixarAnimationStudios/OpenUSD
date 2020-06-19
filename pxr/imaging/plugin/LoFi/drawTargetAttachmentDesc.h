//
// Copyright 2020 benmalartre
//
// Unlicensed
// 
#ifndef PXR_IMAGING_PLUGIN_LOFI_DRAW_TARGET_ATTACHMENT_DESC_H
#define PXR_IMAGING_PLUGIN_LOFI_DRAW_TARGET_ATTACHMENT_DESC_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/base/vt/value.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE


/// \class LoFiDrawTargetAttachmentDesc
///
/// Represents an render to texture render pass.
///
/// \note This is a temporary API to aid transition to Storm, and is subject
/// to major changes.
///
class LoFiDrawTargetAttachmentDesc final
{
public:
    /// default constructor - only for use by containers.
    LOFI_API
    LoFiDrawTargetAttachmentDesc();

    /// Construct a draw target attachment description
    LOFI_API
    LoFiDrawTargetAttachmentDesc(const std::string &name,
                                HdFormat           format,
                                const VtValue     &clearColor,
                                HdWrap             wrapS,
                                HdWrap             wrapT,
                                HdMinFilter        minFilter,
                                HdMagFilter        magFilter);
    ~LoFiDrawTargetAttachmentDesc() = default;

    // Copy for container support.
    LOFI_API
    LoFiDrawTargetAttachmentDesc(const LoFiDrawTargetAttachmentDesc &copy);
    LOFI_API
    LoFiDrawTargetAttachmentDesc &operator =(
                                      const LoFiDrawTargetAttachmentDesc &copy);

    const std::string &GetName()       const { return _name; }
    HdFormat           GetFormat()     const { return _format; }
    const VtValue     &GetClearColor() const { return _clearColor; }
    HdWrap             GetWrapS()      const { return _wrapS; }
    HdWrap             GetWrapT()      const { return _wrapS; }
    HdMinFilter        GetMinFilter()  const { return _minFilter; }
    HdMagFilter        GetMagFilter()  const { return _magFilter; }

    // VtValue requirements
    LOFI_API
    size_t GetHash() const;
    LOFI_API
    void   Dump(std::ostream &out) const;
    LOFI_API
    bool operator==(const LoFiDrawTargetAttachmentDesc &other) const;
    LOFI_API
    bool operator!=(const LoFiDrawTargetAttachmentDesc &other) const;

private:
    std::string _name;
    HdFormat    _format;
    VtValue     _clearColor;
    HdWrap      _wrapS;
    HdWrap      _wrapT;
    HdMinFilter _minFilter;
    HdMagFilter _magFilter;
};

LOFI_API
size_t hash_value(LoFiDrawTargetAttachmentDesc const &attachment);
LOFI_API
std::ostream &operator <<(std::ostream &out,
                          const LoFiDrawTargetAttachmentDesc &pv);


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_DRAW_TARGET_ATTACHMENT_DESC_H
