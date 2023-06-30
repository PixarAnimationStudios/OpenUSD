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
#ifndef PXR_IMAGING_HD_ST_DRAW_BATCH_H
#define PXR_IMAGING_HD_ST_DRAW_BATCH_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hdSt/resourceBinder.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hdSt/materialNetworkShader.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HdStDrawItem;
class HdStDrawItemInstance;
class HgiGraphicsCmds;

using HdSt_DrawBatchSharedPtr = std::shared_ptr<class HdSt_DrawBatch>;
using HdSt_DrawBatchSharedPtrVector = std::vector<HdSt_DrawBatchSharedPtr>;
using HdSt_GeometricShaderSharedPtr =
    std::shared_ptr<class HdSt_GeometricShader>;
using HdStGLSLProgramSharedPtr= std::shared_ptr<class HdStGLSLProgram>;

using HdStRenderPassStateSharedPtr = std::shared_ptr<class HdStRenderPassState>;
using HdStResourceRegistrySharedPtr = 
    std::shared_ptr<class HdStResourceRegistry>;

/// \class HdSt_DrawBatch
///
/// A drawing batch.
///
/// This is the finest grained element of drawing, representing potentially
/// aggregated drawing resources dispatched with a minimal number of draw
/// calls.
///
class HdSt_DrawBatch
{
public:
    HDST_API
    HdSt_DrawBatch(HdStDrawItemInstance * drawItemInstance);

    HDST_API
    virtual ~HdSt_DrawBatch();

    /// Attempts to append \a drawItemInstance to the batch, returning \a false
    /// if the item could not be appended, e.g. if there was an aggregation
    /// conflict.
    HDST_API
    bool Append(HdStDrawItemInstance * drawItemInstance);

    /// Attempt to rebuild the batch in-place, returns false if draw items are
    /// no longer compatible.
    HDST_API
    bool Rebuild();

    enum class ValidationResult {
        ValidBatch = 0,
        RebuildBatch,
        RebuildAllBatches
    };

    /// Validates whether the  batch can be reused (i.e., submitted) as-is, or
    /// if it needs to be rebuilt, or if all batches need to be rebuilt.
    virtual ValidationResult Validate(bool deepValidation) = 0;

    /// Prepare draw commands and apply view frustum culling for this batch.
    virtual void PrepareDraw(
        HgiGraphicsCmds *gfxCmds,
        HdStRenderPassStateSharedPtr const &renderPassState,
        HdStResourceRegistrySharedPtr const &resourceRegistry) = 0;

    /// Encode drawing commands for this batch.
    virtual void EncodeDraw(
        HdStRenderPassStateSharedPtr const & renderPassState,
        HdStResourceRegistrySharedPtr const & resourceRegistry) = 0;

    /// Executes the drawing commands for this batch.
    virtual void ExecuteDraw(
        HgiGraphicsCmds *gfxCmds,
        HdStRenderPassStateSharedPtr const &renderPassState,
        HdStResourceRegistrySharedPtr const & resourceRegistry) = 0;

    /// Let the batch know that one of it's draw item instances has changed
    /// NOTE: This callback is called from multiple threads, so needs to be
    /// threadsafe.
    HDST_API
    virtual void DrawItemInstanceChanged(HdStDrawItemInstance const* instance);

    /// Let the batch know whether to use tiny prim culling.
    HDST_API
    virtual void SetEnableTinyPrimCulling(bool tinyPrimCulling);

protected:
    HDST_API
    virtual void _Init(HdStDrawItemInstance * drawItemInstance);

    /// \class _DrawingProgram
    ///
    /// This wraps glsl code generation and keeps track of
    /// binding assigments for bindable resources.
    ///
    class _DrawingProgram {
    public:
        using DrawingCoordBufferBinding =
                HdSt_ResourceBinder::MetaData::DrawingCoordBufferBinding;

        _DrawingProgram() {}

