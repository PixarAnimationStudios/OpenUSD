//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_INSTANCER_H
#define PXR_IMAGING_HD_ST_INSTANCER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/instancer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/tf/hashmap.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdRprim;
class HdStDrawItem;
struct HdRprimSharedData;

using HdBufferArrayRangeSharedPtr = std::shared_ptr<class HdBufferArrayRange>;

/// \class HdStInstancer
///
/// HdSt implements instancing by drawing each proto multiple times with
/// a single draw call.  Application of instance primvars (like transforms)
/// is done in shaders. Instance transforms in particular are computed in
/// ApplyInstanceTransform in instancing.glslfx.
///
/// If this instancer is nested, instance indices will be computed
/// recursively by ascending the hierarchy. HdStInstancer computes a flattened
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
    HdStInstancer(HdSceneDelegate* delegate, SdfPath const &id);

    // Updates the instance primvar buffers.
    // XXX: Note, this is currently called from rprimUtils instead of the
    // render index sync phase, so it needs to take a mutex.
    HDST_API
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits) override;

    HdBufferArrayRangeSharedPtr GetInstancePrimvarRange() const {
        return _instancePrimvarRange;
    }

    /// Populates the instance index indirection buffer for \p prototypeId and
    /// returns a flat array of instance index tuples.
    HDST_API
    VtIntArray GetInstanceIndices(SdfPath const &prototypeId);

protected:
    HDST_API
    void _GetInstanceIndices(SdfPath const &prototypeId,
                             std::vector<VtIntArray> *instanceIndicesArray);

    HDST_API
    void _SyncPrimvars(HdSceneDelegate *sceneDelegate,
                       HdDirtyBits *dirtyBits);

private:
    // # of entries in an instance primvar.  This should be consistent between
    // all primvars, and also consistent with the instance indices (meaning
    // no instance index is out-of-range).
    size_t _instancePrimvarNumElements;

    // The BAR of the instance primvars for this instancer.
    // (Note: instance indices are computed per prototype and the rprim owns
    // the bar).
    HdBufferArrayRangeSharedPtr _instancePrimvarRange;

    // Visibility
    bool _visible;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_INSTANCER_H
