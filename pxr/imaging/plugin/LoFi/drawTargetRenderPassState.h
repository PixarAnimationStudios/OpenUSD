//
// Copyright 2020 benmalartre
//
// Unlicensed
// 
#ifndef PXR_IMAGING_PLUGIN_LOFI_DRAW_TARGET_RENDER_PASS_STATE_H
#define PXR_IMAGING_PLUGIN_LOFI_DRAW_TARGET_RENDER_PASS_STATE_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE


class VtValue;

/// \class LoFiDrawTargetRenderPassState
///
/// Represents common non-gl context specific render pass state for a draw
/// target.
///
/// \note This is a temporary API to aid transition to Storm, and is subject
/// to major changes.  It is likely this functionality will be absorbed into
/// the base class.
///
class LoFiDrawTargetRenderPassState final {
public:
    LOFI_API
    LoFiDrawTargetRenderPassState();
    LOFI_API
    ~LoFiDrawTargetRenderPassState();  // final no need to be virtual

    /// Set the number of color buffer's to use.
    LOFI_API
    void SetNumColorAttachments(size_t numAttachments);

    /// Set the clear value for a color buffer that is applied at the beginning
    /// of rendering.  The expected type of clearValue is dependent on the
    /// format of the buffer specified in current draw target at execute time.
    /// (i.e. there is no order dependency between setting the draw target and
    /// color values.
    /// An unexpected formats results in an error and the buffer not being
    /// cleared.
    LOFI_API
    void SetColorClearValue(size_t attachmentIdx, const VtValue &clearValue);

    /// Set the clear value for the depth buffer.  It is expected the
    /// clear value is a normalize float.
    LOFI_API
    void SetDepthClearValue(float clearValue);

    /// Sets the priority of values in the depth buffer.
    /// i.e. should pixels closer or further from the camera win.
    LOFI_API
    void SetDepthPriority(HdDepthPriority priority);

    /// Set the path to the camera to use to draw this render path from.
    LOFI_API
    void SetCamera(const SdfPath &cameraId);

    LOFI_API
    void SetRprimCollection(HdRprimCollection const& col);

    /// Returns the number of color buffers attached to the draw target.
    size_t GetNumColorAttachments() const { return _colorClearValues.size(); }

    /// Returns the clear color for the specified buffer.  The type
    /// is dependant on the format of the buffer.
    const VtValue &GetColorClearValue(size_t attachmentIdx) const
    {
        TF_DEV_AXIOM(attachmentIdx < _colorClearValues.size());

        return _colorClearValues[attachmentIdx];
    }

    /// Returns the clear value for the z-buffer.
    float GetDepthClearValue() const { return _depthClearValue; }


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
    std::vector<VtValue> _colorClearValues;
    float                _depthClearValue;
    HdDepthPriority      _depthPriority;

    SdfPath              _cameraId;

    HdRprimCollection    _rprimCollection;
    unsigned int         _rprimCollectionVersion;

    LoFiDrawTargetRenderPassState(const LoFiDrawTargetRenderPassState &) = delete;
    LoFiDrawTargetRenderPassState &operator =(const LoFiDrawTargetRenderPassState &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_DRAW_TARGET_RENDER_PASS_STATE_H
