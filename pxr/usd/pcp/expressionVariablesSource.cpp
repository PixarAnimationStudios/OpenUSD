//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/pcp/expressionVariablesSource.h"

#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"

PXR_NAMESPACE_OPEN_SCOPE

PcpExpressionVariablesSource::PcpExpressionVariablesSource() = default;

PcpExpressionVariablesSource::PcpExpressionVariablesSource(
    const PcpLayerStackIdentifier& layerStackIdentifier,
    const PcpLayerStackIdentifier& rootLayerStackIdentifier)
    : _identifier(
        layerStackIdentifier == rootLayerStackIdentifier ?
        nullptr : new PcpLayerStackIdentifier(layerStackIdentifier))
{
}

PcpExpressionVariablesSource::~PcpExpressionVariablesSource() = default;

size_t
PcpExpressionVariablesSource::GetHash() const
{
    return _identifier ? TfHash()(*_identifier) : TfHash()(0);
}

bool 
PcpExpressionVariablesSource::operator==(
    const PcpExpressionVariablesSource& rhs) const
{
    if (this == &rhs) {
        return true;
    }

    const bool hasId = static_cast<bool>(_identifier);
    const bool rhsHasId = static_cast<bool>(rhs._identifier);

    if (hasId && rhsHasId) {
        return *_identifier == *rhs._identifier;
    }
    return hasId == rhsHasId;
}

bool
PcpExpressionVariablesSource::operator!=(
    const PcpExpressionVariablesSource& rhs) const
{
    return !(*this == rhs);
}

bool
PcpExpressionVariablesSource::operator<(
    const PcpExpressionVariablesSource& rhs) const
{
    const bool hasId = static_cast<bool>(_identifier);
    const bool rhsHasId = static_cast<bool>(rhs._identifier);
    
    if (hasId && rhsHasId) {
        return *_identifier < *rhs._identifier;
    }
    return hasId < rhsHasId;
}

const PcpLayerStackIdentifier&
PcpExpressionVariablesSource::ResolveLayerStackIdentifier(
    const PcpLayerStackIdentifier& rootLayerStackIdentifier) const
{
    if (IsRootLayerStack()) {
        return rootLayerStackIdentifier;
    }
    return *_identifier;
}

const PcpLayerStackIdentifier&
PcpExpressionVariablesSource::ResolveLayerStackIdentifier(
    const PcpCache& cache) const
{
    return ResolveLayerStackIdentifier(cache.GetLayerStackIdentifier());
}

PXR_NAMESPACE_CLOSE_SCOPE
