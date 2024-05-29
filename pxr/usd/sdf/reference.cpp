//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Reference.cpp
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/reference.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

#include <algorithm>
#include <functional>
#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

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
    // Pass through SdfAssetPath() to issue an error and produce empty string if
    // \p assetPath contains invalid characters.
    _assetPath(SdfAssetPath(assetPath).GetAssetPath()),
    _primPath(primPath),
    _layerOffset(layerOffset),
    _customData(customData)
{
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
SdfReference::IsInternal() const
{
    return _assetPath.empty();
}

bool
SdfReference::operator==(const SdfReference &rhs) const
{
    return _assetPath   == rhs._assetPath   &&
           _primPath    == rhs._primPath    &&
           _layerOffset == rhs._layerOffset &&
           _customData  == rhs._customData;
}

bool
SdfReference::operator<(const SdfReference &rhs) const
{
    // XXX: This would be much cleaner and less error prone if we used 
    // std::tie for comparison, however, it's not ideal given the awkward 
    // (and not truly correct) comparison of customData size. If customData 
    // is ever removed this should be updated to use the cleaner std::tie.
    return (_assetPath < rhs._assetPath || (_assetPath == rhs._assetPath && 
        (_primPath < rhs._primPath || (_primPath == rhs._primPath && 
        (_layerOffset < rhs._layerOffset || (_layerOffset == rhs._layerOffset &&
        (_customData.size() < rhs._customData.size())))))));
}

int
SdfFindReferenceByIdentity(
    const SdfReferenceVector &references,
    const SdfReference &referenceId)
{
    SdfReferenceVector::const_iterator it = std::find_if(
        references.begin(), references.end(),
        [&referenceId](SdfReference const &ref) {
            return SdfReference::IdentityEqual()(referenceId, ref);
        });

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

PXR_NAMESPACE_CLOSE_SCOPE
