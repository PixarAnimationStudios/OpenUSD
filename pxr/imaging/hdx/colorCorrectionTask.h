//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
#include "pxr/imaging/hgi/graphicsCmds.h"
#include "pxr/imaging/hgi/graphicsPipeline.h"
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgi/shaderProgram.h"
#include "pxr/imaging/hgi/texture.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

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


/// \class HdxColorCorrectionTask
///
/// A task for performing color correction (and optionally color grading) on a
/// color buffer to transform its color for display.
///
class HdxColorCorrectionTask : public HdxTask
{
public:
    using TaskParams = HdxColorCorrectionTaskParams;

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

    // Description of a texture resource and sampler
    struct _TextureSamplerDesc {
        HgiTextureDesc     textureDesc;
        HgiSamplerDesc     samplerDesc;
        std::vector<float> samples;
    };

    // Description of a buffer resource
    struct _UniformBufferDesc {
        std::string          typeName;
        std::string          name;
        std::vector<uint8_t> data;
        uint32_t             dataSize;
        uint32_t             count;
    };
    friend struct HdxColorCorrectionTask_UboBuilder;

    // Description of resources required by OCIO GPU implementation
    struct _OCIOResources {
        std::vector<_TextureSamplerDesc> luts;
        std::vector<_UniformBufferDesc>  ubos;
        std::vector<unsigned char> constantValues;
        std::string gpuShaderText;
    };

    // Utility to query OCIO for required resources
    static void
    _CreateOpenColorIOResources(
        Hgi *hgi,
        HdxColorCorrectionTaskParams const& params,
        _OCIOResources *result);
    static void
    _CreateOpenColorIOResourcesImpl(
        Hgi *hgi,
        HdxColorCorrectionTaskParams const& params,
        _OCIOResources *result);

    // Utility to check if OCIO should be used
    bool _GetUseOcio() const;

    // Utility function to create the GL program for color correction
    bool _CreateShaderResources();

    // OCIO version-specific code for shader code generation.
    std::string _CreateOpenColorIOShaderCode(std::string &ocioGpuShaderText,
                                             HgiShaderFunctionDesc &fragDesc);

    // Utility function to create buffer resources.
    bool _CreateBufferResources();

    // Utility to create resource bindings
    bool _CreateResourceBindings(HgiTextureHandle const& aovTexture);

    // OCIO version-specific code for setting LUT bindings.
    void _CreateOpenColorIOLUTBindings(HgiResourceBindingsDesc &resourceDesc);

    // Utility to create a pipeline
    bool _CreatePipeline(HgiTextureHandle const& aovTexture);

    // Utility to create an AOV sampler
    bool _CreateAovSampler();

    // Apply color correction to the currently bound framebuffer.
    void _ApplyColorCorrection(HgiTextureHandle const& aovTexture);

    // OCIO version-specific code for setting constants.
    void _SetConstants(HgiGraphicsCmds *gfxCmds);

    // Destroy shader program and the shader functions it holds.
    void _DestroyShaderProgram();

    // Print shader compile errors.
    void _PrintCompileErrors();

private: // data

    HdxColorCorrectionTaskParams _params;
    _OCIOResources _ocioResources;

    HgiAttachmentDesc _attachment0;
    HgiBufferHandle _indexBuffer;
    HgiBufferHandle _vertexBuffer;
    HgiSamplerHandle _aovSampler;

    struct TextureSamplerInfo
    {
        unsigned char    dim;
        std::string      texName;
        HgiTextureHandle texHandle;
        std::string      samplerName;
        HgiSamplerHandle samplerHandle;
    };
    std::vector<TextureSamplerInfo> _textureLUTs;

    struct BufferInfo
    {
        std::string      typeName;
        std::string      name;
        uint32_t         count;
        HgiBufferHandle  handle;
    };
    std::vector<BufferInfo>    _bufferConstants;

    HgiShaderProgramHandle _shaderProgram;
    HgiResourceBindingsHandle _resourceBindings;
    HgiGraphicsPipelineHandle _pipeline;
    float _screenSize[2];

    std::unique_ptr<class WorkDispatcher> _workDispatcher;
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
