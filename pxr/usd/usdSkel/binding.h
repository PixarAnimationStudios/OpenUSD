//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_SKEL_BINDING_H
#define PXR_USD_USD_SKEL_BINDING_H

/// \file usdSkel/binding.h

#include "pxr/pxr.h"
#include "pxr/usd/usdSkel/api.h"

#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdSkel/skinningQuery.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdSkelBinding
///
/// Helper object that describes the binding of a skeleton to a set of
/// skinnable objects. The set of skinnable objects is given as
/// UsdSkelSkinningQuery prims, which can be used both to identify the
/// skinned prim as well compute skinning properties of the prim.
class UsdSkelBinding
{
public:
    UsdSkelBinding() {}

    UsdSkelBinding(const UsdSkelSkeleton& skel,
                   const VtArray<UsdSkelSkinningQuery>& skinningQueries)
        : _skel(skel), _skinningQueries(skinningQueries) {}

    /// Returns the bound skeleton.
    const UsdSkelSkeleton& GetSkeleton() const { return _skel; }

    /// Returns the set skinning targets.
    const VtArray<UsdSkelSkinningQuery>& GetSkinningTargets() const
        { return _skinningQueries; }

private:
    UsdSkelSkeleton _skel;
    VtArray<UsdSkelSkinningQuery> _skinningQueries;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDSKEL_SKINNING_MAP
