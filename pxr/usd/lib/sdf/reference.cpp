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
// Reference.cpp
//


#include "pxr/usd/sdf/reference.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

#include <boost/bind.hpp>
#include <boost/ref.hpp>

#include <algorithm>
#include <ostream>


TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<SdfReference>();
    TfType::Define<SdfReferenceVector>();
}

SdfReference::SdfReference(
    const std::string &assetPath,
    const SdfPath &primPath,
    const SdfLayerOffset &layerOffset,
    const VtDictionary &customData) :
    _primPath(primPath),
    _layerOffset(layerOffset),
    _customData(customData)
{
    _assetPath = assetPath;
}

void
SdfReference::SetCustomData(const std::string &name, const VtValue &value)
{
    if (value.IsEmpty()) {
        _customData.erase(name);
    } else {
        _customData[name] = value;
    }
}

bool
SdfReference::operator==(const SdfReference &rhs) const
{
    return _assetPath   == rhs._assetPath and
           _primPath    == rhs._primPath and
           _layerOffset == rhs._layerOffset and
           _customData  == rhs._customData;
}

bool
SdfReference::operator<(const SdfReference &rhs) const
{
    return (_assetPath   <  rhs._assetPath) or (
           (_assetPath   == rhs._assetPath and _primPath    <rhs._primPath) or (
           (_primPath    == rhs._primPath  and _layerOffset <rhs._layerOffset) or(
           (_layerOffset == rhs._layerOffset) and
               (_customData.size() < rhs._customData.size()))));
}

int
SdfFindReferenceByIdentity(
    const SdfReferenceVector &references,
    const SdfReference &referenceId)
{
    SdfReferenceVector::const_iterator it = std::find_if(
        references.begin(), references.end(), 
        boost::bind<bool>(
            SdfReference::IdentityEqual(),
            boost::cref(referenceId), _1));

    return it != references.end() ? it - references.begin() : -1;
}

std::ostream & operator<<( std::ostream &out,
                           const SdfReference &reference )
{
    return out << "SdfReference("
        << reference.GetAssetPath() << ", "
        << reference.GetPrimPath() << ", "
        << reference.GetLayerOffset() << ", "
        << reference.GetCustomData() << ")";
}
