//
// Copyright 2019 Pixar
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

#include "pxr/usdImaging/usdAppUtils/frameRecorder.h"

#include "pxr/base/gf/camera.h"
#include "pxr/base/tf/scoped.h"

#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/simpleMaterial.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hdSt/hioConversions.h"
#include "pxr/imaging/hdSt/textureUtils.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/types.h"
#include "pxr/imaging/hio/image.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdRender/settings.h"
#include "pxr/usd/usdRender/product.h"

#include "pxr/base/arch/fileSystem.h"

#include <string>


PXR_NAMESPACE_OPEN_SCOPE

UsdAppUtilsFrameRecorder::UsdAppUtilsFrameRecorder(
    const TfToken& rendererPluginId,
    bool gpuEnabled) :
    _imagingEngine(HdDriver(), rendererPluginId, gpuEnabled),
    _imageWidth(960u),
    _complexity(1.0f),
    _colorCorrectionMode(HdxColorCorrectionTokens->disabled),
    _purposes({UsdGeomTokens->default_, UsdGeomTokens->proxy}),
    _cameraLightEnabled(true)
{
    // Disable presentation to avoid the need to create an OpenGL context when
    // using other graphics APIs such as Metal and Vulkan.
    _imagingEngine.SetEnablePresentation(false);

    // Set the interactive to be false on the HdRenderSettingsMap 
    _imagingEngine.SetRendererSetting(
        HdRenderSettingsTokens->enableInteractive, VtValue(false));
}

void
UsdAppUtilsFrameRecorder::SetActiveRenderSettingsPrimPath(SdfPath const& path)
{
    _renderSettingsPrimPath = path;
    if (!_renderSettingsPrimPath.IsEmpty()) {
        _imagingEngine.SetActiveRenderSettingsPrimPath(_renderSettingsPrimPath);
    }
}

void 
UsdAppUtilsFrameRecorder::SetActiveRenderPassPrimPath(SdfPath const& path)
{
    _renderPassPrimPath = path;
    if (!_renderPassPrimPath.IsEmpty()) {
        _imagingEngine.SetActiveRenderPassPrimPath(_renderPassPrimPath);
    }
}

static bool
_HasPurpose(const TfTokenVector& purposes, const TfToken& purpose)
{
    return std::find(purposes.begin(), purposes.end(), purpose) != purposes.end();
}

void
UsdAppUtilsFrameRecorder::SetColorCorrectionMode(
    const TfToken& colorCorrectionMode)
{
    if (_imagingEngine.GetGPUEnabled()) {
        _colorCorrectionMode = colorCorrectionMode;
    } else {
        if (colorCorrectionMode != HdxColorCorrectionTokens->disabled) {
            TF_WARN("Color correction presently unsupported when the GPU is "
                    "disabled.");
        }
        _colorCorrectionMode = HdxColorCorrectionTokens->disabled;
    }
}

void
UsdAppUtilsFrameRecorder::SetCameraLightEnabled(bool cameraLightEnabled)
{
    _cameraLightEnabled = cameraLightEnabled;
}

void
UsdAppUtilsFrameRecorder::SetIncludedPurposes(const TfTokenVector& purposes) 
{
    TfTokenVector  allPurposes = { UsdGeomTokens->render,
                                   UsdGeomTokens->proxy,
                                   UsdGeomTokens->guide };
    _purposes = { UsdGeomTokens->default_ };

    for (TfToken const &p : purposes) {
        if (_HasPurpose(allPurposes, p)) {
            _purposes.push_back(p);
        }
        else if (p != UsdGeomTokens->default_) {
            // We allow "default" to be specified even though
            // it's unnecessary
            TF_CODING_ERROR("Unrecognized purpose value '%s'.",
                            p.GetText());
        }
    }
}

static GfCamera
_ComputeCameraToFrameStage(const UsdStagePtr& stage, UsdTimeCode timeCode,
                           const TfTokenVector& includedPurposes)
{
    // Start with a default (50mm) perspective GfCamera.
    GfCamera gfCamera;
    UsdGeomBBoxCache bboxCache(timeCode, includedPurposes,
                               /* useExtentsHint = */ true);
    GfBBox3d bbox = bboxCache.ComputeWorldBound(stage->GetPseudoRoot());
    GfVec3d center = bbox.ComputeCentroid();
    GfRange3d range = bbox.ComputeAlignedRange();
    GfVec3d dim = range.GetSize();
    TfToken upAxis = UsdGeomGetStageUpAxis(stage);

    // Find corner of bbox in the focal plane.
    GfVec2d plane_corner;
    if (upAxis == UsdGeomTokens->y) {
        plane_corner = GfVec2d(dim[0], dim[1])/2;
    } else {
        plane_corner = GfVec2d(dim[0], dim[2])/2;
    }
    float plane_radius = sqrt(GfDot(plane_corner, plane_corner));

    // Compute distance to focal plane.
    float half_fov = gfCamera.GetFieldOfView(GfCamera::FOVHorizontal)/2.0;
    float distance = plane_radius / tan(GfDegreesToRadians(half_fov));

    // Back up to frame the front face of the bbox.
    if (upAxis == UsdGeomTokens->y) {
        distance += dim[2] / 2;
    } else {
        distance += dim[1] / 2;
    }
    // Small objects that fill out their bounding boxes might be clipped by the 
    // near-clipping plane (always defaulting to 1 here). Increase the distance
    // to make sure that doesn't happen.
    if (distance < gfCamera.GetClippingRange().GetMin()) {
        distance += gfCamera.GetClippingRange().GetMin();
    }

    // Compute local-to-world transform for camera filmback.
    GfMatrix4d xf;
    if (upAxis == UsdGeomTokens->y) {
        xf.SetTranslate(center + GfVec3d(0, 0, distance));
    } else {
        xf.SetRotate(GfRotation(GfVec3d(1,0,0), 90));
        xf.SetTranslateOnly(center + GfVec3d(0, -distance, 0));
    }
    gfCamera.SetTransform(xf);
    return gfCamera;
}

namespace
{

class TextureBufferWriter
{
public:
    TextureBufferWriter(
        UsdImagingGLEngine* engine)
        : _engine(engine)
        , _colorRenderBuffer(nullptr)
    {
        // If rendering via Storm or a non-Storm renderer but with the GPU
        // enabled, we will have a color texture to read from.
        //
        // If using a non-Storm renderer with the GPU disabled, we need to
        // read from the color render buffer.

        if (_engine->GetGPUEnabled()) {
            _colorTextureHandle = engine->GetAovTexture(HdAovTokens->color);
            if (!_colorTextureHandle) {
                TF_CODING_ERROR("No color texture to write out.");
            }
        } else {
            _colorRenderBuffer = engine->GetAovRenderBuffer(HdAovTokens->color);
            if (_colorRenderBuffer) {
                _colorRenderBuffer->Resolve();
            } else {
                TF_CODING_ERROR("No color buffer to write out.");
            }
        }
    }

    bool Write(const std::string& filename)
    {
        if (!_ValidSource()) {
            return false;
        }

        HioImage::StorageSpec storage;
        storage.width = _GetWidth();
        storage.height = _GetHeight();
        storage.format = _GetFormat();
        storage.flipped = true;
        storage.data = _Map();
        TfScoped<> scopedUnmap([this](){ _Unmap(); });

        {
            TRACE_FUNCTION_SCOPE("writing image");

            const HioImageSharedPtr image = HioImage::OpenForWriting(filename);
            const bool writeSuccess = image && image->Write(storage);
            
            if (!writeSuccess) {
                TF_RUNTIME_ERROR("Failed to write image to %s",
                    filename.c_str());
                return false;
            }
        }

        return true;
    }

private:
    bool _ValidSource() const
    {
        return _colorTextureHandle || _colorRenderBuffer;
    }

