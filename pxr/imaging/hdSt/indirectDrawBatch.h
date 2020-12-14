//
// Copyright 2016 Pixar
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
#ifndef PXR_IMAGING_HD_ST_INDIRECT_DRAW_BATCH_H
#define PXR_IMAGING_HD_ST_INDIRECT_DRAW_BATCH_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/dispatchBuffer.h"
#include "pxr/imaging/hdSt/drawBatch.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

using HdBindingRequestVector = std::vector<HdBindingRequest>;

/// \class HdSt_IndirectDrawBatch
///
/// Drawing batch that is executed from an indirect dispatch buffer.
///
/// An indirect drawing batch accepts draw items that have the same
/// primitive mode and that share aggregated drawing resources,
/// e.g. uniform and non uniform primvar buffers.
///
class HdSt_IndirectDrawBatch : public HdSt_DrawBatch
{
public:
    HDST_API
    HdSt_IndirectDrawBatch(HdStDrawItemInstance * drawItemInstance);
    HDST_API
    ~HdSt_IndirectDrawBatch() override;

    // HdSt_DrawBatch overrides
    HDST_API
    ValidationResult
    Validate(bool deepValidation) override;

    /// Prepare draw commands and apply view frustum culling for this batch.
    HDST_API
    void PrepareDraw(
        HdStRenderPassStateSharedPtr const &renderPassState,
        HdStResourceRegistrySharedPtr const &resourceRegistry) override;

    /// Executes the drawing commands for this batch.
    HDST_API
    void ExecuteDraw(
        HdStRenderPassStateSharedPtr const &renderPassState,
        HdStResourceRegistrySharedPtr const &resourceRegistry) override;

    HDST_API
    void DrawItemInstanceChanged(
        HdStDrawItemInstance const* instance) override;

    HDST_API
    void SetEnableTinyPrimCulling(bool tinyPrimCulling) override;

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
    void _ValidateCompatibility(
        HdStBufferArrayRangeSharedPtr const& constantBar,
        HdStBufferArrayRangeSharedPtr const& indexBar,
        HdStBufferArrayRangeSharedPtr const& topologyVisibilityBar,
        HdStBufferArrayRangeSharedPtr const& elementBar,
        HdStBufferArrayRangeSharedPtr const& fvarBar,
        HdStBufferArrayRangeSharedPtr const& varyingBar,
        HdStBufferArrayRangeSharedPtr const& vertexBar,
        int instancerNumLevels,
        HdStBufferArrayRangeSharedPtr const& instanceIndexBar,
        std::vector<HdStBufferArrayRangeSharedPtr> const& instanceBars) const;

    // Culling requires custom resource binding.
    class _CullingProgram : public _DrawingProgram
    {
    public:
        _CullingProgram()
            : _useDrawArrays(false)
            , _useInstanceCulling(false)
            , _bufferArrayHash(0) { }
        void Initialize(bool useDrawArrays, bool useInstanceCulling,
                        size_t bufferArrayHash);
    protected:
        // _DrawingProgram overrides
        void _GetCustomBindings(
            HdBindingRequestVector *customBindings,
            bool *enableInstanceDraw) const override;
    private:
        bool _useDrawArrays;
        bool _useInstanceCulling;
        size_t _bufferArrayHash;
    };

    _CullingProgram &_GetCullingProgram(
        HdStResourceRegistrySharedPtr const &resourceRegistry);

    void _CompileBatch(HdStResourceRegistrySharedPtr const &resourceRegistry);

    void _GPUFrustumInstanceCulling(HdStDrawItem const *item,
        HdStRenderPassStateSharedPtr const &renderPassState,
        HdStResourceRegistrySharedPtr const &resourceRegistry);

    void _GPUFrustumNonInstanceCulling(HdStDrawItem const *item,
        HdStRenderPassStateSharedPtr const &renderPassState,
        HdStResourceRegistrySharedPtr const &resourceRegistry);

    void _BeginGPUCountVisibleInstances(
        HdStResourceRegistrySharedPtr const &resourceRegistry);

    void _EndGPUCountVisibleInstances(
        HdStResourceRegistrySharedPtr const &resourceRegistry, 
        size_t * result);

    HdStDispatchBufferSharedPtr _dispatchBuffer;
    HdStDispatchBufferSharedPtr _dispatchBufferCullInput;

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

    bool _useDrawArrays;
    bool _useInstancing;
    bool _useGpuCulling;
    bool _useGpuInstanceCulling;

    int _instanceCountOffset;
    int _cullInstanceCountOffset;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_INDIRECT_DRAW_BATCH_H
