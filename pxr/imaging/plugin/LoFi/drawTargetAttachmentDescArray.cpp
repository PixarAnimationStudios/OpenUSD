//
// Copyright 2020 benmalartre
//
// Unlicensed
// 
#include "pxr/imaging/plugin/LoFi/drawTargetAttachmentDescArray.h"

PXR_NAMESPACE_OPEN_SCOPE


LoFiDrawTargetAttachmentDescArray::LoFiDrawTargetAttachmentDescArray()
 : _attachments()
 , _depthWrapS(HdWrapRepeat)
 , _depthWrapT(HdWrapRepeat)
 , _depthMinFilter(HdMinFilterLinear)
 , _depthMagFilter(HdMagFilterLinear)
 , _depthPriority(HdDepthPriorityNearest)
{

}

LoFiDrawTargetAttachmentDescArray::LoFiDrawTargetAttachmentDescArray(
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


LoFiDrawTargetAttachmentDescArray::LoFiDrawTargetAttachmentDescArray(
                                  const LoFiDrawTargetAttachmentDescArray &copy)
  : _attachments(copy._attachments)
  , _depthWrapS(copy._depthWrapS)
  , _depthWrapT(copy._depthWrapT)
  , _depthMinFilter(copy._depthMinFilter)
  , _depthMagFilter(copy._depthMagFilter)
  , _depthPriority(copy._depthPriority)
{

}


LoFiDrawTargetAttachmentDescArray &
LoFiDrawTargetAttachmentDescArray::operator =(
                                  const LoFiDrawTargetAttachmentDescArray &copy)
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
LoFiDrawTargetAttachmentDescArray::AddAttachment(
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
LoFiDrawTargetAttachmentDescArray::GetNumAttachments() const
{
    return _attachments.size();
}


const LoFiDrawTargetAttachmentDesc &
LoFiDrawTargetAttachmentDescArray::GetAttachment(size_t idx) const
{
    return _attachments[idx];
}

void
LoFiDrawTargetAttachmentDescArray::SetDepthSampler(
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
LoFiDrawTargetAttachmentDescArray::SetDepthPriority(
                                                  HdDepthPriority depthPriority)
{
    _depthPriority = depthPriority;
}


size_t
LoFiDrawTargetAttachmentDescArray::GetHash() const
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
LoFiDrawTargetAttachmentDescArray::Dump(std::ostream &out) const
{
    size_t numAttachments = _attachments.size();

    out << numAttachments;

    for (size_t attachmentNum = 0; attachmentNum < numAttachments;
                                                                ++attachmentNum)
    {
        const LoFiDrawTargetAttachmentDesc &desc = _attachments[attachmentNum];
        out << desc;
    }

    out << _depthWrapS     << " "
        << _depthWrapT     << " "
        << _depthMinFilter << " "
        << _depthMagFilter << " "
        << _depthPriority  << " ";
}


bool
LoFiDrawTargetAttachmentDescArray::operator==(
                           const LoFiDrawTargetAttachmentDescArray &other) const
{
    return ((_attachments     == other._attachments)    &&
            (_depthWrapS      == other._depthWrapS)     &&
            (_depthWrapT      == other._depthWrapT)     &&
            (_depthMinFilter  == other._depthMinFilter) &&
            (_depthMagFilter  == other._depthMagFilter) &&
            (_depthPriority   == other._depthPriority));
}


bool
LoFiDrawTargetAttachmentDescArray::operator!=(
                           const LoFiDrawTargetAttachmentDescArray &other) const
{
    return ((_attachments     != other._attachments)    ||
            (_depthWrapS      != other._depthWrapS)     ||
            (_depthWrapT      != other._depthWrapT)     ||
            (_depthMinFilter  != other._depthMinFilter) ||
            (_depthMagFilter  != other._depthMagFilter) ||
            (_depthPriority   != other._depthPriority));
}


size_t hash_value(LoFiDrawTargetAttachmentDescArray const &attachments)
{
    return attachments.GetHash();
}

std::ostream &operator <<(std::ostream &out,
                          const LoFiDrawTargetAttachmentDescArray &pv)
{
    pv.Dump(out);

    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE

