//
// Copyright 2023 Pixar
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
