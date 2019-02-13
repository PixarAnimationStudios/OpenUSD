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
/// \file LayerStackIdentifier.cpp

#include "pxr/pxr.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/stringUtils.h"

#include <boost/functional/hash.hpp>
#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

PcpLayerStackIdentifier::PcpLayerStackIdentifier() : _hash(0)
{
    // Do nothing
}

PcpLayerStackIdentifier::PcpLayerStackIdentifier(
    const SdfLayerHandle& rootLayer_,
    const SdfLayerHandle& sessionLayer_,
    const ArResolverContext& pathResolverContext_) :
    rootLayer(rootLayer_),
    sessionLayer(sessionLayer_),
    pathResolverContext(pathResolverContext_),
    _hash(rootLayer ? _ComputeHash() : 0)
{
    // Do nothing
}

PcpLayerStackIdentifier&
PcpLayerStackIdentifier::operator=(const PcpLayerStackIdentifier& rhs)
{
    if (this != &rhs) {
        const_cast<SdfLayerHandle&>(rootLayer)    = rhs.rootLayer;
        const_cast<SdfLayerHandle&>(sessionLayer) = rhs.sessionLayer;
        const_cast<ArResolverContext&>
                           (pathResolverContext) = rhs.pathResolverContext;
        const_cast<size_t&>(_hash)               = rhs._hash;
    }
    return *this;
}

PcpLayerStackIdentifier::operator UnspecifiedBoolType() const
{
    return rootLayer ? &This::_hash : NULL;
}

bool
PcpLayerStackIdentifier::operator==(const This &rhs) const
{
    return _hash           == rhs._hash         &&
           rootLayer       == rhs.rootLayer     &&
           sessionLayer    == rhs.sessionLayer  &&
           pathResolverContext == rhs.pathResolverContext;
}

bool
PcpLayerStackIdentifier::operator<(const This &rhs) const
{
    if (sessionLayer < rhs.sessionLayer)
        return true;
    if (rhs.sessionLayer < sessionLayer)
        return false;
    if (rootLayer < rhs.rootLayer)
        return true;
    if (rhs.rootLayer < rootLayer)
        return false;
    return pathResolverContext < rhs.pathResolverContext;
}

size_t
PcpLayerStackIdentifier::_ComputeHash() const
{
    size_t hash = 0;
    boost::hash_combine(hash, TfHash()(rootLayer));
    boost::hash_combine(hash, TfHash()(sessionLayer));
    boost::hash_combine(hash, pathResolverContext);
    return hash;
}

PcpLayerStackIdentifierStr::PcpLayerStackIdentifierStr(
        std::string const &rootLayerId,
        std::string const &sessionLayerId,
        ArResolverContext const &resolverContext)
    : rootLayerId(rootLayerId)
    , sessionLayerId(sessionLayerId)
    , pathResolverContext(resolverContext)
    , _hash(!rootLayerId.empty() ? _ComputeHash() : 0)
{
    // Do nothing
}

PcpLayerStackIdentifierStr::PcpLayerStackIdentifierStr(
    PcpLayerStackIdentifier const &id)
    : rootLayerId(id.rootLayer ? id.rootLayer->GetIdentifier() : std::string())
    , sessionLayerId(id.sessionLayer ?
                     id.sessionLayer->GetIdentifier() : std::string())
    , pathResolverContext(id.pathResolverContext)
    , _hash(!rootLayerId.empty() ? _ComputeHash() : 0)
{
    // Do nothing
}

PcpLayerStackIdentifierStr::operator UnspecifiedBoolType() const
{
    return rootLayerId.empty() ? &This::_hash : nullptr;
}

bool
PcpLayerStackIdentifierStr::operator==(const This &rhs) const
{
    return _hash == rhs._hash &&
           rootLayerId == rhs.rootLayerId &&
           sessionLayerId == rhs.sessionLayerId &&
           pathResolverContext == rhs.pathResolverContext;
}

bool
PcpLayerStackIdentifierStr::operator<(const This &rhs) const
{
    if (sessionLayerId < rhs.sessionLayerId)
        return true;
    if (rhs.sessionLayerId < sessionLayerId)
        return false;
    if (rootLayerId < rhs.rootLayerId)
        return true;
    if (rhs.rootLayerId < rootLayerId)
        return false;
    return pathResolverContext < rhs.pathResolverContext;
}

size_t
PcpLayerStackIdentifierStr::_ComputeHash() const
{
    size_t hash = 0;
    boost::hash_combine(hash, TfHash()(rootLayerId));
    boost::hash_combine(hash, TfHash()(sessionLayerId));
    boost::hash_combine(hash, pathResolverContext);
    return hash;
}

enum Pcp_IdentifierFormat {
    Pcp_IdentifierFormatIdentifier,     // Must be zero for correct default.
    Pcp_IdentifierFormatRealPath,
    Pcp_IdentifierFormatBaseName
};

static long
Pcp_IdentifierFormatIndex()
{
    static const long index = std::ios_base::xalloc();
    return index;
}

static std::string
Pcp_FormatIdentifier(std::ostream& os, const SdfLayerHandle& layer)
{
    if (!layer) {
        return std::string("<expired>");
    }

    switch (os.iword(Pcp_IdentifierFormatIndex())) {
    default:
    case Pcp_IdentifierFormatIdentifier:
        return layer->GetIdentifier();

    case Pcp_IdentifierFormatRealPath:
        return layer->GetRealPath();

    case Pcp_IdentifierFormatBaseName:
        return TfGetBaseName(layer->GetIdentifier());
    }
}

static std::string
Pcp_FormatIdentifier(std::ostream& os, std::string const& layerId)
{
    if (layerId.empty()) {
        return std::string("<empty>");
    }

    switch (os.iword(Pcp_IdentifierFormatIndex())) {
    default:
    case Pcp_IdentifierFormatIdentifier:
    case Pcp_IdentifierFormatRealPath:
        return layerId;

    case Pcp_IdentifierFormatBaseName:
        return TfGetBaseName(layerId);
    }
}

std::ostream& PcpIdentifierFormatBaseName(std::ostream& os)
{
    os.iword(Pcp_IdentifierFormatIndex()) = Pcp_IdentifierFormatBaseName;
    return os;
}

std::ostream& PcpIdentifierFormatRealPath(std::ostream& os)
{
    os.iword(Pcp_IdentifierFormatIndex()) = Pcp_IdentifierFormatRealPath;
    return os;
}

std::ostream& PcpIdentifierFormatIdentifier(std::ostream& os)
{
    os.iword(Pcp_IdentifierFormatIndex()) = Pcp_IdentifierFormatIdentifier;
    return os;
}

std::ostream&
operator<<(std::ostream& s, const PcpLayerStackIdentifier& x)
{
    // XXX: Should probably write the resolver context, too.
    if (x.sessionLayer) {
        return s << "@" << Pcp_FormatIdentifier(s, x.rootLayer) << "@,"
                 << "@" << Pcp_FormatIdentifier(s, x.sessionLayer) << "@"
                 << PcpIdentifierFormatIdentifier;
    }
    else {
        return s << "@" << Pcp_FormatIdentifier(s, x.rootLayer) << "@"
                 << PcpIdentifierFormatIdentifier;
    }
}

std::ostream&
operator<<(std::ostream& s, const PcpLayerStackIdentifierStr& x)
{
    // XXX: Should probably write the resolver context, too.
    if (!x.sessionLayerId.empty()) {
        return s << "@" << Pcp_FormatIdentifier(s, x.rootLayerId) << "@,"
                 << "@" << Pcp_FormatIdentifier(s, x.sessionLayerId) << "@"
                 << PcpIdentifierFormatIdentifier;
    }
    else {
        return s << "@" << Pcp_FormatIdentifier(s, x.rootLayerId) << "@"
                 << PcpIdentifierFormatIdentifier;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
