//
// Copyright 2021 Pixar
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
#ifndef PXR_IMAGING_HD_ST_PIPELINE_DRAW_BATCH_H
#define PXR_IMAGING_HD_ST_PIPELINE_DRAW_BATCH_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/dispatchBuffer.h"
#include "pxr/imaging/hdSt/drawBatch.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HgiCapabilities;
struct HgiIndirectCommands;
using HdStBindingRequestVector = std::vector<class HdStBindingRequest>;

/// \class HdSt_PipelineDrawBatch
///
/// Drawing batch that is executed using an Hgi graphics pipeline.
///
/// A valid draw batch contains draw items that have the same
/// primitive type and that share aggregated drawing resources,
/// e.g. uniform and non uniform primvar buffers.
///
class HdSt_PipelineDrawBatch : public HdSt_DrawBatch
{
public:
    HDST_API
    HdSt_PipelineDrawBatch(HdStDrawItemInstance * drawItemInstance,
                           bool const allowGpuFrustumCulling = true,
                           bool const allowIndirectCommandEncoding = true);
    HDST_API
    ~HdSt_PipelineDrawBatch() override;

    // HdSt_DrawBatch overrides
    HDST_API
    ValidationResult
    Validate(bool deepValidation) override;

    /// Prepare draw commands and apply view frustum culling for this batch.
    HDST_API
    void PrepareDraw(
        HgiGraphicsCmds *gfxCmds,
        HdStRenderPassStateSharedPtr const & renderPassState,
        HdStResourceRegistrySharedPtr const & resourceRegistry) override;

    /// Encode drawing commands for this batch.
    HDST_API
    void EncodeDraw(
        HdStRenderPassStateSharedPtr const & renderPassState,
        HdStResourceRegistrySharedPtr const & resourceRegistry,
        bool firstDrawBatch) override;

    /// Executes the drawing commands for this batch.
    HDST_API
    void ExecuteDraw(
        HgiGraphicsCmds *gfxCmds,
        HdStRenderPassStateSharedPtr const & renderPassState,
        HdStResourceRegistrySharedPtr const & resourceRegistry,
        bool firstDrawBatch) override;

    HDST_API
    void DrawItemInstanceChanged(
        HdStDrawItemInstance const * instance) override;

    HDST_API
    void SetEnableTinyPrimCulling(bool tinyPrimCulling) override;

    /// Returns whether pipeline draw batching is enabled.
    HDST_API
    static bool IsEnabled(HgiCapabilities const *hgiCapabilities);

    /// Returns whether to do frustum culling on the GPU
    HDST_API
    static bool IsEnabledGPUFrustumCulling();

    /// Returns whether to read back the count of visible items from the GPU
    /// Disabled by default, since there is some performance penalty.
    HDST_API
    static bool IsEnabledGPUCountVisibleInstances();

    /// Returns whether to do per-instance culling on the GPU
    HDST_API
    static bool IsEnabledGPUInstanceFrustumCulling();

protected:
    HDST_API
    void _Init(HdStDrawItemInstance * drawItemInstance) override;

private:
    // Culling requires custom resource binding.
    class _CullingProgram : public _DrawingProgram
    {
    public:
        _CullingProgram()
            : _useDrawIndexed(true)
            , _useInstanceCulling(false)
            , _bufferArrayHash(0) { }
        void Initialize(bool useDrawIndexed,
                        bool useInstanceCulling,
                        size_t bufferArrayHash);
    protected:
        // _DrawingProgram overrides
        void _GetCustomBindings(
            HdStBindingRequestVector * customBindings,
            bool * enableInstanceDraw) const override;
    private:
        bool _useDrawIndexed;
        bool _useInstanceCulling;
        size_t _bufferArrayHash;
    };

    void _CreateCullingProgram(
        HdStResourceRegistrySharedPtr const & resourceRegistry);

    void _CompileBatch(HdStResourceRegistrySharedPtr const & resourceRegistry);
    
    void _PrepareIndirectCommandBuffer(
        HdStRenderPassStateSharedPtr const & renderPassState,
        HdStResourceRegistrySharedPtr const & resourceRegistry,
        bool firstDrawBatch);

    bool _HasNothingToDraw() const;

    void _ExecuteDrawIndirect(
                HgiGraphicsCmds * gfxCmds,
                HdStBufferArrayRangeSharedPtr const & indexBar);

    void _ExecuteDrawImmediate(
                HgiGraphicsCmds * gfxCmds,
                HdStBufferArrayRangeSharedPtr const & indexBar);

    void _ExecuteFrustumCull(
                bool updateDispatchBuffer,
                HdStRenderPassStateSharedPtr const & renderPassState,
                HdStResourceRegistrySharedPtr const & resourceRegistry);

    void _ExecutePTCS(
            HgiGraphicsCmds *ptcsGfxCmds,
            HdStRenderPassStateSharedPtr const & renderPassState,
            HdStResourceRegistrySharedPtr const & resourceRegistry,
            bool firstDrawBatch);

    void _BeginGPUCountVisibleInstances(
        HdStResourceRegistrySharedPtr const & resourceRegistry);

    void _EndGPUCountVisibleInstances(
        HdStResourceRegistrySharedPtr const & resourceRegistry,
        size_t * result);

    HdStDispatchBufferSharedPtr _dispatchBuffer;
    HdStDispatchBufferSharedPtr _dispatchBufferCullInput;

    HdStBufferResourceSharedPtr _tessFactorsBuffer;

    std::vector<uint32_t> _drawCommandBuffer;
    bool _drawCommandBufferDirty;

    size_t _bufferArraysHash;
    size_t _barElementOffsetsHash;

    HdStBufferResourceSharedPtr _resultBuffer;

    size_t _numVisibleItems;
    size_t _numTotalVertices;
    size_t _numTotalElements;

    _CullingProgram _cullingProgram;
    bool _useTinyPrimCulling;
    bool _dirtyCullingProgram;

    bool _useDrawIndexed;
    bool _useInstancing;
    bool _useGpuCulling;
    bool _useInstanceCulling;
    bool const _allowGpuFrustumCulling;
    bool const _allowIndirectCommandEncoding;

    size_t _instanceCountOffset;
    size_t _cullInstanceCountOffset;
    size_t _drawCoordOffset;
    size_t _patchBaseVertexByteOffset;
    
    std::unique_ptr<HgiIndirectCommands> _indirectCommands;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_PIPELINE_DRAW_BATCH_H
