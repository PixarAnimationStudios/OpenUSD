//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_UNIT_TEST_HELPER_H
#define PXR_IMAGING_HGI_UNIT_TEST_HELPER_H

#include "pxr/pxr.h"

#include "pxr/imaging/hgi/hgi.h"

PXR_NAMESPACE_OPEN_SCOPE

using HgiUniquePtr = std::unique_ptr<class Hgi>;

class HgiInitializationTestDriver
{
public:
    HGI_API
    HgiInitializationTestDriver();
    
    HGI_API
    ~HgiInitializationTestDriver();

    HGI_API
    Hgi* GetHgi() { return _hgi.get(); }

private:
    HgiUniquePtr _hgi;
};

class HgiPipelineCreationTestDriver
{
public:
    HGI_API
    HgiPipelineCreationTestDriver();
    
    HGI_API
    ~HgiPipelineCreationTestDriver();

    HGI_API
    bool CreateTestPipeline();

    HGI_API
    Hgi* GetHgi() { return _hgi.get(); }

protected:
    bool _CreateShaderProgram();
    void _DestroyShaderProgram();
    void _CreateVertexBufferDescriptor();
    bool _CreatePipeline();
    void _PrintCompileErrors();

    HgiUniquePtr _hgi;
    HgiShaderProgramHandle _shaderProgram;
    HgiGraphicsPipelineHandle _pipeline;
    HgiVertexBufferDesc _vboDesc;
    HgiAttachmentDesc _colorAtt;
    HgiAttachmentDesc _depthAtt;
};

class HgiGfxCmdBfrExecutionTestDriver : public HgiPipelineCreationTestDriver
{
public:
    HGI_API
    HgiGfxCmdBfrExecutionTestDriver();
    
    HGI_API
    ~HgiGfxCmdBfrExecutionTestDriver();

    HGI_API
    bool ExecuteTestGfxCmdBfr();
    
    HGI_API
    bool WriteToFile(const std::string& filePath);

private:
    GfVec3i _renderDim;

    bool _CreateResourceBuffers();
    bool _CreateRenderTargets();

    HgiBufferHandle _indexBuffer;
    HgiBufferHandle _vertexBuffer;

    HgiTextureHandle _colorTarget;
    HgiTextureViewHandle _colorTargetView;

    HgiTextureHandle _depthTarget;
    HgiTextureViewHandle _depthTargetView;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HGI_UNIT_TEST_HELPER_H