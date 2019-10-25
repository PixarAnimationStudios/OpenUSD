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
#include "pxr/imaging/hdSt/drawTargetAttachmentDescArray.h"

PXR_NAMESPACE_OPEN_SCOPE


HdStDrawTargetAttachmentDescArray::HdStDrawTargetAttachmentDescArray()
 : _attachments()
 , _depthWrapS(HdWrapRepeat)
 , _depthWrapT(HdWrapRepeat)
 , _depthMinFilter(HdMinFilterLinear)
 , _depthMagFilter(HdMagFilterLinear)
 , _depthPriority(HdDepthPriorityNearest)
{

}

HdStDrawTargetAttachmentDescArray::HdStDrawTargetAttachmentDescArray(
                                                         size_t attachmentCount)
 : _attachments()
 , _depthWrapS(HdWrapRepeat)
 , _depthWrapT(HdWrapRepeat)
 , _depthMinFilter(HdMinFilterLinear)
 , _depthMagFilter(HdMagFilterLinear)
 , _depthPriority(HdDepthPriorityNearest)
{
    _attachments.reserve(attachmentCount);
}


HdStDrawTargetAttachmentDescArray::HdStDrawTargetAttachmentDescArray(
                                  const HdStDrawTargetAttachmentDescArray &copy)
  : _attachments(copy._attachments)
  , _depthWrapS(copy._depthWrapS)
  , _depthWrapT(copy._depthWrapT)
  , _depthMinFilter(copy._depthMinFilter)
  , _depthMagFilter(copy._depthMagFilter)
  , _depthPriority(copy._depthPriority)
{

}


HdStDrawTargetAttachmentDescArray &
HdStDrawTargetAttachmentDescArray::operator =(
                                  const HdStDrawTargetAttachmentDescArray &copy)
{
    _attachments    = copy._attachments;
    _depthWrapS     = copy._depthWrapS;
    _depthWrapT     = copy._depthWrapT;
    _depthMinFilter = copy._depthMinFilter;
    _depthMagFilter = copy._depthMagFilter;
    _depthPriority  = copy._depthPriority;

    return *this;
}


void
HdStDrawTargetAttachmentDescArray::AddAttachment(
                                                const std::string &name,
                                                HdFormat           format,
                                                const VtValue      &clearColor,
                                                HdWrap             wrapS,
                                                HdWrap             wrapT,
                                                HdMinFilter        minFilter,
                                                HdMagFilter        magFilter)
{
    _attachments.emplace_back(name,
                              format,
                              clearColor,
                              wrapS,
                              wrapT,
                              minFilter,
                              magFilter);
}


size_t
HdStDrawTargetAttachmentDescArray::GetNumAttachments() const
{
    return _attachments.size();
}


const HdStDrawTargetAttachmentDesc &
HdStDrawTargetAttachmentDescArray::GetAttachment(size_t idx) const
{
    return _attachments[idx];
}

void
HdStDrawTargetAttachmentDescArray::SetDepthSampler(
                                                  HdWrap      depthWrapS,
                                                  HdWrap      depthWrapT,
                                                  HdMinFilter depthMinFilter,
                                                  HdMagFilter depthMagFilter)
{
    _depthWrapS = depthWrapS;
    _depthWrapT = depthWrapT;
    _depthMinFilter = depthMinFilter;
    _depthMagFilter = depthMagFilter;
}

void
HdStDrawTargetAttachmentDescArray::SetDepthPriority(
                                                  HdDepthPriority depthPriority)
{
    _depthPriority = depthPriority;
}


size_t
HdStDrawTargetAttachmentDescArray::GetHash() const
{
    size_t hash = boost::hash_value(_attachments);
    boost::hash_combine(hash, _depthWrapS);
    boost::hash_combine(hash, _depthWrapT);
    boost::hash_combine(hash, _depthMinFilter);
    boost::hash_combine(hash, _depthMagFilter);
    boost::hash_combine(hash, _depthPriority);

    return hash;
}


void
HdStDrawTargetAttachmentDescArray::Dump(std::ostream &out) const
{
    size_t numAttachments = _attachments.size();

    out << numAttachments;

    for (size_t attachmentNum = 0; attachmentNum < numAttachments;
                                                                ++attachmentNum)
    {
        const HdStDrawTargetAttachmentDesc &desc = _attachments[attachmentNum];
        out << desc;
    }

    out << _depthWrapS     << " "
        << _depthWrapT     << " "
        << _depthMinFilter << " "
        << _depthMagFilter << " "
        << _depthPriority  << " ";
}


bool
HdStDrawTargetAttachmentDescArray::operator==(
                           const HdStDrawTargetAttachmentDescArray &other) const
{
    return ((_attachments     == other._attachments)    &&
            (_depthWrapS      == other._depthWrapS)     &&
            (_depthWrapT      == other._depthWrapT)     &&
            (_depthMinFilter  == other._depthMinFilter) &&
            (_depthMagFilter  == other._depthMagFilter) &&
            (_depthPriority   == other._depthPriority));
}


bool
HdStDrawTargetAttachmentDescArray::operator!=(
                           const HdStDrawTargetAttachmentDescArray &other) const
{
    return ((_attachments     != other._attachments)    ||
            (_depthWrapS      != other._depthWrapS)     ||
            (_depthWrapT      != other._depthWrapT)     ||
            (_depthMinFilter  != other._depthMinFilter) ||
            (_depthMagFilter  != other._depthMagFilter) ||
            (_depthPriority   != other._depthPriority));
}


size_t hash_value(HdStDrawTargetAttachmentDescArray const &attachments)
{
    return attachments.GetHash();
}

std::ostream &operator <<(std::ostream &out,
                          const HdStDrawTargetAttachmentDescArray &pv)
{
    pv.Dump(out);

    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE

