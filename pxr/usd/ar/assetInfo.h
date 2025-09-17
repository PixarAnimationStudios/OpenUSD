//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_AR_ASSET_INFO_H
#define PXR_USD_AR_ASSET_INFO_H

/// \file ar/assetInfo.h

#include "pxr/pxr.h"
#include "pxr/usd/ar/api.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/vt/value.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class ArAssetInfo
///
/// Contains information about a resolved asset.
///
class ArAssetInfo 
{
public:
    /// Version of the resolved asset, if any.
    std::string version;

    /// The name of the asset represented by the resolved
    /// asset, if any.
    std::string assetName;

    /// \deprecated
    /// The repository path corresponding to the resolved asset.
    std::string repoPath;

    /// Additional information specific to the active plugin
    /// asset resolver implementation.
    VtValue resolverInfo;
};

template <class HashState>
void TfHashAppend(HashState& h, const ArAssetInfo& info)
{
    h.Append(info.version, info.assetName, info.repoPath, info.resolverInfo);
}

inline
size_t hash_value(const ArAssetInfo& info)
{
    return TfHash()(info);
}

/// \relates ArAssetInfo
inline
void swap(ArAssetInfo& lhs, ArAssetInfo& rhs)
{
    lhs.version.swap(rhs.version);
    lhs.assetName.swap(rhs.assetName);
    lhs.repoPath.swap(rhs.repoPath);
    lhs.resolverInfo.Swap(rhs.resolverInfo);
}

/// \relates ArAssetInfo
AR_API
bool 
operator==(const ArAssetInfo& lhs, const ArAssetInfo& rhs);

/// \relates ArAssetInfo
AR_API
bool 
operator!=(const ArAssetInfo& lhs, const ArAssetInfo& rhs);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_AR_ASSET_INFO_H
