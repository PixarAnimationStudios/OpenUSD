//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

using HdStBindingRequestVector = std::vector<class HdStBindingRequest>;

/// \class HdSt_IndirectDrawBatch
///
/// Drawing batch that is executed from an indirect dispatch buffer.
///
/// An indirect drawing batch accepts draw items that have the same
/// primitive type and that share aggregated drawing resources,
/// e.g. uniform and non uniform primvar buffers.
///
class HdSt_IndirectDrawBatch : public HdSt_DrawBatch
{
public:
    HDST_API
    HdSt_IndirectDrawBatch(HdStDrawItemInstance * drawItemInstance,
                           bool const allowGpuFrustumCulling = true,
                           bool const allowTextureResourceRebinding = false);
    HDST_API
    ~HdSt_IndirectDrawBatch() override;

    // HdSt_DrawBatch overrides
    HDST_API
    ValidationResult
    Validate(bool deepValidation) override;

    /// Prepare draw commands and apply view frustum culling for this batch.
    HDST_API
    void PrepareDraw(
        HgiGraphicsCmds *gfxCmds,
        HdStRenderPassStateSharedPtr const &renderPassState,
        HdStResourceRegistrySharedPtr const &resourceRegistry) override;

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
        HdStRenderPassStateSharedPtr const &renderPassState,
        HdStResourceRegistrySharedPtr const &resourceRegistry,
        bool firstDrawBatch) override;

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
    void _ExecuteDraw(
        HdStRenderPassStateSharedPtr const &renderPassState,
        HdStResourceRegistrySharedPtr const &resourceRegistry);

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
            : _useDrawIndexed(true)
            , _useInstanceCulling(false)
            , _bufferArrayHash(0) { }
        void Initialize(bool useDrawIndexed, bool useInstanceCulling,
                        size_t bufferArrayHash);
    protected:
        // _DrawingProgram overrides
        void _GetCustomBindings(
            HdStBindingRequestVector *customBindings,
            bool *enableInstanceDraw) const override;
    private:
        bool _useDrawIndexed;
        bool _useInstanceCulling;
        size_t _bufferArrayHash;
    };

    _CullingProgram &_GetCullingProgram(
        HdStResourceRegistrySharedPtr const &resourceRegistry);

    void _CompileBatch(HdStResourceRegistrySharedPtr const &resourceRegistry);

    bool _HasNothingToDraw() const;

    void _ExecuteDrawIndirect(
                HdSt_GeometricShaderSharedPtr const & geometricShader,
                HdStDispatchBufferSharedPtr const & dispatchBuffer,
                HdStBufferArrayRangeSharedPtr const & indexBar);

    void _ExecuteDrawImmediate(
                HdSt_GeometricShaderSharedPtr const & geometricShader,
                HdStDispatchBufferSharedPtr const & dispatchBuffer,
                HdStBufferArrayRangeSharedPtr const & indexBar,
                _DrawingProgram const & program);

    void _ExecuteFrustumCull(
                bool updateDispatchBuffer,
                HdStRenderPassStateSharedPtr const & renderPassState,
                HdStResourceRegistrySharedPtr const & resourceRegistry);

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

    bool _useDrawIndexed;
    bool _useInstancing;
    bool _useGpuCulling;
    bool _useInstanceCulling;
    bool const _allowGpuFrustumCulling;

    int _instanceCountOffset;
    int _cullInstanceCountOffset;

    bool _needsTextureResourceRebinding;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_INDIRECT_DRAW_BATCH_H