    unsigned int _GetWidth() const
    {
        if (_colorTextureHandle) {
            return _colorTextureHandle->GetDescriptor().dimensions[0];
        } else if (_colorRenderBuffer) {
            return _colorRenderBuffer->GetWidth();
        } else {
            return 0;
        }
    }

    unsigned int _GetHeight() const
    {
        if (_colorTextureHandle) {
            return _colorTextureHandle->GetDescriptor().dimensions[1];
        } else if (_colorRenderBuffer) {
            return _colorRenderBuffer->GetHeight();
        } else {
            return 0;
        }
    }

    HioFormat _GetFormat() const
    {
        if (_colorTextureHandle) {
            return HdxGetHioFormat(_colorTextureHandle->GetDescriptor().format);
        } else if (_colorRenderBuffer) {
            return HdStHioConversions::GetHioFormat(
                _colorRenderBuffer->GetFormat());
        } else {
            return HioFormatInvalid;
        }
    }

    void* _Map()
    {
        if (_colorTextureHandle) {
            size_t size = 0;
            _mappedColorTextureBuffer = HdStTextureUtils::HgiTextureReadback(
                _engine->GetHgi(), _colorTextureHandle, &size);
            return _mappedColorTextureBuffer.get();
        } else if (_colorRenderBuffer) {
            return _colorRenderBuffer->Map();
        } else {
            return nullptr;
        }
    }

    void _Unmap()
    {
        if (_colorRenderBuffer) {
            _colorRenderBuffer->Unmap();
        }
    }

    UsdImagingGLEngine* _engine;
    HgiTextureHandle _colorTextureHandle;
    HdRenderBuffer* _colorRenderBuffer;
    HdStTextureUtils::AlignedBuffer<uint8_t> _mappedColorTextureBuffer;
};

}

static bool
_RenderProductsGenerated(
    const UsdStagePtr& stage,
    const SdfPath& renderSettingsPrimPath)
{
    if (renderSettingsPrimPath.IsEmpty()) {
        return false;
    }

    bool productsGenerated = false;
    UsdRenderSettings settings = 
        UsdRenderSettings(stage->GetPrimAtPath(renderSettingsPrimPath));
    
    // Each Render Product should generate an image
    SdfPathVector renderProductTargets;
    settings.GetProductsRel().GetForwardedTargets(&renderProductTargets);

    for (const auto& productPath : renderProductTargets) {
        UsdRenderProduct product =
            UsdRenderProduct(stage->GetPrimAtPath(productPath));
        TfToken productName;
        product.GetProductNameAttr().Get(&productName);
        if (ArchOpenFile(productName.GetText(), "r")) {
            TF_STATUS("Product '%s' generated from RenderProduct prim <%s> "
                      "on RenderSettings <%s>", productName.GetText(), 
                      productPath.GetText(), renderSettingsPrimPath.GetText());
            productsGenerated |= true;
        } else {
            TF_WARN("Missing generated Product '%s' from RenderProduct prim "
                    "<%s> on RenderSettings <%s>", productName.GetText(),
                    productPath.GetText(), renderSettingsPrimPath.GetText());
        }
    }

    if (renderProductTargets.empty()) {
        TF_WARN("No Render Prouducts found on the RenderSettings prim <%s>\n",
                renderSettingsPrimPath.GetText());
    }

    return productsGenerated;
}

