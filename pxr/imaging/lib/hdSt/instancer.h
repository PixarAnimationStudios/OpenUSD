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
#ifndef HDST_INSTANCER_H
#define HDST_INSTANCER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/instancer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/tf/hashmap.h"

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

class HdDrawItem;
struct HdRprimSharedData;

typedef boost::shared_ptr<class HdBufferArrayRange> HdBufferArrayRangeSharedPtr;

/// \class HdStInstancer
///
/// HdSt implements instancing by drawing each proto multiple times with
/// a single draw call.  Application of instance primvars (like transforms)
/// is done in shaders. Instance transforms in particular are computed in
/// ApplyInstanceTransform in instancing.glslfx.
///
/// If this instancer is nested, instance indices will be computed
/// recursively by ascending the heirarchy. HdStInstancer computes a flattened
/// index structure for each prototype by taking the cartesian product of the
/// instance indices at each level.
///
/// For example:
///   - InstancerA draws instances [ProtoX, InstancerB, ProtoX, InstancerB]
///   - InstancerB draws instances [ProtoY, ProtoZ, ProtoY]
/// The flattened index for Proto Y is:
/// [0, 0, 1]; [1, 0, 3]; [2, 2, 1]; [3, 2, 3];
/// where the first tuple element is the position in the flattened index;
/// the second tuple element is the position in Instancer B;
/// and the last tuple element is the position in Instancer A.
///
/// The flattened index gives the number of times the proto is drawn, and the
/// index tuple can be passed to the shader so that each instance can look up
/// its instance primvars in the bound primvar arrays.

class HdStInstancer : public HdInstancer {
public:
    /// Constructor.
    HDST_API
    HdStInstancer(HdSceneDelegate* delegate, SdfPath const &id,
                  SdfPath const &parentInstancerId);

    /// Populates instance primvars and returns the buffer range.
    HDST_API
    HdBufferArrayRangeSharedPtr GetInstancePrimvars();

    /// Populates the instance index indirection buffer for \p prototypeId and
    /// returns the buffer range.
    HDST_API
    HdBufferArrayRangeSharedPtr GetInstanceIndices(SdfPath const &prototypeId);

    /// Populates the rprim's draw item with the appropriate instancer
    /// buffer range data.
    HDST_API
    void PopulateDrawItem(HdDrawItem *drawItem, HdRprimSharedData *sharedData,
                          HdDirtyBits *dirtyBits, int instancePrimvarSlot);

protected:
    HDST_API
    void _GetInstanceIndices(SdfPath const &prototypeId,
                             std::vector<VtIntArray> *instanceIndicesArray);

private:
    std::mutex _instanceLock;
    int _numInstancePrimvars;

    HdBufferArrayRangeSharedPtr _instancePrimvarRange;
    TfHashMap<SdfPath,
              HdBufferArrayRangeSharedPtr,
              SdfPath::Hash> _instanceIndexRangeMap;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDST_INSTANCER_H
