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
#ifndef HD_DRAW_BATCH_H
#define HD_DRAW_BATCH_H

#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/resourceBinder.h" // XXX: including private header
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/shader.h"
#include "pxr/imaging/garch/gl.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/vec2f.h"

#include <boost/shared_ptr.hpp>
#include <vector>

class HdDrawItem;
class HdDrawItemInstance;

typedef boost::shared_ptr<class Hd_DrawBatch> Hd_DrawBatchSharedPtr;
typedef boost::shared_ptr<class Hd_GeometricShader> Hd_GeometricShaderSharedPtr;
typedef boost::shared_ptr<class HdGLSLProgram> HdGLSLProgramSharedPtr;
typedef boost::shared_ptr<class HdRenderPassState> HdRenderPassStateSharedPtr;
typedef std::vector<Hd_DrawBatchSharedPtr> Hd_DrawBatchSharedPtrVector;
typedef std::vector<class HdBindingRequest> HdBindingRequestVector;

/// \class Hd_DrawBatch
///
/// A drawing batch.
///
/// This is the finest grained element of drawing, representing potentially
/// aggregated drawing resources dispatched with a minimal number of draw
/// calls.
///
class Hd_DrawBatch {
public:
    Hd_DrawBatch(HdDrawItemInstance * drawItemInstance);

    virtual ~Hd_DrawBatch();

    /// Attempts to append \a drawItemInstance to the batch, returning \a false
    /// if the item could not be appended, e.g. if there was an aggregation
    /// conflict.
    bool Append(HdDrawItemInstance * drawItemInstance);

    /// Attempt to rebuild the batch in-place, returns false if draw items are
    /// no longer compatible.
    bool Rebuild();

    /// Validates that all batches are referring up to date bufferarrays.
    /// If not, returns false
    virtual bool Validate(bool deepValidation) = 0;

    /// Prepare draw commands and apply view frustum culling for this batch.
    virtual void PrepareDraw(HdRenderPassStateSharedPtr const &renderPassState) = 0;

    /// Executes the drawing commands for this batch.
    virtual void ExecuteDraw(HdRenderPassStateSharedPtr const &renderPassState) = 0;

    /// Let the batch know that one of it's draw item instances has changed
    /// NOTE: This callback is called from multiple threads, so needs to be
    /// threadsafe.
    virtual void DrawItemInstanceChanged(HdDrawItemInstance const* instance);

protected:
    virtual void _Init(HdDrawItemInstance * drawItemInstance);

    /// \class _DrawingProgram
    ///
    /// This wraps glsl code generation and keeps track of
    /// binding assigments for bindable resources.
    ///
    class _DrawingProgram {
    public:
        _DrawingProgram() {}

        void CompileShader(
                HdDrawItem const *drawItem,
                Hd_GeometricShaderSharedPtr const &geometricShader,
                HdShaderSharedPtrVector const &shaders,
                bool indirect);

        HdGLSLProgramSharedPtr GetGLSLProgram() const {
            return _glslProgram;
        }

        /// Returns the resouce binder, which is used for buffer resource
        /// bindings at draw time.
        const Hd_ResourceBinder &GetBinder() const { return _resourceBinder; }

        void Reset() {
            _glslProgram.reset();
            _resourceBinder = Hd_ResourceBinder();
        }

    protected:
        // overrides populate customBindings and enableInstanceDraw which
        // will be used to determine if glVertexAttribDivisor needs to be
        // enabled or not.
        virtual void _GetCustomBindings(
            HdBindingRequestVector *customBindings,
            bool *enableInstanceDraw) const;

        virtual bool _Link(HdGLSLProgramSharedPtr const & glslProgram);

    private:
        HdGLSLProgramSharedPtr _glslProgram;
        Hd_ResourceBinder _resourceBinder;
    };

    _DrawingProgram & _GetDrawingProgram(
        HdRenderPassStateSharedPtr const &state, bool indirect);

protected:
    static bool _IsAggregated(HdDrawItem const *drawItem0,
                              HdDrawItem const *drawItem1);

    std::vector<HdDrawItemInstance const*> _drawItemInstances;

private:
    _DrawingProgram _program;
    HdShader::ID _shaderHash;
};

#endif // HD_DRAW_BATCH_H
