//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_GL_COMPUTE_CMDS_H
#define PXR_IMAGING_HGI_GL_COMPUTE_CMDS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/computeCmds.h"
#include "pxr/imaging/hgi/computePipeline.h"
#include "pxr/imaging/hgiGL/api.h"
#include "pxr/imaging/hgiGL/hgi.h"
#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE

struct HgiComputeCmdsDesc;

/// \class HgiGLComputeCmds
///
/// OpenGL implementation of HgiComputeCmds.
///
class HgiGLComputeCmds final : public HgiComputeCmds
{
public:
    HGIGL_API
    ~HgiGLComputeCmds() override;

    HGIGL_API
    void PushDebugGroup(const char* label) override;

    HGIGL_API
    void PopDebugGroup() override;

    HGIGL_API
    void BindPipeline(HgiComputePipelineHandle pipeline) override;

    HGIGL_API
    void BindResources(HgiResourceBindingsHandle resources) override;

    HGIGL_API
    void SetConstantValues(
        HgiComputePipelineHandle pipeline,
        uint32_t bindIndex,
        uint32_t byteSize,
        const void* data) override;
    
    HGIGL_API
    void Dispatch(int dimX, int dimY) override;

    HGIGL_API
    void InsertMemoryBarrier(HgiMemoryBarrier barrier) override;

    HGIGL_API
    HgiComputeDispatch GetDispatchMethod() const override;

protected:
    friend class HgiGL;

    HGIGL_API
    HgiGLComputeCmds(HgiGLDevice* device, HgiComputeCmdsDesc const& desc);

    HGIGL_API
    bool _Submit(Hgi* hgi, HgiSubmitWaitType wait) override;

private:
    HgiGLComputeCmds() = delete;
    HgiGLComputeCmds & operator=(const HgiGLComputeCmds&) = delete;
    HgiGLComputeCmds(const HgiGLComputeCmds&) = delete;

    HgiGLOpsVector _ops;
    int _pushStack;
    GfVec3i _localWorkGroupSize;

    // Cmds is used only one frame so storing multi-frame state on will not
    // survive.
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
