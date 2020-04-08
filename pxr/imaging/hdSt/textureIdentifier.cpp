//
// Copyright 2020 Pixar
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
#include "pxr/imaging/hdSt/textureIdentifier.h"

#include "pxr/imaging/hdSt/subtextureIdentifier.h"

#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE

static
std::unique_ptr<const HdStSubtextureIdentifier>
_CloneSubtextureId(
        std::unique_ptr<const HdStSubtextureIdentifier> const &subtextureId) {
    if (subtextureId) {
        return subtextureId->Clone();
    }
    return nullptr;
}

HdStTextureIdentifier::HdStTextureIdentifier() = default;

HdStTextureIdentifier::HdStTextureIdentifier(
    const TfToken &filePath)
  : _filePath(filePath)
{
}

HdStTextureIdentifier::HdStTextureIdentifier(
    const TfToken &filePath,
    std::unique_ptr<const HdStSubtextureIdentifier> &&subtextureId)
  : _filePath(filePath),
    _subtextureId(std::move(subtextureId))
{
}

HdStTextureIdentifier::HdStTextureIdentifier(
    const HdStTextureIdentifier &textureId)
  : _filePath(textureId._filePath),
    _subtextureId(_CloneSubtextureId(textureId._subtextureId))
{
}

HdStTextureIdentifier &
HdStTextureIdentifier::operator=(HdStTextureIdentifier &&textureId) = default;

HdStTextureIdentifier &
HdStTextureIdentifier::operator=(const HdStTextureIdentifier &textureId)
{
    _filePath = textureId._filePath;
    _subtextureId = _CloneSubtextureId(textureId._subtextureId);

    return *this;
}

HdStTextureIdentifier::~HdStTextureIdentifier() = default;

HdStTextureIdentifier::ID
HdStTextureIdentifier::Hash() const
{
     size_t result = _filePath.Hash();
     if (_subtextureId) {
         boost::hash_combine(result, _subtextureId->Hash());
     }
     return result;
 }

PXR_NAMESPACE_CLOSE_SCOPE
