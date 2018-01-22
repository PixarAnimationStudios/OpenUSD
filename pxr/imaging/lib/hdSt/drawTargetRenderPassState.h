//
// Copyright 2017 Pixar
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
#ifndef HDST_DRAW_TARGET_RENDER_PASS_STATE_H
#define HDST_DRAW_TARGET_RENDER_PASS_STATE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE


class VtValue;

/// \class HdStDrawTargetRenderPassState
///
/// Represents common non-gl context specific render pass state for a draw
/// target.
///
/// \note This is a temporary API to aid transition to Hydra, and is subject
/// to major changes.  It is likely this functionality will be absorbed into
/// the base class.
///
class HdStDrawTargetRenderPassState final {
public:
    HDST_API
    HdStDrawTargetRenderPassState();
    HDST_API
    ~HdStDrawTargetRenderPassState();  // final no need to be virtual

    /// Set the number of color buffer's to use.
    HDST_API
    void SetNumColorAttachments(size_t numAttachments);

    /// Set the clear value for a color buffer that is applied at the beginning
    /// of rendering.  The expected type of clearValue is dependent on the
    /// format of the buffer specified in current draw target at execute time.
    /// (i.e. there is no order dependency between setting the draw target and
    /// color values.
    /// An unexpected formats results in an error and the buffer not being
    /// cleared.
    HDST_API
    void SetColorClearValue(size_t attachmentIdx, const VtValue &clearValue);

    /// Set the clear value for the depth buffer.  It is expected the
    /// clear value is a normalize float.
    HDST_API
    void SetDepthClearValue(float clearValue);

    /// Set the path to the camera to use to draw this render path from.
    HDST_API
    void SetCamera(const SdfPath &cameraId);

    HDST_API
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
    SdfPath              _cameraId;

    HdRprimCollection    _rprimCollection;
    unsigned int         _rprimCollectionVersion;

    HdStDrawTargetRenderPassState(const HdStDrawTargetRenderPassState &) = delete;
    HdStDrawTargetRenderPassState &operator =(const HdStDrawTargetRenderPassState &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDST_DRAW_TARGET_RENDER_PASS_STATE_H
