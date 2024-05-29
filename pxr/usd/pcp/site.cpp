//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/site.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/base/tf/hash.h"

#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////

PcpSite::PcpSite()
{
    // Do nothing
}

PcpSite::PcpSite( const PcpLayerStackIdentifier & layerStackIdentifier_,
                  const SdfPath & path_ ) :
    layerStackIdentifier(layerStackIdentifier_),
    path(path_)
{
    // Do nothing
}

PcpSite::PcpSite( const PcpLayerStackPtr & layerStack,
                  const SdfPath & path_ ) :
    path(path_)
{
    if (layerStack) {
        layerStackIdentifier = layerStack->GetIdentifier();
    }
}

PcpSite::PcpSite( const SdfLayerHandle & layer, const SdfPath & path_ ) :
    layerStackIdentifier(layer),
    path(path_)
{
    // Do nothing
}

PcpSite::PcpSite( const PcpLayerStackSite & site ) :
    path(site.path)
{
    if (site.layerStack) {
        layerStackIdentifier = site.layerStack->GetIdentifier();
    }
}

bool
PcpSite::operator==(const PcpSite &rhs) const
{
    return layerStackIdentifier == rhs.layerStackIdentifier
        && path == rhs.path;
}

bool
PcpSite::operator<(const PcpSite &rhs) const
{
    return (layerStackIdentifier < rhs.layerStackIdentifier) ||
           (layerStackIdentifier == rhs.layerStackIdentifier && 
            path < rhs.path);
}

size_t
PcpSite::Hash::operator()(const PcpSite &site) const
{
    return TfHash::Combine(
        site.layerStackIdentifier,
        site.path
    );
}

////////////////////////////////////////////////////////////////////////


PcpLayerStackSite::PcpLayerStackSite()
{
    // Do nothing
}

PcpLayerStackSite::PcpLayerStackSite( const PcpLayerStackRefPtr & layerStack_,
                                      const SdfPath & path_ ) :
    layerStack(layerStack_),
    path(path_)
{
    // Do nothing
}

bool
PcpLayerStackSite::operator==(const PcpLayerStackSite &rhs) const
{
    return layerStack == rhs.layerStack && path == rhs.path;
}

bool
PcpLayerStackSite::operator<(const PcpLayerStackSite &rhs) const
{
    return (layerStack < rhs.layerStack) ||
           (layerStack == rhs.layerStack && path < rhs.path);
}

size_t
PcpLayerStackSite::Hash::operator()(const PcpLayerStackSite &site) const
{
    return TfHash::Combine(
        site.layerStack,
        site.path
    );
}

////////////////////////////////////////////////////////////////////////

std::ostream&
operator<<(std::ostream& s, const PcpSite& x)
{
    return s << x.layerStackIdentifier << "<" << x.path << ">";
}

std::ostream&
operator<<(std::ostream& s, const PcpLayerStackSite& x)
{
    return s << x.layerStack << "<" << x.path << ">";
}

PXR_NAMESPACE_CLOSE_SCOPE
