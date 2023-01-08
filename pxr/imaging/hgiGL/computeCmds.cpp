//
// Copyright 2020 Pixar
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
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hgiGL/buffer.h"
#include "pxr/imaging/hgiGL/computeCmds.h"
#include "pxr/imaging/hgiGL/conversions.h"
#include "pxr/imaging/hgiGL/device.h"
#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/ops.h"
#include "pxr/imaging/hgiGL/graphicsPipeline.h"
#include "pxr/imaging/hgiGL/resourceBindings.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiGLComputeCmds::HgiGLComputeCmds(
    HgiGLDevice* device,
    HgiComputeCmdsDesc const&)
    : HgiComputeCmds()
    , _pushStack(0)
    , _localWorkGroupSize(GfVec3i(1, 1, 1))
{
}

HgiGLComputeCmds::~HgiGLComputeCmds() = default;

void
HgiGLComputeCmds::BindPipeline(HgiComputePipelineHandle pipeline)
{
    _ops.push_back( HgiGLOps::BindPipeline(pipeline) );

    // Get and store local work group size from shader function desc
    const HgiShaderFunctionHandleVector shaderFunctionsHandles = 
        pipeline.Get()->GetDescriptor().shaderProgram.Get()->GetDescriptor().
            shaderFunctions;

    for (const auto &handle : shaderFunctionsHandles) {
        const HgiShaderFunctionDesc &shaderDesc = handle.Get()->GetDescriptor();
        if (shaderDesc.shaderStage == HgiShaderStageCompute) {
            if (shaderDesc.computeDescriptor.localSize[0] > 0 && 
                shaderDesc.computeDescriptor.localSize[1] > 0 &&
                shaderDesc.computeDescriptor.localSize[2] > 0) {
                _localWorkGroupSize = shaderDesc.computeDescriptor.localSize;
            }
        }
    }
}

void
HgiGLComputeCmds::BindResources(HgiResourceBindingsHandle res)
{
    _ops.push_back( HgiGLOps::BindResources(res) );
}

void
HgiGLComputeCmds::SetConstantValues(
    HgiComputePipelineHandle pipeline,
    uint32_t bindIndex,
    uint32_t byteSize,
    const void* data)
{
    _ops.push_back(
        HgiGLOps::SetConstantValues(
            pipeline,
            bindIndex,
            byteSize,
            data)
        );
}

void
HgiGLComputeCmds::Dispatch(int dimX, int dimY)
{
    const int threadsPerGroupX = _localWorkGroupSize[0];
    const int threadsPerGroupY = _localWorkGroupSize[1];
    int numWorkGroupsX = (dimX + (threadsPerGroupX - 1)) / threadsPerGroupX;
    int numWorkGroupsY = (dimY + (threadsPerGroupY - 1)) / threadsPerGroupY;

    // Determine device's num compute work group limits
    int maxNumWorkGroups[2] = { 0, 0 };
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxNumWorkGroups[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxNumWorkGroups[1]);

    if (numWorkGroupsX > maxNumWorkGroups[0]) {
        TF_WARN("Max number of work group available from device is %i, larger "
                "than %i", maxNumWorkGroups[0], numWorkGroupsX);
        numWorkGroupsX = maxNumWorkGroups[0];
    }
    if (numWorkGroupsY > maxNumWorkGroups[1]) {
        TF_WARN("Max number of work group available from device is %i, larger "
                "than %i", maxNumWorkGroups[1], numWorkGroupsY);
        numWorkGroupsY = maxNumWorkGroups[1];
    }

    _ops.push_back(
        HgiGLOps::Dispatch(numWorkGroupsX, numWorkGroupsY)
        );
}

void
HgiGLComputeCmds::PushDebugGroup(const char* label)
{
    if (HgiGLDebugEnabled()) {
        _pushStack++;
        _ops.push_back( HgiGLOps::PushDebugGroup(label) );
    }
}

void
HgiGLComputeCmds::PopDebugGroup()
{
    if (HgiGLDebugEnabled()) {
        _pushStack--;
        _ops.push_back( HgiGLOps::PopDebugGroup() );
    }
}

void
HgiGLComputeCmds::InsertMemoryBarrier(HgiMemoryBarrier barrier)
{
    _ops.push_back( HgiGLOps::InsertMemoryBarrier(barrier) );
}

HgiComputeDispatch
HgiGLComputeCmds::GetDispatchMethod() const
{
    return HgiComputeDispatchSerial;
}

bool
HgiGLComputeCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
{
    if (_ops.empty()) {
        return false;
    }

    TF_VERIFY(_pushStack==0, "Push and PopDebugGroup do not even out");

    HgiGL* hgiGL = static_cast<HgiGL*>(hgi);
    HgiGLDevice* device = hgiGL->GetPrimaryDevice();
    device->SubmitOps(_ops);
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
