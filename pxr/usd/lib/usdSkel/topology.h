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
#ifndef USDSKEL_TOPOLOGY_H
#define USDSKEL_TOPOLOGY_H

/// \file usdSkel/topology.h

#include "pxr/pxr.h"
#include "pxr/usd/usdSkel/api.h"

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
    USDSKEL_API
    UsdSkelTopology();

    /// Construct a skel topology from an ordered set of joint paths,
    /// given as tokens.
    USDSKEL_API
    UsdSkelTopology(const VtTokenArray& paths);

    /// Construct a skel topology from an ordered set of joint paths.
    USDSKEL_API
    UsdSkelTopology(const SdfPathVector& paths);

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

    inline const VtIntArray& GetParentIndices() const;

    inline size_t GetNumJoints() const;

    /// Returns the parent joint of the \p index'th joint,
    /// Returns -1 for joints with no parent (roots).
    inline int GetParent(size_t index) const;

private:
    VtIntArray _parentIndices;
    const int* _parentIndicesData;
};


const VtIntArray&
UsdSkelTopology::GetParentIndices() const
{
    return _parentIndices;
}


size_t
UsdSkelTopology::GetNumJoints() const
{
    return _parentIndices.size();
}


int
UsdSkelTopology::GetParent(size_t index) const
{
    TF_DEV_AXIOM(index < _parentIndices.size());
    return _parentIndicesData[index];
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDSKEL_TOPOLOGY_H
