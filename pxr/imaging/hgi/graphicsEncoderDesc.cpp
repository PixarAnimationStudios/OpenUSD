//
// Copyright 2019 Pixar
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
#include "pxr/imaging/hgi/graphicsEncoderDesc.h"

PXR_NAMESPACE_OPEN_SCOPE

bool operator==(
    const HgiAttachmentDesc& lhs,
    const HgiAttachmentDesc& rhs) 
{
    return  lhs.clearValue == rhs.clearValue &&
            lhs.loadOp == rhs.loadOp &&
            lhs.storeOp == rhs.storeOp &&
            lhs.texture == rhs.texture;
}

bool operator!=(
    const HgiAttachmentDesc& lhs,
    const HgiAttachmentDesc& rhs)
{
    return !(lhs == rhs);
}

std::ostream& operator<<(
    std::ostream& out,
    const HgiAttachmentDesc& attachment)
{
    out << "HgiAttachmentDesc: {"
        << "has_texture: " << (attachment.texture!=nullptr) << ", "
        << "clearValue: " << attachment.clearValue << ", "
        << "loadOp: " << attachment.loadOp << ", "
        << "storeOp: " << attachment.storeOp <<
    "}";
    return out;
}

bool operator==(
    const HgiGraphicsEncoderDesc& lhs,
    const HgiGraphicsEncoderDesc& rhs) 
{
    return  lhs.width == rhs.width &&
            lhs.height == rhs.height &&
            lhs.depthAttachment == rhs.depthAttachment &&
            lhs.colorAttachments == rhs.colorAttachments;
}

bool operator!=(
    const HgiGraphicsEncoderDesc& lhs,
    const HgiGraphicsEncoderDesc& rhs)
{
    return !(lhs == rhs);
}

std::ostream& operator<<(
    std::ostream& out,
    const HgiGraphicsEncoderDesc& encoder)
{
    out << "HgiGraphicsEncoderDesc: {";
    out << "width: " << encoder.width << ", ";
    out << "height: " << encoder.height << ", ";

    for (HgiAttachmentDesc const& a : encoder.colorAttachments) {
        out << a;
    }

    out << encoder.depthAttachment;

    out << "}";
    return out;
}


PXR_NAMESPACE_CLOSE_SCOPE
