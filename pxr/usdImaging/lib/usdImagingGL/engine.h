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

/// \file engine.h

#pragma once

#include "pxr/usdImaging/usdImaging/version.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/type.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/timeCode.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/base/vt/dictionary.h"

#include <boost/noncopyable.hpp>
#include <boost/unordered_map.hpp>

class UsdPrim;

typedef boost::shared_ptr<class GlfGLContext> GlfGLContextSharedPtr;
TF_DECLARE_WEAK_AND_REF_PTRS(GlfDrawTarget);
TF_DECLARE_WEAK_PTRS(GlfSimpleLightingContext);

/// \class UsdImagingGLEngine
///
/// Interface class for render engines.
///
class UsdImagingGLEngine : private boost::noncopyable {
public:
    virtual ~UsdImagingGLEngine();

    enum DrawMode {
        DRAW_POINTS,
        DRAW_WIREFRAME,
        DRAW_WIREFRAME_ON_SURFACE,
        DRAW_SHADED_FLAT,
        DRAW_SHADED_SMOOTH,
        DRAW_GEOM_ONLY,
        DRAW_GEOM_FLAT,
        DRAW_GEOM_SMOOTH
    };

    enum CullStyle {
        CULL_STYLE_NOTHING,
        CULL_STYLE_BACK,
        CULL_STYLE_FRONT,
        CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED,

        CULL_STYLE_COUNT
    };

    typedef std::vector<GfVec4d> ClipPlanesVector;

    struct RenderParams {
        UsdTimeCode frame;
        float complexity;
        DrawMode drawMode;
        bool showGuides;
        bool showRenderGuides;
        bool forceRefresh;
        bool flipFrontFacing;
        CullStyle cullStyle;
        bool enableIdRender;
        bool enableLighting;
        bool enableSampleAlphaToCoverage;
        bool applyRenderState;
        bool gammaCorrectColors;
        bool highlight;
        GfVec4f overrideColor;
        GfVec4f wireframeColor;
        float alphaThreshold; // threshold < 0 implies automatic
        ClipPlanesVector clipPlanes;
        bool enableHardwareShading;

        RenderParams() : 
            frame(UsdTimeCode::Default()),
            complexity(1.0),
            drawMode(DRAW_SHADED_SMOOTH),
            showGuides(false),
            showRenderGuides(false),
            forceRefresh(false),
            flipFrontFacing(false),
            cullStyle(CULL_STYLE_NOTHING),
            enableIdRender(false),
            enableLighting(true),
            enableSampleAlphaToCoverage(false),
            applyRenderState(true),
            gammaCorrectColors(true),
            highlight(false),
            overrideColor(.0f, .0f, .0f, .0f),
            wireframeColor(.0f, .0f, .0f, .0f),
            alphaThreshold(-1),
            clipPlanes(),
            enableHardwareShading(true)
        {
        }

        bool operator==(const RenderParams &other) const {
            return frame                        == other.frame
                and complexity                  == other.complexity
                and drawMode                    == other.drawMode
                and showGuides                  == other.showGuides
                and showRenderGuides            == other.showRenderGuides
                and forceRefresh                == other.forceRefresh
                and flipFrontFacing             == other.flipFrontFacing
                and cullStyle                   == other.cullStyle
                and enableIdRender              == other.enableIdRender
                and enableLighting              == other.enableLighting
                and enableSampleAlphaToCoverage == other.enableSampleAlphaToCoverage
                and applyRenderState            == other.applyRenderState
                and gammaCorrectColors          == other.gammaCorrectColors
                and highlight                   == other.highlight
                and overrideColor               == other.overrideColor
                and wireframeColor              == other.wireframeColor
                and alphaThreshold              == other.alphaThreshold
                and clipPlanes                  == other.clipPlanes
                and enableHardwareShading       == other.enableHardwareShading;
        }
        bool operator!=(const RenderParams &other) const {
            return not (*this == other);
        }
    };

    struct HitInfo {
        GfVec3d worldSpaceHitPoint;
        int hitInstanceIndex;
    };
    typedef TfHashMap<SdfPath, HitInfo, SdfPath::Hash> HitBatch;

    /// Support for batched drawing
    virtual void PrepareBatch(const UsdPrim& root, RenderParams params);
    virtual void RenderBatch(const SdfPathVector& paths, RenderParams params);

    /// Entry point for kicking off a render
    virtual void Render(const UsdPrim& root, RenderParams params) = 0;

    virtual void InvalidateBuffers() = 0;

    virtual void SetCameraState(const GfMatrix4d& viewMatrix,
                                const GfMatrix4d& projectionMatrix,
                                const GfVec4d& viewport);

