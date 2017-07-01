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
#ifndef HD_INDIRECT_DRAW_BATCH_H
#define HD_INDIRECT_DRAW_BATCH_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/dispatchBuffer.h"
#include "pxr/imaging/hd/drawBatch.h"
#include "pxr/imaging/hd/persistentBuffer.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


typedef std::vector<HdBindingRequest> HdBindingRequestVector;

/// \class Hd_IndirectDrawBatch
///
/// Drawing batch that is executed from an indirect dispatch buffer.
///
/// An indirect drawing batch accepts draw items that have the same
/// primitive mode and that share aggregated drawing resources,
/// e.g. uniform and non uniform primvar buffers.
///
class Hd_IndirectDrawBatch : public Hd_DrawBatch {
public:
    HD_API
    Hd_IndirectDrawBatch(HdDrawItemInstance * drawItemInstance);
    HD_API
    ~Hd_IndirectDrawBatch();

    // Hd_DrawBatch overrides
    HD_API
    virtual bool Validate(bool deepValidation);

    /// Prepare draw commands and apply view frustum culling for this batch.
    HD_API
    virtual void PrepareDraw(HdRenderPassStateSharedPtr const &renderPassState);

    /// Executes the drawing commands for this batch.
    HD_API
    virtual void ExecuteDraw(HdRenderPassStateSharedPtr const &renderPassState);

    HD_API
    virtual void DrawItemInstanceChanged(HdDrawItemInstance const* instance);

    /// Returns whether to do frustum culling on the GPU
    HD_API
    static bool IsEnabledGPUFrustumCulling();

    /// Returns whether to read back the count of visible items from the GPU
    /// Disabled by default, since there is some performance penalty.
    HD_API
    static bool IsEnabledGPUCountVisibleInstances();

    /// Returns whether to cull tiny prims (in screen space) during GPU culling 
    /// Enabled by default.
    HD_API
    static bool IsEnabledGPUTinyPrimCulling();

    /// Returns whether to do per-instance culling on the GPU
    HD_API
    static bool IsEnabledGPUInstanceFrustumCulling();

protected:
    HD_API
    virtual void _Init(HdDrawItemInstance * drawItemInstance);

private:
    void _ValidateCompatibility(
            HdBufferArrayRangeGLSharedPtr const& constantBar,
            HdBufferArrayRangeGLSharedPtr const& indexBar,
            HdBufferArrayRangeGLSharedPtr const& elementBar,
            HdBufferArrayRangeGLSharedPtr const& fvarBar,
            HdBufferArrayRangeGLSharedPtr const& vertexBar,
            int instancerNumLevels,
            HdBufferArrayRangeGLSharedPtr const& instanceIndexBar,
            std::vector<HdBufferArrayRangeGLSharedPtr> const& instanceBars) const;

    // Culling requires custom resource binding.
    class _CullingProgram : public _DrawingProgram {
    public:
        _CullingProgram()
            : _useDrawArrays(false)
            , _useInstanceCulling(false)
            , _bufferArrayHash(0) { }
        void Initialize(bool useDrawArrays, bool useInstanceCulling,
                        size_t bufferArrayHash);
    protected:
        // _DrawingProgram overrides
        virtual void _GetCustomBindings(
            HdBindingRequestVector *customBindings,
            bool *enableInstanceDraw) const;
        virtual bool _Link(HdGLSLProgramSharedPtr const & glslProgram);
    private:
        bool _useDrawArrays;
        bool _useInstanceCulling;
        size_t _bufferArrayHash;
    };
    _CullingProgram &_GetCullingProgram();

    void _CompileBatch();

    void _GPUFrustumCulling(HdDrawItem const *item,
                            HdRenderPassStateSharedPtr const &renderPassState);

    void _GPUFrustumCullingXFB(HdDrawItem const *item,
                               HdRenderPassStateSharedPtr const &renderPassState);

    void _BeginGPUCountVisibleInstances();
    // GLsync is not defined in gl.h. It's defined in spec as an opaque pointer:
    typedef struct __GLsync *GLsync;
    void _EndGPUCountVisibleInstances(GLsync resultSync, size_t * result);

    HdDispatchBufferSharedPtr _dispatchBuffer;
    HdDispatchBufferSharedPtr _dispatchBufferCullInput;

    std::vector<GLuint> _drawCommandBuffer;
    bool _drawCommandBufferDirty;
    size_t _bufferArraysHash;

    HdPersistentBufferSharedPtr _resultBuffer;

    size_t _numVisibleItems;
    size_t _numTotalVertices;
    size_t _numTotalElements;

    _CullingProgram _cullingProgram;
    bool _lastTinyPrimCulling;

    bool _useDrawArrays;
    bool _useInstancing;
    bool _useGpuCulling;
    bool _useGpuInstanceCulling;

    int _instanceCountOffset;
    int _cullInstanceCountOffset;

    // We'll use this fence to signal when GPU frustum culling is
    // complete if we need to read back result data from the GPU.
    GLsync _cullResultSync;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_INDIRECT_DRAW_BATCH_H
