//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"

#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

bool operator==(
    const HgiGraphicsCmdsDesc& lhs,
    const HgiGraphicsCmdsDesc& rhs) 
{
    return  lhs.depthAttachmentDesc == rhs.depthAttachmentDesc &&
            lhs.colorAttachmentDescs == rhs.colorAttachmentDescs &&
            lhs.depthTexture == rhs.depthTexture &&
            lhs.depthResolveTexture == rhs.depthResolveTexture &&
            lhs.colorTextures == rhs.colorTextures &&
            lhs.colorResolveTextures == rhs.colorResolveTextures;
}

bool operator!=(
    const HgiGraphicsCmdsDesc& lhs,
    const HgiGraphicsCmdsDesc& rhs)
{
    return !(lhs == rhs);
}

std::ostream& operator<<(
    std::ostream& out,
    const HgiGraphicsCmdsDesc& desc)
{
    out << "HgiGraphicsCmdsDesc: {";

    for (HgiAttachmentDesc const& a : desc.colorAttachmentDescs) {
        out << a;
    }
    
    for (size_t i=0; i<desc.colorTextures.size(); i++) {
        out << "colorTexture" << i << " ";
        out << "dimensions:" << 
            desc.colorTextures[i]->GetDescriptor().dimensions << ", ";
    }

    for (size_t i=0; i<desc.colorResolveTextures.size(); i++) {
        out << "colorResolveTexture" << i << ", ";
    }

    if (desc.depthTexture) {
        out << desc.depthAttachmentDesc;
        out << "depthTexture ";
        out << "dimensions:" << desc.depthTexture->GetDescriptor().dimensions;
    }

    if (desc.depthResolveTexture) {
        out << "depthResolveTexture";
    }

    out << "}";
    return out;
}


PXR_NAMESPACE_CLOSE_SCOPE
