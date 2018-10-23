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

/// \file usdImagingGL/engine.h

#ifndef USDIMAGINGGL_ENGINE_H
#define USDIMAGINGGL_ENGINE_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImagingGL/api.h"
#include "pxr/usdImaging/usdImaging/version.h"

#include "pxr/usdImaging/usdImagingGL/renderParams.h"
#include "pxr/usdImaging/usdImagingGL/rendererSettings.h"

#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/simpleMaterial.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/timeCode.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/base/vt/dictionary.h"

PXR_NAMESPACE_OPEN_SCOPE


class UsdPrim;
class HdRenderIndex;

typedef boost::shared_ptr<class GlfGLContext> GlfGLContextSharedPtr;
TF_DECLARE_WEAK_AND_REF_PTRS(GlfDrawTarget);
TF_DECLARE_WEAK_PTRS(GlfSimpleLightingContext);

/// \class UsdImagingGLEngine
///
/// Interface class for render engines.
///
class UsdImagingGLEngine {
public:
    UsdImagingGLEngine() = default;

    // Disallow copies
    UsdImagingGLEngine(const UsdImagingGLEngine&) = delete;
    UsdImagingGLEngine& operator=(const UsdImagingGLEngine&) = delete;

    USDIMAGINGGL_API
    virtual ~UsdImagingGLEngine();

    /// Returns true if Hydra is enabled for GL drawing.
    USDIMAGINGGL_API
    static bool IsHydraEnabled();

    struct HitInfo {
        GfVec3d worldSpaceHitPoint;
        int hitInstanceIndex;
    };
    typedef TfHashMap<SdfPath, HitInfo, SdfPath::Hash> HitBatch;

    /// Support for batched drawing
    USDIMAGINGGL_API
    virtual void PrepareBatch(const UsdPrim& root, 
                              const UsdImagingGLRenderParams& params);
    USDIMAGINGGL_API
    virtual void RenderBatch(const SdfPathVector& paths, 
                             const UsdImagingGLRenderParams& params);

    /// Entry point for kicking off a render
    virtual void Render(const UsdPrim& root, 
                        const UsdImagingGLRenderParams &params) = 0;

    virtual void InvalidateBuffers() = 0;

    USDIMAGINGGL_API
    virtual void SetCameraState(const GfMatrix4d& viewMatrix,
                                const GfMatrix4d& projectionMatrix,
                                const GfVec4d& viewport);

    /// Helper function to extract camera state from opengl and then
    /// call SetCameraState.
    USDIMAGINGGL_API
    void SetCameraStateFromOpenGL();

    /// Helper function to extract lighting state from opengl and then
    /// call SetLights.
    USDIMAGINGGL_API
    virtual void SetLightingStateFromOpenGL();

    /// Copy lighting state from another lighting context.
    USDIMAGINGGL_API
    virtual void SetLightingState(GlfSimpleLightingContextPtr const &src);

    /// Set lighting state
    /// Derived classes should ensure that passing an empty lights
    /// vector disables lighting.
    /// \param lights is the set of lights to use, or empty to disable lighting.
    USDIMAGINGGL_API
    virtual void SetLightingState(GlfSimpleLightVector const &lights,
                                  GlfSimpleMaterial const &material,
                                  GfVec4f const &sceneAmbient);

    /// Sets the root transform.
    USDIMAGINGGL_API
    virtual void SetRootTransform(GfMatrix4d const& xf);

    /// Sets the root visibility.
    USDIMAGINGGL_API
    virtual void SetRootVisibility(bool isVisible);

    // selection highlighting

    /// Sets (replaces) the list of prim paths that should be included in selection
    /// highlighting. These paths may include root paths which will be expanded
    /// internally.
    USDIMAGINGGL_API
    virtual void SetSelected(SdfPathVector const& paths);

    /// Clear the list of prim paths that should be included in selection
    /// highlighting.
    USDIMAGINGGL_API
    virtual void ClearSelected();

