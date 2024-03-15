//
// Copyright (c) 2022-2024, NVIDIA CORPORATION.
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

#ifndef HD_USD_WRITER_CAMERA_H
#define HD_USD_WRITER_CAMERA_H

#include "pxr/usdImaging/plugin/hdUsdWriter/api.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/utils.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/js/json.h"
#include "pxr/imaging/cameraUtil/conformWindow.h"
#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hf/perfLog.h"
#include "pxr/pxr.h"
#include "pxr/usd/usd/prim.h"

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdUsdWriterCamera
///
class HdUsdWriterCamera : public HdSprim
{
public:
    HF_MALLOC_TAG_NEW("new HdUsdWriterCamera");

    /// HdUsdWriterCamera constructor.
    ///   \param id The scene-graph path to this camera.
    HDUSDWRITER_API
    explicit HdUsdWriterCamera(SdfPath const& id);

    /// HdUsdWriterCamera destructor.
    HDUSDWRITER_API
    ~HdUsdWriterCamera() override = default;

    /// Inform the scene graph which state needs to be downloaded in the
    /// first Sync() call: in this case, topology and Light data to build
    /// the geometry object in the scene graph.
    ///   \return The initial dirty state this camera wants to query.
    HDUSDWRITER_API
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    /// Pull invalidated scene data and prepare/update the renderable
    /// representation.
    ///   \param sceneDelegate The data source for this geometry item.
    ///   \param renderParam State.
    ///   \param dirtyBits A specifier for which scene data has changed.
    HDUSDWRITER_API
    void Sync(HdSceneDelegate* sceneDelegate, HdRenderParam* renderParam, HdDirtyBits* dirtyBits) override;

    /// Serialize the primitive to USD.
    ///
    ///   \param stage Reference to HdUsdWriter Stage Proxy.
    HDUSDWRITER_API
    virtual void SerializeToUsd(const UsdStagePtr &stage);

protected:
    // This class does not support copying.
    HdUsdWriterCamera(const HdUsdWriterCamera&) = delete;
    HdUsdWriterCamera& operator=(const HdUsdWriterCamera&) = delete;

    // Camera Params
    HdUsdWriterOptional<float> _focalLength;
    HdUsdWriterOptional<float> _focusDistance;
    HdUsdWriterOptional<float> _fStop;
    HdUsdWriterOptional<float> _horizontalAperture;
    HdUsdWriterOptional<float> _horizontalApertureOffset;
    HdUsdWriterOptional<float> _verticalAperture;
    HdUsdWriterOptional<float> _verticalApertureOffset;
    HdUsdWriterOptional<float> _exposure;
    HdUsdWriterOptional<GfRange1f> _clippingRange;
    HdUsdWriterOptional<double> _shutterOpen;
    HdUsdWriterOptional<double> _shutterClose;
    HdUsdWriterOptional<TfToken> _stereoRole;
    HdUsdWriterOptional<HdCamera::Projection> _projection;
    HdUsdWriterOptional<bool> _focusOn;
    HdUsdWriterOptional<float> _dofAspect;
    HdUsdWriterOptional<int> _splitDiopterCount;
    HdUsdWriterOptional<float> _splitDiopterAngle;
    HdUsdWriterOptional<float> _splitDiopterOffset1;
    HdUsdWriterOptional<float> _splitDiopterWidth1;
    HdUsdWriterOptional<float> _splitDiopterFocusDistance1;
    HdUsdWriterOptional<float> _splitDiopterOffset2;
    HdUsdWriterOptional<float> _splitDiopterWidth2;
    HdUsdWriterOptional<float> _splitDiopterFocusDistance2;
    HdUsdWriterOptional<TfToken> _lensDistortionType;
    HdUsdWriterOptional<float> _lensDistortionK1;
    HdUsdWriterOptional<float> _lensDistortionK2;
    HdUsdWriterOptional<float> _lensDistortionCenter;
    HdUsdWriterOptional<float> _lensDistortionAnaSq;
    HdUsdWriterOptional<float> _lensDistortionAsym;
    HdUsdWriterOptional<float> _lensDistortionScale;
    HdUsdWriterOptional<float> _lensDistortionIor;



    HdUsdWriterOptional<CameraUtilConformWindowPolicy> _windowPolicy;
    HdUsdWriterOptional<std::vector<GfVec4d>> _clipPlanes;

    HdUsdWriterOptional<GfMatrix4d> _transform;
    HdUsdWriterOptional<bool> _visible;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif