//
// Copyright 2020 benmalartre
//
// Unlicensed
// 
#ifndef PXR_IMAGING_PLUGIN_LOFI_DRAW_TARGET_ATTACHMENT_DESC_ARRAY_H
#define PXR_IMAGING_PLUGIN_LOFI_DRAW_TARGET_ATTACHMENT_DESC_ARRAY_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"
#include "pxr/imaging/plugin/LoFi/drawTargetAttachmentDesc.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class LoFiDrawTargetAttachmentDescArray
///
/// Describes all the color buffer attachments for a draw target.
/// The array should not contain a depth buffer - that is managed
/// separately.
///
/// \note This is a temporary API to aid transition to Storm, and is subject
/// to major changes.
///
/// class is derivable for TfAny support.
///
class LoFiDrawTargetAttachmentDescArray
{
public:
    /// Default constructor for container purposes.
    LOFI_API
    LoFiDrawTargetAttachmentDescArray();

    LOFI_API
    LoFiDrawTargetAttachmentDescArray(size_t attachmentCount);
    virtual ~LoFiDrawTargetAttachmentDescArray() = default;

    LOFI_API
    LoFiDrawTargetAttachmentDescArray(const LoFiDrawTargetAttachmentDescArray &copy);
    LOFI_API
    LoFiDrawTargetAttachmentDescArray &operator =(const LoFiDrawTargetAttachmentDescArray &copy);

    /// Pushes a new attachment onto the end of the list of attachments.
    LOFI_API
    void AddAttachment(const std::string &name,
                       HdFormat           format,
                       const VtValue      &clearColor,
                       HdWrap             wrapS,
                       HdWrap             wrapT,
                       HdMinFilter        minFilter,
                       HdMagFilter        magFilter);


    LOFI_API
    size_t GetNumAttachments() const;
    LOFI_API
    const LoFiDrawTargetAttachmentDesc &GetAttachment(size_t idx) const;

    /// Sampler State for Depth attachment
    LOFI_API
    void SetDepthSampler(HdWrap      depthWrapS,
                         HdWrap      depthWrapT,
                         HdMinFilter depthMinFilter,
                         HdMagFilter depthMagFilter);


    HdWrap      GetDepthWrapS()     const { return _depthWrapS;     }
    HdWrap      GetDepthWrapT()     const { return _depthWrapT;     }
    HdMinFilter GetDepthMinFilter() const { return _depthMinFilter; }
    HdMagFilter GetDepthMagFilter() const { return _depthMagFilter; }

    // Depth display properties
    LOFI_API
    void SetDepthPriority(HdDepthPriority depthPriority);

    HdDepthPriority GetDepthPriority() const { return _depthPriority; }


    // VtValue requirements
    LOFI_API
    size_t GetHash() const;
    LOFI_API
    void   Dump(std::ostream &out) const;
    LOFI_API
    bool operator==(const LoFiDrawTargetAttachmentDescArray &other) const;
    LOFI_API
    bool operator!=(const LoFiDrawTargetAttachmentDescArray &other) const;

private:
    typedef std::vector<LoFiDrawTargetAttachmentDesc> _AttachmentDescArray;

    _AttachmentDescArray _attachments;

    // Sampler State for Depth attachment
    HdWrap      _depthWrapS;
    HdWrap      _depthWrapT;
    HdMinFilter _depthMinFilter;
    HdMagFilter _depthMagFilter;

    // Depth display properties
    HdDepthPriority _depthPriority;
};

LOFI_API
size_t hash_value(const LoFiDrawTargetAttachmentDescArray &attachments);
LOFI_API
std::ostream &operator <<(std::ostream &out,
                          const LoFiDrawTargetAttachmentDescArray &pv);


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_PLUGIN_LOFI_DRAW_TARGET_ATTACHMENT_DESC_ARRAY_H
