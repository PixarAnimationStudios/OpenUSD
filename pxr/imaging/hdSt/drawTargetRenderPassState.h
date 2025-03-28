//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_DRAW_TARGET_RENDER_PASS_STATE_H
#define PXR_IMAGING_HD_ST_DRAW_TARGET_RENDER_PASS_STATE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE


class VtValue;
using HdRenderPassAovBindingVector =
    std::vector<struct HdRenderPassAovBinding>;

/// \class HdStDrawTargetRenderPassState
///
/// Represents common non-gl context specific render pass state for a draw
/// target.
///
/// \note This is a temporary API to aid transition to Storm, and is subject
/// to major changes.  It is likely this functionality will be absorbed into
/// the base class.
///
class HdStDrawTargetRenderPassState final
{
public:
    HDST_API
    HdStDrawTargetRenderPassState();
    HDST_API
    ~HdStDrawTargetRenderPassState();  // final no need to be virtual

    const HdRenderPassAovBindingVector &GetAovBindings() const {
        return _aovBindings;
    }

    HDST_API
    void SetAovBindings(const HdRenderPassAovBindingVector &aovBindings);

    /// Sets the priority of values in the depth buffer.
    /// i.e. should pixels closer or further from the camera win.
    HDST_API
    void SetDepthPriority(HdDepthPriority priority);

    /// Set the path to the camera to use to draw this render path from.
    HDST_API
    void SetCamera(const SdfPath &cameraId);

    HDST_API
    void SetRprimCollection(HdRprimCollection const& col);

    HdDepthPriority GetDepthPriority() const { return _depthPriority; }


    /// Returns the path to the camera to render from.
    const SdfPath &GetCamera() const { return _cameraId; }

    /// Returns an increasing version number for when the collection object
    /// is changed.
    /// Note: This tracks the actual object and not the contents of the
    /// collection.
    unsigned int       GetRprimCollectionVersion() const
    {
        return _rprimCollectionVersion;
    }

    /// Returns the collection associated with this draw target.
    const HdRprimCollection &GetRprimCollection() const
    {
        return _rprimCollection;
    }

private:
    HdRenderPassAovBindingVector _aovBindings;
    HdDepthPriority      _depthPriority;

    SdfPath              _cameraId;

    HdRprimCollection    _rprimCollection;
    unsigned int         _rprimCollectionVersion;

    HdStDrawTargetRenderPassState(const HdStDrawTargetRenderPassState &) = delete;
    HdStDrawTargetRenderPassState &operator =(const HdStDrawTargetRenderPassState &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_DRAW_TARGET_RENDER_PASS_STATE_H
