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
#ifndef USDSKEL_SKELDEFINITION_H
#define USDSKEL_SKELDEFINITION_H

/// \file usdSkel/definition.h

#include "pxr/pxr.h"
#include "pxr/usd/usdSkel/api.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/weakBase.h"

#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdSkel/topology.h"

#include <atomic>
#include <mutex>


PXR_NAMESPACE_OPEN_SCOPE


TF_DECLARE_WEAK_AND_REF_PTRS(UsdSkel_SkelDefinition);


/// \class UsdSkel_SkelDefinition
///
/// Structure storing the core definition of a Skeleton.
/// A definition is a simple cache of the *validated* structure
/// of a skeleton, including its topology, bind pose and rest pose.
/// Skeleton definitions are meant to shared across instances.
class UsdSkel_SkelDefinition : public TfRefBase, public TfWeakBase
{
public:
    virtual ~UsdSkel_SkelDefinition() {}

    /// Create a definition from a skeleton.
    /// Returns an empty pointer if \p skel or its structure is invalid.
    static UsdSkel_SkelDefinitionRefPtr New(const UsdSkelSkeleton& skel);

    explicit operator bool() const { 
        return static_cast<bool>(_skel); 
    }

    const UsdSkelSkeleton& GetSkeleton() const { return _skel; }

    const VtTokenArray&    GetJointOrder() const { return _jointOrder; }

    const UsdSkelTopology& GetTopology() const { return _topology; }

    /// Returns rest pose joint transforms in joint-local space.
    template <typename Matrix4>
    bool GetJointLocalRestTransforms(VtArray<Matrix4>* xforms);

    /// Returns rest pose joint transforms in skel space.
    template <typename Matrix4>
    bool GetJointSkelRestTransforms(VtArray<Matrix4>* xforms);

    /// Returns bind pose joint transforms in world space.
    template <typename Matrix4>
    bool GetJointWorldBindTransforms(VtArray<Matrix4>* xforms);

    /// Returns the inverse of the world-space joint bind transforms.
    template <typename Matrix4>
    bool GetJointWorldInverseBindTransforms(VtArray<Matrix4>* xforms);

    /// Returns the inverse of the local-space rest transforms.
    template <typename Matrix4>
    bool GetJointLocalInverseRestTransforms(VtArray<Matrix4>* xforms);

private:
    UsdSkel_SkelDefinition();

    bool _Init(const UsdSkelSkeleton& skel);

    template <int ComputeFlag, typename Matrix4>
    bool _GetJointSkelRestTransforms(VtArray<Matrix4>* xforms);

    template <int ComputeFlag, typename Matrix4>
    bool _GetJointWorldInverseBindTransforms(VtArray<Matrix4>* xforms);

    template <int ComputeFlag, typename Matrix4>
    bool _GetJointLocalInverseRestTransforms(VtArray<Matrix4>* xforms);

    template <int ComputeFlag, typename Matrix4>
    bool _ComputeJointSkelRestTransforms();
    
    template <int ComputeFlag, typename Matrix4>
    bool _ComputeJointWorldInverseBindTransforms();

    template <int ComputeFlag, typename Matrix4>
    bool _ComputeJointLocalInverseRestTransforms();

    /// Helper for managing a set of cached transforms
    /// with both float and double precision.
    struct _XformHolder {
        template <typename Matrix4>
        VtArray<Matrix4>& Get();

        template <typename Matrix4>
        const VtArray<Matrix4>& Get() const;

        VtMatrix4dArray xforms4d;
        VtMatrix4fArray xforms4f;
    };

private:
    UsdSkelSkeleton _skel;
    VtTokenArray    _jointOrder;
    UsdSkelTopology _topology;
    VtMatrix4dArray _jointLocalRestXforms;
    VtMatrix4dArray _jointWorldBindXforms;
    // Properties computed (and cached) on-demand.
    // Different forms of transforms are cached because different
    // consumption tasks generally require different transforms.    
    // They are cached on the definition in order to provide cache
    // sharing across instanced skeletons.
    _XformHolder _jointSkelRestXforms;
    _XformHolder _jointWorldInverseBindXforms;
    _XformHolder _jointLocalInverseRestXforms;

    std::atomic<int> _flags;
    std::mutex _mutex;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDSKEL_SKELDEFINITION_H
