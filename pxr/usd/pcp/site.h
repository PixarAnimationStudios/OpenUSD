//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
