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
#include "pxr/imaging/hdx/drawTargetAttachmentDescArray.h"

HdxDrawTargetAttachmentDescArray::HdxDrawTargetAttachmentDescArray()
 : _attachments()
{

}

HdxDrawTargetAttachmentDescArray::HdxDrawTargetAttachmentDescArray(
                                                         size_t attachmentCount)
 : _attachments()
{
    _attachments.reserve(attachmentCount);
}


HdxDrawTargetAttachmentDescArray::HdxDrawTargetAttachmentDescArray(
                                    const HdxDrawTargetAttachmentDescArray &copy)
  : _attachments(copy._attachments)
{

}


HdxDrawTargetAttachmentDescArray &
HdxDrawTargetAttachmentDescArray::operator =(
                                    const HdxDrawTargetAttachmentDescArray &copy)
{
    _attachments = copy._attachments;

    return *this;
}


void
HdxDrawTargetAttachmentDescArray::AddAttachment(const std::string &name,
                                               HdFormat           format,
                                               const VtValue      &clearColor)
{
    _attachments.emplace_back(name, format, clearColor);
}


size_t
HdxDrawTargetAttachmentDescArray::GetNumAttachments() const
{
    return _attachments.size();
}


const HdxDrawTargetAttachmentDesc &
HdxDrawTargetAttachmentDescArray::GetAttachment(size_t idx) const
{
    return _attachments[idx];
}


size_t
HdxDrawTargetAttachmentDescArray::GetHash() const
{
    size_t hash = boost::hash_value(_attachments);

    return hash;
}


void
HdxDrawTargetAttachmentDescArray::Dump(std::ostream &out) const
{
    size_t numAttachments = _attachments.size();

    out << numAttachments;

    for (size_t attachmentNum = 0; attachmentNum < numAttachments;
                                                                ++attachmentNum)
    {
        const HdxDrawTargetAttachmentDesc &desc = _attachments[attachmentNum];
        out << desc;
    }
}


bool
HdxDrawTargetAttachmentDescArray::operator==(
                             const HdxDrawTargetAttachmentDescArray &other) const
{
    return (_attachments == other._attachments);
}


bool
HdxDrawTargetAttachmentDescArray::operator!=(
                            const HdxDrawTargetAttachmentDescArray &other) const
{
    return (_attachments != other._attachments);
}


size_t hash_value(HdxDrawTargetAttachmentDescArray const &attachments)
{
    return attachments.GetHash();
}

std::ostream &operator <<(std::ostream &out,
                          const HdxDrawTargetAttachmentDescArray &pv)
{
    pv.Dump(out);

    return out;
}