    /// Add a path with instanceIndex to the list of prim paths that should be
    /// included in selection highlighting. UsdImagingDelegate::ALL_INSTANCES
    /// can be used for highlighting all instances if path is an instancer.
    USDIMAGINGGL_API
    virtual void AddSelected(SdfPath const &path, int instanceIndex);

    /// Sets the selection highlighting color.
    USDIMAGINGGL_API
    virtual void SetSelectionColor(GfVec4f const& color);

    /// Finds closest point of intersection with a frustum by rendering.
    ///	
    /// This method uses a PickRender and a customized depth buffer to find an
    /// approximate point of intersection by rendering. This is less accurate
    /// than implicit methods or rendering with GL_SELECT, but leverages any data
    /// already cached in the renderer.
    ///
    /// Returns whether a hit occurred and if so, \p outHitPoint will contain the
    /// intersection point in world space (i.e. \p projectionMatrix and
    /// \p viewMatrix factored back out of the result).
    ///
    USDIMAGINGGL_API
    virtual bool TestIntersection(
        const GfMatrix4d &viewMatrix,
        const GfMatrix4d &projectionMatrix,
        const GfMatrix4d &worldToLocalSpace,
        const UsdPrim& root,
        const UsdImagingGLRenderParams& params,
        GfVec3d *outHitPoint,
        SdfPath *outHitPrimPath = NULL,
        SdfPath *outInstancerPath = NULL,
        int *outHitInstanceIndex = NULL,
        int *outHitElementIndex = NULL) = 0;

    /// A callback function to control collating intersection test hits.
    /// See the documentation for TestIntersectionBatch() below for more detail.
    typedef std::function< SdfPath(const SdfPath&, const SdfPath&, const int) > PathTranslatorCallback;

    /// Finds closest point of intersection with a frustum by rendering a batch.
    ///
    /// This method uses a PickRender and a customized depth buffer to find an
    /// approximate point of intersection by rendering. This is less accurate
    /// than implicit methods or rendering with GL_SELECT, but leverages any data
    /// already cached in the renderer. The resolution of the pick renderer is
    /// controlled through \p pickResolution.
    ///
    /// In batched selection scenarios, the path desired may not be as granular as
    /// the leaf-level prim. For example, one might want to find the closest hit
    /// for all prims underneath a certain path scope, or ignore others altogether.
    /// The \p pathTranslator receives an \c SdfPath pointing to the hit prim
    /// as well as an \c SdfPath pointing to the instancer prim and an integer
    /// instance index in the case where the hit is an instanced object. It may
    /// return an empty path (signifying an ignored hit), or a different
    /// simplified path altogether.
    ///
    /// Returned hits are collated by the translated \c SdfPath above, and placed
    /// in the structure pointed to by \p outHit. For each \c SdfPath in the
    /// \c HitBatch, the closest found hit point and instance id is given. The
    /// intersection point returned is in world space (i.e. \p projectionMatrix
    /// and \p viewMatrix factored back out of the result).
    ///
    /// \c outHit is not cleared between consecutive runs -- this allows
    /// hits to be accumulated across multiple calls to \cTestIntersection. Hits
    /// to any single SdfPath will be overwritten on successive calls.
    ///
    USDIMAGINGGL_API
    virtual bool TestIntersectionBatch(
        const GfMatrix4d &viewMatrix,
        const GfMatrix4d &projectionMatrix,
        const GfMatrix4d &worldToLocalSpace,
        const SdfPathVector& paths, 
        const UsdImagingGLRenderParams& params,
        unsigned int pickResolution,
        PathTranslatorCallback pathTranslator,
        HitBatch *outHit) = 0;

    /// Using an Id extracted from an Id render, returns the associated
    /// rprim path.
    ///
    /// Note that this function doesn't resolve instancer relationship.
    /// returning prim can be a prototype mesh which may not exist in usd stage.
    /// It can be resolved to the actual usd prim and corresponding instance
    /// index by GetPrimPathFromInstanceIndex().
    ///
    USDIMAGINGGL_API
    virtual SdfPath GetRprimPathFromPrimId(int primId) const;


