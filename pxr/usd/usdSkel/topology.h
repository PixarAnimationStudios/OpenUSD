//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_SKEL_TOPOLOGY_H
#define PXR_USD_USD_SKEL_TOPOLOGY_H

/// \file usdSkel/topology.h

#include "pxr/pxr.h"
#include "pxr/usd/usdSkel/api.h"

#include "pxr/base/tf/span.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/types.h"


PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdSkelTopology
///
/// Object holding information describing skeleton topology.
/// This provides the hierarchical information needed to reason about joint
/// relationships in a manner suitable to computations.
class UsdSkelTopology
{
public:
    /// Construct an empty topology.
    UsdSkelTopology() = default;

    /// Construct a skel topology from \p paths, an array holding ordered joint
    /// paths as tokens.
    /// Internally, each token must be converted to an SdfPath. If SdfPath
    /// objects are already accessible, it is more efficient to use the
    /// construct taking an SdfPath array.
    USDSKEL_API
    UsdSkelTopology(TfSpan<const TfToken> paths);

    /// Construct a skel topology from \p paths, an array of joint paths.
    USDSKEL_API
    UsdSkelTopology(TfSpan<const SdfPath> paths);

    /// Construct a skel topology from an array of parent indices.
    /// For each joint, this provides the parent index of that
    /// joint, or -1 if none.
    USDSKEL_API
    UsdSkelTopology(const VtIntArray& parentIndices);

    /// Validate the topology.
    /// If validation is unsuccessful, a reason
    /// why will be written to \p reason, if provided.
    USDSKEL_API
    bool Validate(std::string* reason=nullptr) const;

    const VtIntArray& GetParentIndices() const { return _parentIndices; }

    size_t GetNumJoints() const { return size(); }

    size_t size() const { return _parentIndices.size(); }

    /// Returns the parent joint of the \p index'th joint,
    /// Returns -1 for joints with no parent (roots).
    inline int GetParent(size_t index) const;

    /// Returns true if the \p index'th joint is a root joint.
    bool IsRoot(size_t index) const { return GetParent(index) < 0; }

    bool operator==(const UsdSkelTopology& o) const;

    bool operator!=(const UsdSkelTopology& o) const {
        return !(*this == o);
    }

private:
    VtIntArray _parentIndices;
};


int
UsdSkelTopology::GetParent(size_t index) const
{
    TF_DEV_AXIOM(index < _parentIndices.size());
    return _parentIndices[index];
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_SKEL_TOPOLOGY_H
