//
// Copyright 2020 benmalartre
//
// Unlicensed
// 
#include "pxr/imaging/plugin/LoFi/drawTargetAttachmentDesc.h"

PXR_NAMESPACE_OPEN_SCOPE


LoFiDrawTargetAttachmentDesc::LoFiDrawTargetAttachmentDesc()
 : _name()
 , _format(HdFormatInvalid)
 , _clearColor()
 , _wrapS(HdWrapRepeat)
 , _wrapT(HdWrapRepeat)
 , _minFilter(HdMinFilterLinear)
 , _magFilter(HdMagFilterLinear)
{

}


LoFiDrawTargetAttachmentDesc::LoFiDrawTargetAttachmentDesc(
                                                  const std::string &name,
                                                  HdFormat           format,
                                                  const VtValue     &clearColor,
                                                  HdWrap             wrapS,
                                                  HdWrap             wrapT,
                                                  HdMinFilter        minFilter,
                                                  HdMagFilter        magFilter)
  : _name(name)
  , _format(format)
  , _clearColor(clearColor)
  , _wrapS(wrapS)
  , _wrapT(wrapT)
  , _minFilter(minFilter)
  , _magFilter(magFilter)
{

}


LoFiDrawTargetAttachmentDesc::LoFiDrawTargetAttachmentDesc(
                                       const LoFiDrawTargetAttachmentDesc &copy)
  : _name(copy._name)
  , _format(copy._format)
  , _clearColor(copy._clearColor)
  , _wrapS(copy._wrapS)
  , _wrapT(copy._wrapT)
  , _minFilter(copy._minFilter)
  , _magFilter(copy._magFilter)
{

}


LoFiDrawTargetAttachmentDesc &
LoFiDrawTargetAttachmentDesc::operator =(
                                       const LoFiDrawTargetAttachmentDesc &copy)
{
    _name       = copy._name;
    _format     = copy._format;
    _clearColor = copy._clearColor;
    _wrapS      = copy._wrapS;
    _wrapT      = copy._wrapT;
    _minFilter  = copy._minFilter;
    _magFilter  = copy._magFilter;

    return *this;
}


size_t
LoFiDrawTargetAttachmentDesc::GetHash() const
{
    size_t hash = boost::hash_value(_name);
    boost::hash_combine(hash, _format);
    boost::hash_combine(hash, _clearColor);
    boost::hash_combine(hash, _wrapS);
    boost::hash_combine(hash, _wrapT);
    boost::hash_combine(hash, _minFilter);
    boost::hash_combine(hash, _magFilter);

    return hash;
}


void
LoFiDrawTargetAttachmentDesc::Dump(std::ostream &out) const
{
    out << _name        << " "
        << _format      << " "
        << _clearColor  << " "
        << _wrapS       << " "
        << _wrapT       << " "
        << _minFilter   << " "
        << _magFilter;
}


bool
LoFiDrawTargetAttachmentDesc::operator==(
                                const LoFiDrawTargetAttachmentDesc &other) const
{
    return ((_name       == other._name)   &&
            (_format     == other._format) &&
            (_clearColor == other._clearColor) &&
            (_wrapS      == other._wrapS) &&
            (_wrapT      == other._wrapT) &&
            (_minFilter  == other._minFilter) &&
            (_magFilter  == other._magFilter));
}


bool
LoFiDrawTargetAttachmentDesc::operator!=(
                                const LoFiDrawTargetAttachmentDesc &other) const
{
    return ((_name       != other._name)       ||
            (_format     != other._format)     ||
            (_clearColor != other._clearColor) ||
            (_wrapS      != other._wrapS)      ||
            (_wrapT      != other._wrapT)      ||
            (_minFilter  != other._minFilter)  ||
            (_magFilter  != other._magFilter));
}


size_t hash_value(LoFiDrawTargetAttachmentDesc const &attachment)
{
    return attachment.GetHash();
}

std::ostream &operator <<(std::ostream &out,
                          const LoFiDrawTargetAttachmentDesc &pv)
{
    pv.Dump(out);

    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE

