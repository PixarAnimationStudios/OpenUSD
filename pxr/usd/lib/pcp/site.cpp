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
#include "pxr/usd/pcp/site.h"
#include "pxr/usd/pcp/layerStack.h"
#include <ostream>

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
        and path == rhs.path;
}

bool
PcpSite::operator<(const PcpSite &rhs) const
{
    return (layerStackIdentifier < rhs.layerStackIdentifier) or
           (layerStackIdentifier == rhs.layerStackIdentifier and
            path < rhs.path);
}

size_t
PcpSite::Hash::operator()(const PcpSite &site) const
{
    size_t hash = 0;
    boost::hash_combine(hash, site.layerStackIdentifier);
    boost::hash_combine(hash, site.path);
    return hash;
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
    return layerStack == rhs.layerStack and path == rhs.path;
}

bool
PcpLayerStackSite::operator<(const PcpLayerStackSite &rhs) const
{
    return (layerStack < rhs.layerStack) or
           (layerStack == rhs.layerStack and path < rhs.path);
}

size_t
PcpLayerStackSite::Hash::operator()(const PcpLayerStackSite &site) const
{
    size_t hash = 0;
    boost::hash_combine(hash, site.layerStack);
    boost::hash_combine(hash, site.path);
    return hash;
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
