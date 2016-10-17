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

/// \file gl.h

#pragma once

#include "pxr/usdImaging/usdImagingGL/engine.h"

#include <boost/shared_ptr.hpp>

class SdfPath;

typedef std::vector<SdfPath> SdfPathVector;
typedef std::vector<UsdPrim> UsdPrimVector;

typedef boost::shared_ptr<class UsdImagingGLEngine> UsdImagingGLEngineSharedPtr;
typedef boost::shared_ptr<class UsdImagingGL> UsdImagingGLSharedPtr;
typedef std::vector<UsdImagingGLSharedPtr> UsdImagingGLSharedPtrVector;

/// \class UsdImagingGL
///
/// Convenience class that abstracts whether we are rendering via
/// a high-performance Hd render engine, or a simple vbo renderer that can
/// run on old openGl versions.
///
/// The first time a UsdImagingGL is created in a process, we decide whether
/// it and all subsequently created objects will use Hd if:
/// \li the machine's hardware and insalled openGl are sufficient
/// \li the environment variable HD_ENABLED is unset, or set to "1"
/// 
/// So, to disable Hd rendering for testing purposes, set HD_ENABLED to "0"
///
class UsdImagingGL : public UsdImagingGLEngine {
public:

    /// Returns true if Hydra is enabled for GL drawing.
    static bool IsEnabledHydra();

    UsdImagingGL();
    UsdImagingGL(const SdfPath& rootPath,
                 const SdfPathVector& excludedPaths,
                 const SdfPathVector& invisedPaths=SdfPathVector(),
                 const SdfPath& sharedId = SdfPath::AbsoluteRootPath(),
                 const UsdImagingGLSharedPtr& sharedImaging = UsdImagingGLSharedPtr());

    virtual ~UsdImagingGL();

    // Support for batched rendering
    // Currently, supported only when Hydra is enabled
    static bool IsBatchingSupported();

    // Prepares the given sub-index delegates for drawing.
    // This is equivalent to calling PrepareBatch on each renderer in
    // \p renderers with the corresponding root prim, time, and parameters.
    static void PrepareBatch(
        const UsdImagingGLSharedPtrVector& renderers,
        const UsdPrimVector& rootPrims,
        const std::vector<UsdTimeCode>& times,
        RenderParams params);

    /// Prepares a sub-index delegate for drawing.
    ///
    /// This can be called many times for different sub-indexes (prim paths)
    /// over the stage, and then all rendered together with a call to
    /// RenderBatch()
    virtual void PrepareBatch(const UsdPrim& root, RenderParams params);

    /// Draws all sub-indices indentified by \p paths.  Presumes that each
    /// sub-index has already been prepared for drawing by calling
    /// PrepareBatch()
    virtual void RenderBatch(const SdfPathVector& paths, RenderParams params);

    /// Render everything at and beneath \p root, using the configuration in
    /// \p params
    ///
    /// If this is the first call to Render(), \p root will become the limiting
    /// root for all future calls to Render().  That is, you can call Render()
    /// again on \p root or any descendant of \p root, but not on any parent,
    /// sibling, or cousin of \p root.
    virtual void Render(const UsdPrim& root, RenderParams params);

    virtual void InvalidateBuffers();

    virtual void SetCameraState(const GfMatrix4d& viewMatrix,
                                const GfMatrix4d& projectionMatrix,
                                const GfVec4d& viewport);

    /// Helper function to extract lighting state from opengl and then
    /// call SetLights.
    virtual void SetLightingStateFromOpenGL();

    /// Copy lighting state from another lighting context.
    virtual void SetLightingState(GlfSimpleLightingContextPtr const &src);

    virtual void SetRootTransform(GfMatrix4d const& xf);

    virtual void SetRootVisibility(bool isVisible);

    /// Set the paths for selection highlighting. Note that these paths may 
    /// include prefix root paths, which will be expanded internally.
    virtual void SetSelected(SdfPathVector const& paths);

    virtual void ClearSelected();
    virtual void AddSelected(SdfPath const &path, int instanceIndex);

    /// Set the color for selection highlighting.
    virtual void SetSelectionColor(GfVec4f const& color);

    virtual SdfPath GetPrimPathFromPrimIdColor(
        GfVec4i const& primIdColor,
        GfVec4i const& instanceIdColor,
        int* instanceIndexOut = NULL);

    virtual SdfPath GetPrimPathFromInstanceIndex(
        const SdfPath& protoPrimPath,
        int instanceIndex,
        int *absoluteInstanceIndex = NULL,
        SdfPath * rprimPath=NULL,
        SdfPathVector *instanceContext=NULL);

    virtual bool IsConverged() const;

    virtual std::vector<TfType> GetRenderGraphPlugins();

    virtual bool SetRenderGraphPlugin(TfType const &type);

    virtual bool TestIntersection(
        const GfMatrix4d &viewMatrix,
        const GfMatrix4d &projectionMatrix,
        const GfMatrix4d &worldToLocalSpace,
        const UsdPrim& root, 
        RenderParams params,
        GfVec3d *outHitPoint,
        SdfPath *outHitPrimPath = NULL,
        SdfPath *outHitInstancerPath = NULL,
        int *outHitInstanceIndex = NULL);

    virtual bool TestIntersectionBatch(
        const GfMatrix4d &viewMatrix,
        const GfMatrix4d &projectionMatrix,
        const GfMatrix4d &worldToLocalSpace,
        const SdfPathVector& paths, 
        RenderParams params,
        unsigned int pickResolution,
        PathTranslatorCallback pathTranslator,
        HitBatch *outHit);

    virtual VtDictionary GetResourceAllocation() const;

private:
    UsdImagingGLEngineSharedPtr _engine;
};