        HDST_API
        bool IsValid() const;

        HDST_API
        bool CompileShader(
                HdStDrawItem const *drawItem,
                HdStResourceRegistrySharedPtr const &resourceRegistry);

        HdStGLSLProgramSharedPtr GetGLSLProgram() const {
            return _glslProgram;
        }

        /// Returns the resouce binder, which is used for buffer resource
        /// bindings at draw time.
        const HdSt_ResourceBinder &GetBinder() const { 
            return _resourceBinder; 
        }

        void Reset() {
            _glslProgram.reset();
            _materialNetworkShader.reset();
            _geometricShader.reset();
            _resourceBinder = HdSt_ResourceBinder();
            _shaders.clear();
        }
        
        void SetDrawingCoordBufferBinding(
            DrawingCoordBufferBinding const &
                drawingCoordBufferBinding) {
            _drawingCoordBufferBinding = drawingCoordBufferBinding;
        }

        const DrawingCoordBufferBinding &
        GetDrawingCoordBufferBinding() const {
            return _drawingCoordBufferBinding;
        }

        void SetMaterialNetworkShader(
                HdSt_MaterialNetworkShaderSharedPtr const &shader) {
            _materialNetworkShader = shader;
        }

        const HdSt_MaterialNetworkShaderSharedPtr &
        GetMaterialNetworkShader() const {
            return _materialNetworkShader; 
        }

        void SetGeometricShader(HdSt_GeometricShaderSharedPtr shader) {
            _geometricShader = shader;
        }

        const HdSt_GeometricShaderSharedPtr &GetGeometricShader() const { 
            return _geometricShader; 
        }

        /// Set shaders (lighting/renderpass). In the case of Geometric Shaders 
        /// or Surface shaders you can use the specific setters.
        void SetShaders(HdStShaderCodeSharedPtrVector shaders) {
            _shaders = shaders; 
        }

        /// Returns array of shaders, this will not include the
        /// material network shader passed via SetMaterialNetworkShader
        /// (or the geometric shader).
        const HdStShaderCodeSharedPtrVector &GetShaders() const {
            return _shaders; 
        }

        /// Returns array of composed shaders, this include the shaders passed
        /// via SetShaders and the shader passed to SetMaterialNetworkShader.
        HdStShaderCodeSharedPtrVector GetComposedShaders() const {
            HdStShaderCodeSharedPtrVector shaders = _shaders;
            if (_materialNetworkShader) {
                shaders.push_back(_materialNetworkShader);
            }
            return shaders;
        }

    protected:
        // overrides populate customBindings and enableInstanceDraw which
        // will be used to determine if glVertexAttribDivisor needs to be
        // enabled or not.
        HDST_API
        virtual void _GetCustomBindings(
            HdStBindingRequestVector *customBindings,
            bool *enableInstanceDraw) const;

        HDST_API
        virtual bool _Link(HdStGLSLProgramSharedPtr const & glslProgram);

    private:
        HdStGLSLProgramSharedPtr _glslProgram;
        HdSt_ResourceBinder _resourceBinder;
        DrawingCoordBufferBinding _drawingCoordBufferBinding;
        HdStShaderCodeSharedPtrVector _shaders;
        HdSt_GeometricShaderSharedPtr _geometricShader;
        HdSt_MaterialNetworkShaderSharedPtr _materialNetworkShader;
    };

    HDST_API
    _DrawingProgram & _GetDrawingProgram(
        HdStRenderPassStateSharedPtr const &state, 
        HdStResourceRegistrySharedPtr const &resourceRegistry);

protected:
    HDST_API
    static bool _IsAggregated(HdStDrawItem const *drawItem0,
                              HdStDrawItem const *drawItem1);

    std::vector<HdStDrawItemInstance const*> _drawItemInstances;

private:
    _DrawingProgram _program;
    HdStShaderCode::ID _shaderHash;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_DRAW_BATCH_H
