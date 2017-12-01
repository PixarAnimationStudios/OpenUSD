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
#ifndef HDST_DRAW_BATCH_H
#define HDST_DRAW_BATCH_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/resourceBinder.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/shaderCode.h"

#include <boost/shared_ptr.hpp>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


class HdDrawItem;
class HdStDrawItemInstance;

typedef boost::shared_ptr<class HdSt_DrawBatch> HdSt_DrawBatchSharedPtr;
typedef boost::shared_ptr<class Hd_GeometricShader> Hd_GeometricShaderSharedPtr;
typedef boost::shared_ptr<class HdGLSLProgram> HdGLSLProgramSharedPtr;
typedef boost::shared_ptr<class HdRenderPassState> HdRenderPassStateSharedPtr;
typedef std::vector<HdSt_DrawBatchSharedPtr> HdSt_DrawBatchSharedPtrVector;
typedef std::vector<class HdBindingRequest> HdBindingRequestVector;
typedef boost::shared_ptr<class HdStResourceRegistry>
    HdStResourceRegistrySharedPtr;

/// \class HdSt_DrawBatch
///
/// A drawing batch.
///
/// This is the finest grained element of drawing, representing potentially
/// aggregated drawing resources dispatched with a minimal number of draw
/// calls.
///
class HdSt_DrawBatch {
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

    /// Validates that all batches are referring up to date bufferarrays.
    /// If not, returns false
    virtual bool Validate(bool deepValidation) = 0;

    /// Prepare draw commands and apply view frustum culling for this batch.
    virtual void PrepareDraw(
        HdRenderPassStateSharedPtr const &renderPassState,
        HdStResourceRegistrySharedPtr const &resourceRegistry) = 0;

    /// Executes the drawing commands for this batch.
    virtual void ExecuteDraw(
        HdRenderPassStateSharedPtr const &renderPassState,
        HdStResourceRegistrySharedPtr const & resourceRegistry) = 0;

    /// Let the batch know that one of it's draw item instances has changed
    /// NOTE: This callback is called from multiple threads, so needs to be
    /// threadsafe.
    HDST_API
    virtual void DrawItemInstanceChanged(HdStDrawItemInstance const* instance);

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
        _DrawingProgram() {}

        HDST_API
        bool CompileShader(
                HdDrawItem const *drawItem,
                bool indirect,
                HdStResourceRegistrySharedPtr const &resourceRegistry);

        HdGLSLProgramSharedPtr GetGLSLProgram() const {
            return _glslProgram;
        }

        /// Returns the resouce binder, which is used for buffer resource
        /// bindings at draw time.
        const Hd_ResourceBinder &GetBinder() const { 
            return _resourceBinder; 
        }

        void Reset() {
            _glslProgram.reset();
            _surfaceShader.reset();
            _geometricShader.reset();
            _resourceBinder = Hd_ResourceBinder();
            _shaders.clear();
        }
        
        void SetSurfaceShader(HdShaderCodeSharedPtr shader) {
            _surfaceShader = shader;
        }

        const HdShaderCodeSharedPtr &GetSurfaceShader() {
            return _surfaceShader; 
        }

        void SetGeometricShader(Hd_GeometricShaderSharedPtr shader) {
            _geometricShader = shader;
        }

        const Hd_GeometricShaderSharedPtr &GetGeometricShader() { 
            return _geometricShader; 
        }

        /// Set shaders (lighting/renderpass). In the case of Geometric Shaders 
        /// or Surface shaders you can use the specific setters.
        void SetShaders(HdShaderCodeSharedPtrVector shaders) {
            _shaders = shaders; 
        }

        /// Returns array of shaders, this will not include the surface shader
        /// passed via SetSurfaceShader (or the geometric shader).
        const HdShaderCodeSharedPtrVector &GetShaders() const {
            return _shaders; 
        }

        /// Returns array of composed shaders, this include the shaders passed
        /// via SetShaders and the shader passed to SetSurfaceShader.
        HdShaderCodeSharedPtrVector GetComposedShaders() const {
            HdShaderCodeSharedPtrVector shaders = _shaders;
            if (_surfaceShader) {
                shaders.push_back(_surfaceShader);
            }
            return shaders;
        }

    protected:
        // overrides populate customBindings and enableInstanceDraw which
        // will be used to determine if glVertexAttribDivisor needs to be
        // enabled or not.
        HDST_API
        virtual void _GetCustomBindings(
            HdBindingRequestVector *customBindings,
            bool *enableInstanceDraw) const;

        HDST_API
        virtual bool _Link(HdGLSLProgramSharedPtr const & glslProgram);

    private:
        HdGLSLProgramSharedPtr _glslProgram;
        Hd_ResourceBinder _resourceBinder;
        HdShaderCodeSharedPtrVector _shaders;
        Hd_GeometricShaderSharedPtr _geometricShader;
        HdShaderCodeSharedPtr _surfaceShader;
    };

    HDST_API
    _DrawingProgram & _GetDrawingProgram(
        HdRenderPassStateSharedPtr const &state, 
        bool indirect,
        HdStResourceRegistrySharedPtr const &resourceRegistry);

protected:
    HDST_API
    static bool _IsAggregated(HdDrawItem const *drawItem0,
                              HdDrawItem const *drawItem1);

    std::vector<HdStDrawItemInstance const*> _drawItemInstances;

private:
    _DrawingProgram _program;
    HdShaderCode::ID _shaderHash;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDST_DRAW_BATCH_H
