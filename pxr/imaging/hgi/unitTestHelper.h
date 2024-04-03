//
// Copyright 2023 Pixar
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