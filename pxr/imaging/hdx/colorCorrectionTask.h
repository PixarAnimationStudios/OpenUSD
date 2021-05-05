//
// Copyright 2018 Pixar
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
#ifndef HDX_COLORCORRECTION_TASK_H
#define HDX_COLORCORRECTION_TASK_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/task.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hgi/attachmentDesc.h"
#include "pxr/imaging/hgi/buffer.h"
#include "pxr/imaging/hgi/graphicsPipeline.h"
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgi/shaderProgram.h"
#include "pxr/imaging/hgi/texture.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE


/// \class HdxColorCorrectionTask
///
/// A task for performing color correction (and optionally color grading) on a 
/// color buffer to transform its color for display.
///
class HdxColorCorrectionTask : public HdxTask
{
public:
    HDX_API
    HdxColorCorrectionTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    ~HdxColorCorrectionTask() override;

    /// Prepare the tasks resources
    HDX_API
    void Prepare(HdTaskContext* ctx,
                 HdRenderIndex* renderIndex) override;

    /// Execute the color correction task
    HDX_API
    void Execute(HdTaskContext* ctx) override;

protected:
    /// Sync the render pass resources
    HDX_API
    void _Sync(HdSceneDelegate* delegate,
               HdTaskContext* ctx,
               HdDirtyBits* dirtyBits) override;

private:
    HdxColorCorrectionTask() = delete;
    HdxColorCorrectionTask(const HdxColorCorrectionTask &) = delete;
    HdxColorCorrectionTask &operator =(const HdxColorCorrectionTask &) = delete;

    // Utility to create OCIO resources and returns the OCIO shadercode
    std::string _CreateOpenColorIOResources();

    // Utility to check if OCIO should be used
    bool _GetUseOcio() const;

    // Utility function to create the GL program for color correction
    bool _CreateShaderResources();

    // Utility function to create buffer resources.
    bool _CreateBufferResources();

    // Utility to create resource bindings
    bool _CreateResourceBindings(HgiTextureHandle const& aovTexture);

    // Utility to create a pipeline
    bool _CreatePipeline(HgiTextureHandle const& aovTexture);

    // Utility to create a texture sampler
    bool _CreateSampler();

    // Apply color correction to the currently bound framebuffer.
    void _ApplyColorCorrection(HgiTextureHandle const& aovTexture);

    // Destroy shader program and the shader functions it holds.
    void _DestroyShaderProgram();

    // Print shader compile errors.
    void _PrintCompileErrors();

    HgiAttachmentDesc _attachment0;
    HgiBufferHandle _indexBuffer;
    HgiBufferHandle _vertexBuffer;
    HgiTextureHandle _texture3dLUT;
    HgiSamplerHandle _sampler;
    HgiShaderProgramHandle _shaderProgram;
    HgiResourceBindingsHandle _resourceBindings;
    HgiGraphicsPipelineHandle _pipeline;

    TfToken _colorCorrectionMode;
    std::string _displayOCIO;
    std::string _viewOCIO;
    std::string _colorspaceOCIO;
    std::string _looksOCIO;
    int _lut3dSizeOCIO;

    float _screenSize[2];

    TfToken _aovName;
};


/// \class HdxColorCorrectionTaskParams
///
/// ColorCorrectionTask parameters.
///
/// `lut3dSizeOCIO`: We default to 65 (0-64) samples which works well with
//                   OCIO resampling.
///
struct HdxColorCorrectionTaskParams
{
    HDX_API
    HdxColorCorrectionTaskParams();
    
    // Switch between HdColorCorrectionTokens.
    // We default to 'disabled' to be backwards compatible with clients that are
    // still running with sRGB buffers.
    TfToken colorCorrectionMode;

    // 'display', 'view', 'colorspace' and 'look' are options the client may 
    // supply to configure OCIO. If one is not provided the default values
    // is substituted. You can find the values for these strings inside the
    // profile/config .ocio file. For example:
    //
    //  displays:
    //    rec709g22:
    //      !<View> {name: studio, colorspace: linear, looks: studio_65_lg2}
    //
    std::string displayOCIO;
    std::string viewOCIO;
    std::string colorspaceOCIO;
    std::string looksOCIO;

    // The width, height and depth used for the GPU LUT 3d texture.
    int lut3dSizeOCIO;

    // The name of the aov to color correct
    TfToken aovName;
};

// VtValue requirements
HDX_API
std::ostream& operator<<(std::ostream& out, const HdxColorCorrectionTaskParams& pv);
HDX_API
bool operator==(const HdxColorCorrectionTaskParams& lhs,
                const HdxColorCorrectionTaskParams& rhs);
HDX_API
bool operator!=(const HdxColorCorrectionTaskParams& lhs,
                const HdxColorCorrectionTaskParams& rhs);


PXR_NAMESPACE_CLOSE_SCOPE

#endif