    /// Helper function to extract camera state from opengl and then
    /// call SetCameraState.
    void SetCameraStateFromOpenGL();

    /// Helper function to extract lighting state from opengl and then
    /// call SetLights.
    virtual void SetLightingStateFromOpenGL();

    /// Copy lighting state from another lighting context.
    virtual void SetLightingState(GlfSimpleLightingContextPtr const &src);

    /// Sets the root transform.
    virtual void SetRootTransform(GfMatrix4d const& xf);

    /// Sets the root visibility.
    virtual void SetRootVisibility(bool isVisible);

    // selection highlighting

    /// Sets (replaces) the list of prim paths that should be included in selection
    /// highlighting. These paths may include root paths which will be expanded
    /// internally.
    virtual void SetSelected(SdfPathVector const& paths);

    /// Clear the list of prim paths that should be included in selection
    /// highlighting.
    virtual void ClearSelected();

    /// Add a path with instanceIndex to the list of prim paths that should be
    /// included in selection highlighting. UsdImagingDelegate::ALL_INSTANCES
    /// can be used for highlighting all instances if path is an instancer.
    virtual void AddSelected(SdfPath const &path, int instanceIndex);

    /// Sets the selection highlighting color.
    virtual void SetSelectionColor(GfVec4f const& color);

    /// Finds closest point of interesection with a frustum by rendering.
    ///	
    /// This method uses a PickRender and a customized depth buffer to find an
    /// approximate point of intersection by rendering. This is less accurate
    /// than implicit methods or rendering with GL_SELECT, but leverages any data
    /// already cached in the renderer.
    ///
    /// Returns whether a hit occured and if so, \p outHitPoint will contain the
    /// intersection point in world space (i.e. \p projectionMatrix and
    /// \p viewMatrix factored back out of the result).
    ///
    virtual bool TestIntersection(
        const GfMatrix4d &viewMatrix,
        const GfMatrix4d &projectionMatrix,
        const GfMatrix4d &worldToLocalSpace,
        const UsdPrim& root,
        RenderParams params,
        GfVec3d *outHitPoint,
        SdfPath *outHitPrimPath = NULL,
        SdfPath *outInstancerPath = NULL,
        int *outHitInstanceIndex = NULL);

    /// A callback function to control collating intersection test hits.
    /// See the documentation for TestIntersectionBatch() below for more detail.
    typedef std::function< SdfPath(const SdfPath&, const SdfPath&, const int) > PathTranslatorCallback;

    /// Finds closest point of interesection with a frustum by rendering a batch.
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
    virtual bool TestIntersectionBatch(
        const GfMatrix4d &viewMatrix,
        const GfMatrix4d &projectionMatrix,
        const GfMatrix4d &worldToLocalSpace,
        const SdfPathVector& paths, 
        RenderParams params,
        unsigned int pickResolution,
        PathTranslatorCallback pathTranslator,
        HitBatch *outHit);

    /// Using colors extracted from an Id render, returns the associated
    /// prim path and optional instance index.
    ///
    /// note that this function doesn't resolve instancer relationship.
    /// returning prim can be a prototype mesh which may not exist in usd stage.
    /// It can be resolved to the actual usd prim and corresponding instance
    /// index by GetPrimPathFromInstanceIndex().
    ///
    /// XXX: consider renaming to GetRprimPathFromPrimIdColor
    ///
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
    virtual SdfPath GetPrimPathFromInstanceIndex(
        SdfPath const& protoRprimPath,
        int instanceIndex,
        int *absoluteInstanceIndex=NULL,
        std::vector<UsdPrim> *instanceContext=NULL);

    /// Returns true if the resulting image is fully converged.
    /// (otherwise, caller may need to call Render() again to refine the result)
    virtual bool IsConverged() const;

    /// Return the typevector of available render-graph delegate plugins.
    virtual std::vector<TfType> GetRenderGraphPlugins();

    /// Set the current render-graph delegate to \p type.
    /// the plugin for the type will be loaded if not yet.
    virtual bool SetRenderGraphPlugin(TfType const &type);

    /// Returns GPU resource allocation info
    virtual VtDictionary GetResourceAllocation() const;

protected:
    // Intentionally putting these under protected so that subclasses can share the usage of draw targets.
    // Once refEngine goes away and we only have hdEngine, it may be best to move this to private
    typedef boost::unordered_map<GlfGLContextSharedPtr, GlfDrawTargetRefPtr> _DrawTargetPerContextMap;
    _DrawTargetPerContextMap _drawTargets;
};

