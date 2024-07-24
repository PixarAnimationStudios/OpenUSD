//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Payload.cpp
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/payload.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<SdfPayload>();
    TfType::Define<SdfPayloadVector>();
}

SdfPayload::SdfPayload(
    const std::string &assetPath,
    const SdfPath &primPath,
    const SdfLayerOffset &layerOffset) :
    // Pass through SdfAssetPath() to issue an error and produce empty string if
    // \p assetPath contains invalid characters.
    _assetPath(SdfAssetPath(assetPath).GetAssetPath()),
    _primPath(primPath),
    _layerOffset(layerOffset)
{
}

bool
SdfPayload::operator==(const SdfPayload &rhs) const
{
    return _assetPath   == rhs._assetPath   &&
           _primPath    == rhs._primPath    &&
           _layerOffset == rhs._layerOffset;
}

bool
SdfPayload::operator<(const SdfPayload &rhs) const
{
    return (_assetPath < rhs._assetPath || (_assetPath == rhs._assetPath && 
           (_primPath < rhs._primPath || (_primPath == rhs._primPath && 
           (_layerOffset <rhs._layerOffset)))));
}

std::ostream & operator<<( std::ostream &out,
                           const SdfPayload &payload )
{
    return out << "SdfPayload("
        << payload.GetAssetPath() << ", "
        << payload.GetPrimPath() << ", "
        << payload.GetLayerOffset() << ")";
}

PXR_NAMESPACE_CLOSE_SCOPE
