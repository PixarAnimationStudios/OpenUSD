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
#ifndef PXR_USD_PCP_SITE_H
#define PXR_USD_PCP_SITE_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/declarePtrs.h"

#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(PcpLayerStack);
class PcpLayerStackSite;

/// \class PcpSite
///
/// A site specifies a path in a layer stack of scene description.
///
class PcpSite
{
public:
    PcpLayerStackIdentifier layerStackIdentifier;
    SdfPath path;

    PCP_API
    PcpSite();

    PCP_API
    PcpSite( const PcpLayerStackIdentifier &, const SdfPath & path );
    PCP_API
    PcpSite( const PcpLayerStackPtr &, const SdfPath & path );
    PCP_API
    PcpSite( const SdfLayerHandle &, const SdfPath & path );
    PCP_API
    PcpSite( const PcpLayerStackSite & );

    PCP_API
    bool operator==(const PcpSite &rhs) const;

    bool operator!=(const PcpSite &rhs) const {
        return !(*this == rhs);
    }
    
    PCP_API
    bool operator<(const PcpSite &rhs) const;

    bool operator<=(const PcpSite &rhs) const {
        return !(rhs < *this);
    }

    bool operator>(const PcpSite &rhs) const {
        return rhs < *this;
    }

    bool operator>=(const PcpSite &rhs) const {
        return !(*this < rhs);
    }

    struct Hash {
        PCP_API
        size_t operator()(const PcpSite &) const;
    };
};

/// \class PcpLayerStackSite
///
/// A site specifies a path in a layer stack of scene description.
///
class PcpLayerStackSite
{
public:
    PcpLayerStackRefPtr layerStack;
    SdfPath path;

    PCP_API
    PcpLayerStackSite();

    PCP_API
    PcpLayerStackSite( const PcpLayerStackRefPtr &, const SdfPath & path );

    PCP_API
    bool operator==(const PcpLayerStackSite &rhs) const;

    bool operator!=(const PcpLayerStackSite &rhs) const {
        return !(*this == rhs);
    }

    PCP_API
    bool operator<(const PcpLayerStackSite &rhs) const;

    bool operator<=(const PcpLayerStackSite &rhs) const {
        return !(rhs < *this);
    }

    bool operator>(const PcpLayerStackSite &rhs) const {
        return rhs < *this;
    }

    bool operator>=(const PcpLayerStackSite &rhs) const {
        return !(*this < rhs);
    }


    struct Hash {
        PCP_API
        size_t operator()(const PcpLayerStackSite &) const;
    };
};

PCP_API
std::ostream& operator<<(std::ostream&, const PcpSite&);
PCP_API
std::ostream& operator<<(std::ostream&, const PcpLayerStackSite&);

static inline
size_t
hash_value(const PcpSite& site)
{
    return PcpSite::Hash()(site);
}

static inline
size_t
hash_value(const PcpLayerStackSite& site)
{
    return PcpLayerStackSite::Hash()(site);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_SITE_H