    /// Using colors extracted from an Id render, returns the associated
    /// prim path and optional instance index.
    ///
    /// Note that this function doesn't resolve instancer relationship.
    /// returning prim can be a prototype mesh which may not exist in usd stage.
    /// It can be resolved to the actual usd prim and corresponding instance
    /// index by GetPrimPathFromInstanceIndex().
    ///
    /// XXX: consider renaming to GetRprimPathFromPrimIdColor
    ///
    USDIMAGINGGL_API
    virtual SdfPath GetPrimPathFromPrimIdColor(
        GfVec4i const & primIdColor,
        GfVec4i const & instanceIdColor,
        int * instanceIndexOut = NULL);

    /// Returns the path of the instance prim on the UsdStage being rendered
    /// by this engine that corresponds to the instance index generated by
    /// the specified prototype rprim.
    /// Returns an empty path if no such instance prim exists.
    ///
    /// absoluteInstanceIndex is also returned, which is an instance index
    /// of all instances in the instancer. Note that if the instancer instances
    /// heterogeneously, instanceIndex of the prototype rprim doesn't match
    /// the absoluteInstanceIndex in the instancer (see hd/sceneDelegate.h)
    /// 
    /// If \p instanceContext is not NULL, it is populated with the list of 
    /// instance roots that must be traversed to get to the rprim. The last prim
    /// in this vector is always the resolved (or forwarded) rprim.
    /// 
    USDIMAGINGGL_API
    virtual SdfPath GetPrimPathFromInstanceIndex(
        SdfPath const& protoRprimPath,
        int instanceIndex,
        int *absoluteInstanceIndex=NULL,
        SdfPath * rprimPath=NULL,
        SdfPathVector *instanceContext=NULL);

    /// Returns true if the resulting image is fully converged.
    /// (otherwise, caller may need to call Render() again to refine the result)
    USDIMAGINGGL_API
    virtual bool IsConverged() const;

    /// Return the vector of available render-graph delegate plugins.
    USDIMAGINGGL_API
    virtual TfTokenVector GetRendererPlugins() const;

    /// Return the user-friendly description of a renderer plugin.
    USDIMAGINGGL_API
    virtual std::string GetRendererDisplayName(TfToken const &id) const;

    /// Return the id of the currently used renderer plugin.
    USDIMAGINGGL_API
    virtual TfToken GetCurrentRendererId() const;

    /// Set the current render-graph delegate to \p id.
    /// the plugin will be loaded if it's not yet.
    USDIMAGINGGL_API
    virtual bool SetRendererPlugin(TfToken const &id);

    /// Return the vector of available renderer AOV settings.
    USDIMAGINGGL_API
    virtual TfTokenVector GetRendererAovs() const;

    /// Set the current renderer AOV to \p id.
    USDIMAGINGGL_API
    virtual bool SetRendererAov(TfToken const& id);

    /// Returns GPU resource allocation info
    USDIMAGINGGL_API
    virtual VtDictionary GetResourceAllocation() const;

    /// Returns the list of renderer settings.
    USDIMAGINGGL_API
    virtual UsdImagingGLRendererSettingsList GetRendererSettingsList() const;

    /// Gets a renderer setting's current value.
    USDIMAGINGGL_API
    virtual VtValue GetRendererSetting(TfToken const& id) const;

    /// Sets a renderer setting's value.
    USDIMAGINGGL_API
    virtual void SetRendererSetting(TfToken const& id,
                                    VtValue const& value);

protected:

    /// Open some protected methods for whitebox testing.
    friend class UsdImagingGL_UnitTestGLDrawing;
    friend class UsdImagingGL;

    /// Returns the render index of the engine, if any.  This is only used for
    /// whitebox testing.
    USDIMAGINGGL_API
    virtual HdRenderIndex *_GetRenderIndex() const;

    USDIMAGINGGL_API
    virtual void _Render(const UsdImagingGLRenderParams &params);


};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDIMAGINGGL_ENGINE_H
