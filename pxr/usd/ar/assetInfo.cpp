//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/ar/assetInfo.h"

PXR_NAMESPACE_OPEN_SCOPE

bool 
operator==(
    const ArAssetInfo& lhs, 
    const ArAssetInfo& rhs)
{
    return (lhs.version == rhs.version) 
        && (lhs.assetName == rhs.assetName)
        && (lhs.repoPath == rhs.repoPath)
        && (lhs.resolverInfo == rhs.resolverInfo);
}

bool 
operator!=(
    const ArAssetInfo& lhs, 
    const ArAssetInfo& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
