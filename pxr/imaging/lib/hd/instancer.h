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
#ifndef HD_INSTANCER_H
#define HD_INSTANCER_H

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/tf/hashmap.h"

#include <mutex>

class HdSceneDelegate;
typedef boost::shared_ptr<class HdBufferArrayRange> HdBufferArrayRangeSharedPtr;

/// \class HdInstancer
///
/// This class exists to facilitate point cloud style instancing.
///
/// The primary role of this class is to provide two buffer resources,
/// instance primvars and instance indices.
///
/// 1. instance primvars
///   per-instance primvars. typically translate, rotate and scale.
///   The codegen generates accessors to these values so that the shader
///   can apply instance-specific transforms etc.
///
/// 2. instance indices
///   Index indirection buffer to achieve sparse rendering for given
///   instance primvars with a single draw call.
///
/// All data access (aside from local caches) is delegated to the HdSceneDelegate.
///
/// HdInstancer can be nested. If parentInstancerId is given as an non-empty
/// path, instance primvars and instance indices will be computed recursively
/// by ascending the hierarchy (XXX: not implemented yet)
///
class HdInstancer {
public:
    /// Constructor.
    HdInstancer(HdSceneDelegate* delegate, SdfPath const& id,
                SdfPath const &parentInstancerId);

    /// Returns the identifier.
    SdfPath const& GetId() const { return _id; }

    /// Returns the parent instancer identifier.
    SdfPath const& GetParentId() const { return _parentId; }

    /// Populates instance primvars and returns the buffer range.
    HdBufferArrayRangeSharedPtr GetInstancePrimVars(int level);

    /// Populates the instance index indirection buffer for \p prototypeId and
    /// returns the buffer range.
    HdBufferArrayRangeSharedPtr GetInstanceIndices(SdfPath const &prototypeId);

protected:
    HdSceneDelegate* _GetDelegate() const {
        return _delegate;
    }
    void _GetInstanceIndices(SdfPath const &prototypeId,
                             std::vector<VtIntArray> *instanceIndicesArray);

private:
    HdSceneDelegate* _delegate;
    SdfPath _id;
    SdfPath _parentId;
    std::mutex _instanceLock;
    int _numInstancePrimVars;

    HdBufferArrayRangeSharedPtr _instancePrimVarRange;
    TfHashMap<SdfPath,
                         HdBufferArrayRangeSharedPtr,
                         SdfPath::Hash> _instanceIndexRangeMap;
};

#endif  // HD_INSTANCER_H