bool
UsdAppUtilsFrameRecorder::Record(
    const UsdStagePtr& stage,
    const UsdGeomCamera& usdCamera,
    const UsdTimeCode timeCode,
    const std::string& outputImagePath)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return false;
    }

    if (outputImagePath.empty()) {
        TF_CODING_ERROR("Invalid empty output image path");
        return false;
    }

    const GfVec4f CLEAR_COLOR(0.0f);
    const GfVec4f SCENE_AMBIENT(0.01f, 0.01f, 0.01f, 1.0f);
    const GfVec4f SPECULAR_DEFAULT(0.1f, 0.1f, 0.1f, 1.0f);
    const GfVec4f AMBIENT_DEFAULT(0.2f, 0.2f, 0.2f, 1.0f);
    const float   SHININESS_DEFAULT(32.0);
    
    GfCamera gfCamera;
    if (usdCamera) {
        gfCamera = usdCamera.GetCamera(timeCode);
    } else {
        gfCamera = _ComputeCameraToFrameStage(stage, timeCode, _purposes);
    }

    // Calculate the imageHeight based on the aspect ratio
    // XXX: If the camera's aspect ratio is animated, then a range of calls to
    // this function may generate a sequence of images with different sizes.
    float aspectRatio = gfCamera.GetAspectRatio();
    if (GfIsClose(aspectRatio, 0.0f, 1e-4)) {
        aspectRatio = 1.0f;
    }
    const size_t imageHeight = std::max<size_t>(
        static_cast<size_t>(static_cast<float>(_imageWidth) / aspectRatio),
        1u);

    _imagingEngine.SetRendererAov(HdAovTokens->color);

    const GfFrustum frustum = gfCamera.GetFrustum();
    if (usdCamera) {
        _imagingEngine.SetCameraPath(usdCamera.GetPath());
    }
    else {
        _imagingEngine.SetCameraState(
            frustum.ComputeViewMatrix(),
            frustum.ComputeProjectionMatrix());
    }
    const GfRect2i dataWindow(GfVec2i(0.0), _imageWidth, imageHeight); 
    _imagingEngine.SetFraming(CameraUtilFraming(dataWindow));
    _imagingEngine.SetRenderBufferSize(GfVec2i(_imageWidth, imageHeight));

    GlfSimpleLightVector lights;
    if (_cameraLightEnabled) {
        const GfVec3d &cameraPos = frustum.GetPosition();
        GlfSimpleLight cameraLight(
            GfVec4f(cameraPos[0], cameraPos[1], cameraPos[2], 1.0f));
        cameraLight.SetTransform(frustum.ComputeViewInverse());
        cameraLight.SetAmbient(SCENE_AMBIENT);
        lights.push_back(cameraLight);
    }

    // Make default material and lighting match usdview's
    // defaults... we expect GlfSimpleMaterial to go away soon, so
    // not worth refactoring for sharing
    GlfSimpleMaterial material;
    material.SetAmbient(AMBIENT_DEFAULT);
    material.SetSpecular(SPECULAR_DEFAULT);
    material.SetShininess(SHININESS_DEFAULT);

    _imagingEngine.SetLightingState(lights, material, SCENE_AMBIENT);

    UsdImagingGLRenderParams renderParams;
    renderParams.frame = timeCode;
    renderParams.complexity = _complexity;
    renderParams.colorCorrectionMode = _colorCorrectionMode;
    renderParams.clearColor = CLEAR_COLOR;
    renderParams.showProxy = _HasPurpose(_purposes, UsdGeomTokens->proxy);
    renderParams.showRender = _HasPurpose(_purposes, UsdGeomTokens->render);
    renderParams.showGuides = _HasPurpose(_purposes, UsdGeomTokens->guide);

    const UsdPrim& pseudoRoot = stage->GetPseudoRoot();

    unsigned int sleepTime = 10; // Initial wait time of 10 ms

    while (true) {
        _imagingEngine.Render(pseudoRoot, renderParams);

        if (_imagingEngine.IsConverged()) {
            break;
        } else {
            // Allow render thread to progress before invoking Render again.
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
            // Increase the sleep time up to a max of 100 ms
            sleepTime = std::min(100u, sleepTime + 5);
        }
    };

    // If the RenderProducts on RenderSettings Prim successfully generated
    // images, we do not need to write to the outputImagePath.
    if (_RenderProductsGenerated(stage, _renderSettingsPrimPath)) {
        return true;
    }
    TextureBufferWriter writer(&_imagingEngine);
    return writer.Write(outputImagePath);
}


PXR_NAMESPACE_CLOSE_SCOPE
