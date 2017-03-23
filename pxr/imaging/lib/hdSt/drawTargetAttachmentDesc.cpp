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
#include "pxr/imaging/hdSt/drawTargetAttachmentDesc.h"

PXR_NAMESPACE_OPEN_SCOPE


HdStDrawTargetAttachmentDesc::HdStDrawTargetAttachmentDesc()
 : _name()
 , _format(HdFormatUnknown)
 , _clearColor()
 , _wrapS(HdWrapRepeat)
 , _wrapT(HdWrapRepeat)
 , _minFilter(HdMinFilterLinear)
 , _magFilter(HdMagFilterLinear)
{

}


HdStDrawTargetAttachmentDesc::HdStDrawTargetAttachmentDesc(
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


HdStDrawTargetAttachmentDesc::HdStDrawTargetAttachmentDesc(
                                       const HdStDrawTargetAttachmentDesc &copy)
  : _name(copy._name)
  , _format(copy._format)
  , _clearColor(copy._clearColor)
  , _wrapS(copy._wrapS)
  , _wrapT(copy._wrapT)
  , _minFilter(copy._minFilter)
  , _magFilter(copy._magFilter)
{

}


HdStDrawTargetAttachmentDesc &
HdStDrawTargetAttachmentDesc::operator =(
                                       const HdStDrawTargetAttachmentDesc &copy)
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
HdStDrawTargetAttachmentDesc::GetHash() const
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
HdStDrawTargetAttachmentDesc::Dump(std::ostream &out) const
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
HdStDrawTargetAttachmentDesc::operator==(
                                const HdStDrawTargetAttachmentDesc &other) const
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
HdStDrawTargetAttachmentDesc::operator!=(
                                const HdStDrawTargetAttachmentDesc &other) const
{
    return ((_name       != other._name)       ||
            (_format     != other._format)     ||
            (_clearColor != other._clearColor) ||
            (_wrapS      != other._wrapS)      ||
            (_wrapT      != other._wrapT)      ||
            (_minFilter  != other._minFilter)  ||
            (_magFilter  != other._magFilter));
}


size_t hash_value(HdStDrawTargetAttachmentDesc const &attachment)
{
    return attachment.GetHash();
}

std::ostream &operator <<(std::ostream &out,
                          const HdStDrawTargetAttachmentDesc &pv)
{
    pv.Dump(out);

    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE

