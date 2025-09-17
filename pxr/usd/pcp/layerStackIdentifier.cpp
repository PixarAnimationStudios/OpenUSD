//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file LayerStackIdentifier.cpp

#include "pxr/pxr.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/stringUtils.h"

#include <ostream>
#include <tuple>

PXR_NAMESPACE_OPEN_SCOPE

PcpLayerStackIdentifier::PcpLayerStackIdentifier() : _hash(0)
{
    // Do nothing
}

PcpLayerStackIdentifier::PcpLayerStackIdentifier(
    const SdfLayerHandle& rootLayer_,
    const SdfLayerHandle& sessionLayer_,
    const ArResolverContext& pathResolverContext_,
    const PcpExpressionVariablesSource& expressionVariablesOverrideSource_)
    : rootLayer(rootLayer_)
    , sessionLayer(sessionLayer_)
    , pathResolverContext(pathResolverContext_)
    , expressionVariablesOverrideSource(expressionVariablesOverrideSource_)
    , _hash(rootLayer ? _ComputeHash() : 0)
{
    // Do nothing
}

PcpLayerStackIdentifier&
PcpLayerStackIdentifier::operator=(const PcpLayerStackIdentifier& rhs)
{
    if (this != &rhs) {
        const_cast<SdfLayerHandle&>(rootLayer)    = rhs.rootLayer;
        const_cast<SdfLayerHandle&>(sessionLayer) = rhs.sessionLayer;
        const_cast<ArResolverContext&>(pathResolverContext) = 
            rhs.pathResolverContext;
        const_cast<PcpExpressionVariablesSource&>(expressionVariablesOverrideSource) =
            rhs.expressionVariablesOverrideSource;
        const_cast<size_t&>(_hash) = rhs._hash;
    }
    return *this;
}

PcpLayerStackIdentifier::operator bool() const
{
    return static_cast<bool>(rootLayer);
}

bool
PcpLayerStackIdentifier::operator==(const This &rhs) const
{
    return 
        std::tie(
            _hash, rootLayer, sessionLayer,
            pathResolverContext, expressionVariablesOverrideSource) ==
        std::tie(
            rhs._hash, rhs.rootLayer, rhs.sessionLayer, 
            rhs.pathResolverContext, rhs.expressionVariablesOverrideSource);
}

bool
PcpLayerStackIdentifier::operator<(const This &rhs) const
{
    return
        std::tie(
            sessionLayer, rootLayer, pathResolverContext,
            expressionVariablesOverrideSource) <
        std::tie(
            rhs.sessionLayer, rhs.rootLayer, rhs.pathResolverContext,
            rhs.expressionVariablesOverrideSource);
}

size_t
PcpLayerStackIdentifier::_ComputeHash() const
{
    return TfHash::Combine(
        rootLayer, sessionLayer, pathResolverContext,
        expressionVariablesOverrideSource);
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

static void
_PrintIdentifier(std::ostream& s, const PcpLayerStackIdentifier& x)
{
    // XXX: Should probably write the resolver context, too.
    s << "@" << Pcp_FormatIdentifier(s, x.rootLayer) << "@";
    if (x.sessionLayer) {
        s << ",@" << Pcp_FormatIdentifier(s, x.sessionLayer) << "@";
    }
    if (const PcpLayerStackIdentifier* exprOverrideSource =
        x.expressionVariablesOverrideSource.GetLayerStackIdentifier()) {
        s << ",exprVarOverrideSource=";
        _PrintIdentifier(s, *exprOverrideSource);
    }
}

std::ostream&
operator<<(std::ostream& s, const PcpLayerStackIdentifier& x)
{
    _PrintIdentifier(s, x);
    return s << PcpIdentifierFormatIdentifier;
}

PXR_NAMESPACE_CLOSE_SCOPE
