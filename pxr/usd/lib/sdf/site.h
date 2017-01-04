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
#ifndef SDF_SITE_H
#define SDF_SITE_H

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"

#include <set>
#include <vector>
#include <boost/operators.hpp>

/// \class SdfSite
///
/// An SdfSite is a simple representation of a location in a layer where 
/// opinions may possibly be found. It is simply a pair of layer and path
/// within that layer.
///
class SdfSite 
    : public boost::totally_ordered<SdfSite>
{
public:
    SdfSite() { }
    SdfSite(const SdfLayerHandle& layer_, const SdfPath& path_)
        : layer(layer_)
        , path(path_)
    { }

    bool operator==(const SdfSite& other) const
    {
        return layer == other.layer && path == other.path;
    }

    bool operator<(const SdfSite& other) const
    {
        return layer < other.layer ||
               (!(other.layer < layer) && path < other.path);
    }

#if !defined(doxygen)
    typedef SdfPath SdfSite::*UnspecifiedBoolType;
#endif

    /// Returns true if both layer and path fields are filled with valid values. 
    /// This does NOT imply that there are opinions in the layer at that path.
    operator UnspecifiedBoolType() const
    {
        return (layer && !path.IsEmpty()) ? &SdfSite::path : 0;
    }

public:
    SdfLayerHandle layer;
    SdfPath path;
};

typedef std::set<SdfSite> SdfSiteSet;
typedef std::vector<SdfSite> SdfSiteVector;

#endif // SDF_SITE_H
