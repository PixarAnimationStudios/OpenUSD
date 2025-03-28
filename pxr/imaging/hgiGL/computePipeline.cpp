//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/diagnostic.h"

#include "pxr/imaging/hgiGL/computePipeline.h"
#include "pxr/imaging/hgiGL/conversions.h"
#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/resourceBindings.h"
#include "pxr/imaging/hgiGL/shaderProgram.h"
#include "pxr/imaging/hgiGL/shaderFunction.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiGLComputePipeline::HgiGLComputePipeline(
    HgiComputePipelineDesc const& desc)
    : HgiComputePipeline(desc)
{
}

HgiGLComputePipeline::~HgiGLComputePipeline() = default;

void
HgiGLComputePipeline::BindPipeline()
{
    //
    // Shader program
    //
    HgiGLShaderProgram* glProgram = 
        static_cast<HgiGLShaderProgram*>(_descriptor.shaderProgram.Get());
    if (glProgram) {
        glUseProgram(glProgram->GetProgramId());
    }

    HGIGL_POST_PENDING_GL_ERRORS();
}


PXR_NAMESPACE_CLOSE_SCOPE
