//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

/// \file usdImagingGL/engine.h

#ifndef PXR_USD_IMAGING_USD_IMAGING_GL_ENGINE_H
#define PXR_USD_IMAGING_USD_IMAGING_GL_ENGINE_H

#include "pxr/imaging/hd/noticeBatchingSceneIndex.h"
#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImagingGL/api.h"
#include "pxr/usdImaging/usdImagingGL/version.h"
#include "pxr/usdImaging/usdImaging/version.h"

#include "pxr/usdImaging/usdImagingGL/renderParams.h"
#include "pxr/usdImaging/usdImagingGL/rendererSettings.h"

#include "pxr/imaging/cameraUtil/conformWindow.h"

#include "pxr/imaging/hd/driver.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/pluginRenderDelegateUniqueHandle.h"

#include "pxr/imaging/hdx/selectionTracker.h"
#include "pxr/imaging/hdx/renderSetupTask.h"

#include "pxr/imaging/hgi/hgi.h"

#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/simpleMaterial.h"

#include "pxr/imaging/hgi/hgi.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/timeCode.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4i.h"

#include "pxr/base/vt/dictionary.h"

#include "pxr/base/tf/declarePtrs.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdPrim;
class HdRenderIndex;
class HdxTaskController;
class UsdImagingDelegate;

TF_DECLARE_WEAK_AND_REF_PTRS(GlfSimpleLightingContext);
TF_DECLARE_REF_PTRS(HdNoticeBatchingSceneIndex);
TF_DECLARE_REF_PTRS(UsdImagingStageSceneIndex);
TF_DECLARE_REF_PTRS(UsdImagingRootOverridesSceneIndex);
TF_DECLARE_REF_PTRS(UsdImagingSelectionSceneIndex);
TF_DECLARE_REF_PTRS(HdsiLegacyDisplayStyleOverrideSceneIndex);
TF_DECLARE_REF_PTRS(HdsiPrimTypePruningSceneIndex);
TF_DECLARE_REF_PTRS(HdsiSceneGlobalsSceneIndex);
TF_DECLARE_REF_PTRS(HdSceneIndexBase);
TF_DECLARE_REF_PTRS(HdxTaskControllerSceneIndex);

using UsdStageWeakPtr = TfWeakPtr<class UsdStage>;

namespace UsdImagingGLEngine_Impl
{
    using _AppSceneIndicesSharedPtr = std::shared_ptr<struct _AppSceneIndices>;
}

/// \class UsdImagingGLEngine
///
/// The UsdImagingGLEngine is the main entry point API for rendering USD scenes.
///
class UsdImagingGLEngine
{
public:
    /// Parameters to construct UsdImagingGLEngine.
    struct Parameters
    {
        SdfPath rootPath = SdfPath::AbsoluteRootPath();
        SdfPathVector excludedPaths;
        SdfPathVector invisedPaths;
        SdfPath sceneDelegateID = SdfPath::AbsoluteRootPath();
        /// An HdDriver, containing the Hgi of your choice, can be optionally passed
        /// in during construction. This can be helpful if your application creates
        /// multiple UsdImagingGLEngine's that wish to use the same HdDriver / Hgi.
        HdDriver driver;
        /// The \p rendererPluginId argument indicates the renderer plugin that
        /// Hydra should use. If the empty token is passed in, a default renderer
        /// plugin will be chosen depending on the value of \p gpuEnabled.
        TfToken rendererPluginId;
        /// The \p gpuEnabled argument determines if this instance will allow Hydra
        /// to use the GPU to produce images.
        bool gpuEnabled = true;
        /// \p displayUnloadedPrimsWithBounds draws bounding boxes for unloaded
        /// prims if they have extents/extentsHint authored.
        bool displayUnloadedPrimsWithBounds = false;
        /// \p allowAsynchronousSceneProcessing indicates to constructed hydra
        /// scene indices that asynchronous processing is allowow. Applications
        /// should perodically call PollForAsynchronousUpdates on the engine.
        bool allowAsynchronousSceneProcessing = false;
        /// \p enableUsdDrawModes enables the UsdGeomModelAPI draw mode
        /// feature.
        bool enableUsdDrawModes = true;
    };

    // ---------------------------------------------------------------------
    /// \name Construction
    /// @{
    // ---------------------------------------------------------------------

    USDIMAGINGGL_API
    UsdImagingGLEngine(const Parameters &params);

    /// An HdDriver, containing the Hgi of your choice, can be optionally passed
    /// in during construction. This can be helpful if you application creates
    /// multiple UsdImagingGLEngine that wish to use the same HdDriver / Hgi.
    /// The \p rendererPluginId argument indicates the renderer plugin that
    /// Hyrda should use. If the empty token is passed in, a default renderer
    /// plugin will be chosen depending on the value of \p gpuEnabled.
    /// The \p gpuEnabled argument determines if this instance will allow Hydra
    /// to use the GPU to produce images.
    USDIMAGINGGL_API
    UsdImagingGLEngine(const HdDriver& driver = HdDriver(),
                       const TfToken& rendererPluginId = TfToken(),
                       bool gpuEnabled = true);

    USDIMAGINGGL_API
    UsdImagingGLEngine(const SdfPath& rootPath,
                       const SdfPathVector& excludedPaths,
                       const SdfPathVector& invisedPaths = SdfPathVector(),
                       const SdfPath& sceneDelegateID =
                                        SdfPath::AbsoluteRootPath(),
                       const HdDriver& driver = HdDriver(),
                       const TfToken& rendererPluginId = TfToken(),
                       const bool gpuEnabled = true,
                       const bool displayUnloadedPrimsWithBounds = false,
                       const bool allowAsynchronousSceneProcessing = false,
                       const bool enableUsdDrawModes = true);

    // Disallow copies
    UsdImagingGLEngine(const UsdImagingGLEngine&) = delete;
    UsdImagingGLEngine& operator=(const UsdImagingGLEngine&) = delete;

    USDIMAGINGGL_API
    ~UsdImagingGLEngine();

    /// @}

    // ---------------------------------------------------------------------
    /// \name Rendering
    /// @{
    // ---------------------------------------------------------------------

    /// Support for batched drawing
    USDIMAGINGGL_API
    void PrepareBatch(const UsdPrim& root, 
                      const UsdImagingGLRenderParams& params);
    USDIMAGINGGL_API
    void RenderBatch(const SdfPathVector& paths, 
                     const UsdImagingGLRenderParams& params);

    /// Entry point for kicking off a render
    USDIMAGINGGL_API
    void Render(const UsdPrim& root, 
                const UsdImagingGLRenderParams &params);

    /// Returns true if the resulting image is fully converged.
    /// (otherwise, caller may need to call Render() again to refine the result)
    USDIMAGINGGL_API
    bool IsConverged() const;

    /// @}
    
    // ---------------------------------------------------------------------
    /// \name Root Transform and Visibility
    /// @{
    // ---------------------------------------------------------------------

    /// Sets the root transform.
    USDIMAGINGGL_API
    void SetRootTransform(GfMatrix4d const& xf);

    /// Sets the root visibility.
    USDIMAGINGGL_API
    void SetRootVisibility(bool isVisible);

    /// @}

    // ---------------------------------------------------------------------
    /// \name Camera State
    /// @{
    // ---------------------------------------------------------------------
    
    /// Scene camera API
    /// Set the scene camera path to use for rendering.
    USDIMAGINGGL_API
    void SetCameraPath(SdfPath const& id);

    /// Determines how the filmback of the camera is mapped into
    /// the pixels of the render buffer and what pixels of the render
    /// buffer will be rendered into.
    USDIMAGINGGL_API
    void SetFraming(CameraUtilFraming const& framing);

    /// Specifies whether to force a window policy when conforming
    /// the frustum of the camera to match the display window of
    /// the camera framing.
    ///
    /// If set to {false, ...}, the window policy of the specified camera
    /// will be used.
    ///
    /// Note: std::pair<bool, ...> is used instead of std::optional<...>
    /// because the latter is only available in C++17 or later.
    USDIMAGINGGL_API
    void SetOverrideWindowPolicy(
        const std::optional<CameraUtilConformWindowPolicy> &policy);

    /// Set the size of the render buffers baking the AOVs.
    /// GUI applications should set this to the size of the window.
    ///
    USDIMAGINGGL_API
    void SetRenderBufferSize(GfVec2i const& size);

    /// Set the viewport to use for rendering as (x,y,w,h), where (x,y)
    /// represents the lower left corner of the viewport rectangle, and (w,h)
    /// is the width and height of the viewport in pixels.
    ///
    /// \deprecated Use SetFraming and SetRenderBufferSize instead.
    USDIMAGINGGL_API
    void SetRenderViewport(GfVec4d const& viewport);

    /// Set the window policy to use.
    /// XXX: This is currently used for scene cameras set via SetCameraPath.
    /// See comment in SetCameraState for the free cam.
    USDIMAGINGGL_API
    void SetWindowPolicy(CameraUtilConformWindowPolicy policy);

    /// Free camera API
    /// Set camera framing state directly (without pointing to a camera on the 
    /// USD stage). The projection matrix is expected to be pre-adjusted for the
    /// window policy.
    USDIMAGINGGL_API
    void SetCameraState(const GfMatrix4d& viewMatrix,
                        const GfMatrix4d& projectionMatrix);

    /// @}

    // ---------------------------------------------------------------------
    /// \name Light State
    /// @{
    // ---------------------------------------------------------------------

    /// Copy lighting state from another lighting context.
    USDIMAGINGGL_API
    void SetLightingState(GlfSimpleLightingContextPtr const &src);

    /// Set lighting state
    /// Derived classes should ensure that passing an empty lights
    /// vector disables lighting.
    /// \param lights is the set of lights to use, or empty to disable lighting.
    USDIMAGINGGL_API
    void SetLightingState(GlfSimpleLightVector const &lights,
                          GlfSimpleMaterial const &material,
                          GfVec4f const &sceneAmbient);

    /// @}

    // ---------------------------------------------------------------------
    /// \name Selection Highlighting
    /// @{
    // ---------------------------------------------------------------------

    /// Sets (replaces) the list of prim paths that should be included in 
    /// selection highlighting. These paths may include root paths which will 
    /// be expanded internally.
    USDIMAGINGGL_API
    void SetSelected(SdfPathVector const& paths);

    /// Clear the list of prim paths that should be included in selection
    /// highlighting.
    USDIMAGINGGL_API
    void ClearSelected();

    /// Add a path with instanceIndex to the list of prim paths that should be
    /// included in selection highlighting. UsdImagingDelegate::ALL_INSTANCES
    /// can be used for highlighting all instances if path is an instancer.
    USDIMAGINGGL_API
    void AddSelected(SdfPath const &path, int instanceIndex);

    /// Sets the selection highlighting color.
    USDIMAGINGGL_API
    void SetSelectionColor(GfVec4f const& color);

    /// @}
    
    // ---------------------------------------------------------------------
    /// \name Picking
    /// @{
    // ---------------------------------------------------------------------
    
    /// Finds closest point of intersection with a frustum by rendering.
    ///	
    /// This method uses a PickRender and a customized depth buffer to find an
    /// approximate point of intersection by rendering. This is less accurate
    /// than implicit methods or rendering with GL_SELECT, but leverages any 
    /// data already cached in the renderer.
    ///
    /// Returns whether a hit occurred and if so, \p outHitPoint will contain
    /// the intersection point in world space (i.e. \p projectionMatrix and
    /// \p viewMatrix factored back out of the result), and \p outHitNormal
    /// will contain the world space normal at that point.
    ///
    /// \p outHitPrimPath will point to the gprim selected by the pick.
    /// \p outHitInstancerPath will point to the point instancer (if applicable)
    /// of that gprim. For nested instancing, outHitInstancerPath points to
    /// the closest instancer.
    ///
    /// \deprecated Please use the override of TestIntersection that takes
    /// PickParams and returns an IntersectionResultVector instead!
    USDIMAGINGGL_API
    bool TestIntersection(
        const GfMatrix4d &viewMatrix,
        const GfMatrix4d &projectionMatrix,
        const UsdPrim& root,
        const UsdImagingGLRenderParams &params,
        GfVec3d *outHitPoint,
        GfVec3d *outHitNormal,
        SdfPath *outHitPrimPath = NULL,
        SdfPath *outHitInstancerPath = NULL,
        int *outHitInstanceIndex = NULL,
        HdInstancerContext *outInstancerContext = NULL);

    // Pick result
    struct IntersectionResult
    {
        GfVec3d hitPoint;
        GfVec3d hitNormal;
        SdfPath hitPrimPath;
        SdfPath hitInstancerPath;
        int hitInstanceIndex;
        HdInstancerContext instancerContext;
    };

    typedef std::vector<struct IntersectionResult> IntersectionResultVector;

    // Pick params
    struct PickParams
    {
        TfToken resolveMode;
    };

    /// Perform picking by finding the intersection of objects in the scene with a renderered frustum.
    /// Depending on the resolve mode it may find all objects intersecting the frustum or the closest 
    /// point of intersection within the frustum.
    /// 
    /// If resolve mode is set to resolveDeep it uses Deep Selection to gather all paths within 
    /// the frustum even if obscured by other visible objects.
    /// If resolve mode is set to resolveNearestToCenter it uses a PickRender and 
    /// a customized depth buffer to find all approximate points of intersection by rendering. 
    /// This is less accurate than implicit methods or rendering with GL_SELECT, but leverages any 
    /// data already cached in the renderer.
    ///
    /// Returns whether a hit occurred and if so, \p outResults will point to all the 
    /// gprims selected by the pick as determined by the resolve mode. 
    /// \p outHitPoint will contain the intersection point in world space 
    /// (i.e. \p projectionMatrix and \p viewMatrix factored back out of the result)
    /// \p outHitNormal will contain the world space normal at that point.
    /// \p hitPrimPath will point to the gprim selected by the pick.
    /// \p hitInstancerPath will point to the point instancer (if applicable) of each gprim. 
    ///
    USDIMAGINGGL_API
    bool TestIntersection(
        const PickParams& pickParams,
        const GfMatrix4d& viewMatrix,
        const GfMatrix4d& projectionMatrix,
        const UsdPrim& root,
        const UsdImagingGLRenderParams& params,
        IntersectionResultVector* outResults);

    /// Decodes a pick result given hydra prim ID/instance ID (like you'd get
    /// from an ID render), where ID is represented as a vec4 color.
    USDIMAGINGGL_API
    bool DecodeIntersection(
        unsigned char const primIdColor[4],
        unsigned char const instanceIdColor[4],
        SdfPath *outHitPrimPath = NULL,
        SdfPath *outHitInstancerPath = NULL,
        int *outHitInstanceIndex = NULL,
        HdInstancerContext *outInstancerContext = NULL);

    /// Decodes a pick result given hydra prim ID/instance ID (like you'd get
    /// from an ID render), where ID is represented as a int.
    USDIMAGINGGL_API
    bool DecodeIntersection(
        int primIdx,
        int instanceIdx,
        SdfPath *outHitPrimPath = NULL,
        SdfPath *outHitInstancerPath = NULL,
        int *outHitInstanceIndex = NULL,
        HdInstancerContext *outInstancerContext = NULL);

    /// @}
    
    // ---------------------------------------------------------------------
    /// \name Renderer Plugin Management
    /// @{
    // ---------------------------------------------------------------------

    /// Return the vector of available render-graph delegate plugins.
    USDIMAGINGGL_API
    static TfTokenVector GetRendererPlugins();

    /// Return the user-friendly name of a renderer plugin.
    USDIMAGINGGL_API
    static std::string GetRendererDisplayName(TfToken const &id);

    /// Return the user-friendly name of the Hgi implementation.
    /// For example: OpenGL, Metal, Vulkan. This is only available
    /// if a render plugin was set and it uses Hgi.
    USDIMAGINGGL_API
    std::string GetRendererHgiDisplayName() const;

    /// Return if the GPU is enabled and can be used for any rendering tasks.
    USDIMAGINGGL_API
    bool GetGPUEnabled() const;

    /// Return the id of the currently used renderer plugin.
    USDIMAGINGGL_API
    TfToken GetCurrentRendererId() const;

    /// Set the current render-graph delegate to \p id.
    /// the plugin will be loaded if it's not yet.
    USDIMAGINGGL_API
    bool SetRendererPlugin(TfToken const &id);

    /// @}
    
    // ---------------------------------------------------------------------
    /// \name AOVs
    /// @{
    // ---------------------------------------------------------------------

    /// Return the vector of available renderer AOV settings.
    USDIMAGINGGL_API
    TfTokenVector GetRendererAovs() const;

    /// Set the current renderer AOV to \p id.
    USDIMAGINGGL_API
    bool SetRendererAov(TfToken const& id);

    /// Set the current renderer AOVs to a list of \p ids.
    USDIMAGINGGL_API
    bool SetRendererAovs(TfTokenVector const &ids);

    /// Returns an AOV texture handle for the given token.
    USDIMAGINGGL_API
    HgiTextureHandle GetAovTexture(TfToken const& name) const;

    /// Returns the AOV render buffer for the given token.
    USDIMAGINGGL_API
    HdRenderBuffer* GetAovRenderBuffer(TfToken const& name) const;
        
    // ---------------------------------------------------------------------
    /// \name Render Settings (Legacy)
    /// @{
    // ---------------------------------------------------------------------
    
    /// Returns the list of renderer settings.
    USDIMAGINGGL_API
    UsdImagingGLRendererSettingsList GetRendererSettingsList() const;

    /// Gets a renderer setting's current value.
    USDIMAGINGGL_API
    VtValue GetRendererSetting(TfToken const& id) const;

    /// Sets a renderer setting's value.
    USDIMAGINGGL_API
    void SetRendererSetting(TfToken const& id,
                            VtValue const& value);
    
    /// @}

    // ---------------------------------------------------------------------
    /// \name Scene-defined Render Pass and Render Settings
    /// \note Support is WIP.
    /// @{
    // ---------------------------------------------------------------------

    /// Set active render pass prim to use to drive rendering.
    USDIMAGINGGL_API
    void SetActiveRenderPassPrimPath(SdfPath const &);
    
    /// Set active render settings prim to use to drive rendering.
    USDIMAGINGGL_API
    void SetActiveRenderSettingsPrimPath(SdfPath const &);

    /// Utility method to query available render settings prims.
    USDIMAGINGGL_API
    static SdfPathVector
    GetAvailableRenderSettingsPrimPaths(UsdPrim const &root);

    /// @}

    // ---------------------------------------------------------------------
    /// \name Presentation
    /// @{
    // ---------------------------------------------------------------------
    
    /// Enable / disable presenting the render to bound framebuffer.
    /// An application may choose to manage the AOVs that are rendered into
    /// itself and skip the engine's presentation.
    USDIMAGINGGL_API
    void SetEnablePresentation(bool enabled);

    /// The destination API (e.g., OpenGL, see hgiInterop for details) and
    /// framebuffer that the AOVs are presented into. The framebuffer
    /// is a VtValue that encoding a framebuffer in a destination API
    /// specific way.
    /// E.g., a uint32_t (aka GLuint) for framebuffer object for OpenGL.
    USDIMAGINGGL_API
    void SetPresentationOutput(TfToken const &api, VtValue const &framebuffer);

    /// @}
    
    // ---------------------------------------------------------------------
    /// \name Renderer Command API
    /// @{
    // ---------------------------------------------------------------------

    /// Return command deescriptors for commands supported by the active 
    /// render delegate.
    ///
    USDIMAGINGGL_API
    HdCommandDescriptors GetRendererCommandDescriptors() const;

    /// Invokes command on the active render delegate. If successful, returns
    /// \c true, returns \c false otherwise. Note that the command will not
    /// succeeed if it is not among those returned by
    /// GetRendererCommandDescriptors() for the same active render delegate.
    ///
    USDIMAGINGGL_API
    bool InvokeRendererCommand(
            const TfToken &command, 
            const HdCommandArgs &args = HdCommandArgs()) const;

    // ---------------------------------------------------------------------
    /// \name Control of background rendering threads.
    /// @{
    // ---------------------------------------------------------------------

    /// Query the renderer as to whether it supports pausing and resuming.
    USDIMAGINGGL_API
    bool IsPauseRendererSupported() const;

    /// Pause the renderer.
    ///
    /// Returns \c true if successful.
    USDIMAGINGGL_API
    bool PauseRenderer();

    /// Resume the renderer.
    ///
    /// Returns \c true if successful.
    USDIMAGINGGL_API
    bool ResumeRenderer();

    /// Query the renderer as to whether it supports stopping and restarting.
    USDIMAGINGGL_API
    bool IsStopRendererSupported() const;

    /// Stop the renderer.
    ///
    /// Returns \c true if successful.
    USDIMAGINGGL_API
    bool StopRenderer();

    /// Restart the renderer.
    ///
    /// Returns \c true if successful.
    USDIMAGINGGL_API
    bool RestartRenderer();

    /// @}

    // ---------------------------------------------------------------------
    /// \name Color Correction
    /// @{
    // ---------------------------------------------------------------------

    /// Set \p ccType to one of the HdxColorCorrectionTokens:
    /// {disabled, sRGB, openColorIO}
    ///
    /// If 'openColorIO' is used, \p ocioDisplay, \p ocioView, \p ocioColorSpace
    /// and \p ocioLook are options the client may supply to configure OCIO.
    /// \p ocioColorSpace refers to the input (source) color space. 
    /// The default value is substituted if an option isn't specified.
    /// You can find the values for these strings inside the
    /// profile/config .ocio file. For example:
    ///
    ///  displays:
    ///    rec709g22:
    ///      !<View> {name: studio, colorspace: linear, looks: studio_65_lg2}
    ///
    USDIMAGINGGL_API
    void SetColorCorrectionSettings(
        TfToken const& ccType,
        TfToken const& ocioDisplay = {},
        TfToken const& ocioView = {},
        TfToken const& ocioColorSpace = {},
        TfToken const& ocioLook = {});

    /// @}

    /// Returns true if the platform is color correction capable.
    USDIMAGINGGL_API
    static bool IsColorCorrectionCapable();

    // ---------------------------------------------------------------------
    /// \name Render Statistics
    /// @{
    // ---------------------------------------------------------------------

    /// Returns render statistics.
    ///
    /// The contents of the dictionary will depend on the current render 
    /// delegate.
    ///
    USDIMAGINGGL_API
    VtDictionary GetRenderStats() const;

    /// @}

    // ---------------------------------------------------------------------
    /// \name HGI
    /// @{
    // ---------------------------------------------------------------------

    /// Returns the HGI interface.
    ///
    USDIMAGINGGL_API
    Hgi* GetHgi();

    /// @}

    // ---------------------------------------------------------------------
    /// \name Asynchronous
    /// @{
    // ---------------------------------------------------------------------
    
    /// If \p allowAsynchronousSceneProcessing is true within the Parameters
    /// provided to the UsdImagingGLEngine constructor, an application can
    /// periodically call this from the main thread.
    ///
    /// A return value of true indicates that the scene has changed and the
    /// render should be updated.
    USDIMAGINGGL_API
    bool PollForAsynchronousUpdates() const;

    /// @}

protected:

    /// Open some protected methods for whitebox testing.
    friend class UsdImagingGL_UnitTestGLDrawing;

    /// Returns the render index of the engine, if any.  This is only used for
    /// whitebox testing.
    USDIMAGINGGL_API
    HdRenderIndex *_GetRenderIndex() const;

    /// \deprecated.
    /// Use _Execute(const UsdImaginGLRenderParams &, const SdfPathVector &).
    USDIMAGINGGL_API
    void _Execute(const UsdImagingGLRenderParams &params,
                  const HdTaskSharedPtrVector tasks);

    USDIMAGINGGL_API
    void _Execute(const UsdImagingGLRenderParams &params,
                  const SdfPathVector &taskPaths);

    USDIMAGINGGL_API
    bool _CanPrepare(const UsdPrim& root);
    USDIMAGINGGL_API
    void _PreSetTime(const UsdImagingGLRenderParams& params);
    USDIMAGINGGL_API
    void _PostSetTime(const UsdImagingGLRenderParams& params);

    USDIMAGINGGL_API
    void _PrepareRender(const UsdImagingGLRenderParams& params);

    USDIMAGINGGL_API
    void _SetActiveRenderSettingsPrimFromStageMetadata(UsdStageWeakPtr stage);

    USDIMAGINGGL_API
    void _SetSceneGlobalsCurrentFrame(UsdTimeCode const &time);

    USDIMAGINGGL_API
    void _UpdateDomeLightCameraVisibility();

    using BBoxVector = std::vector<GfBBox3d>;

    USDIMAGINGGL_API
    void _SetBBoxParams(
        const BBoxVector& bboxes,
        const GfVec4f& bboxLineColor,
        float bboxLineDashSize);

    // Create a hydra collection given root paths and render params.
    // Returns true if the collection was updated.
    USDIMAGINGGL_API
    static bool _UpdateHydraCollection(HdRprimCollection *collection,
                          SdfPathVector const& roots,
                          UsdImagingGLRenderParams const& params);
    USDIMAGINGGL_API
    static HdxRenderTaskParams _MakeHydraUsdImagingGLRenderParams(
                          UsdImagingGLRenderParams const& params);
    USDIMAGINGGL_API
    static void _ComputeRenderTags(UsdImagingGLRenderParams const& params,
                          TfTokenVector *renderTags);

    USDIMAGINGGL_API
    void _InitializeHgiIfNecessary();

    USDIMAGINGGL_API
    void _SetRenderDelegateAndRestoreState(
        HdPluginRenderDelegateUniqueHandle &&);

    USDIMAGINGGL_API
    void _SetRenderDelegate(HdPluginRenderDelegateUniqueHandle &&);

    USDIMAGINGGL_API
    SdfPath _ComputeControllerPath(const HdPluginRenderDelegateUniqueHandle &);

    USDIMAGINGGL_API
    static TfToken _GetDefaultRendererPluginId();

    /// Get a direct pointer to the scene delegate.
    /// \deprecated Existing instances of this call will be replaced with new
    ///             APIs on this class, to support multiplexing between the
    ///             scene delegate and scene index. This API is scheduled for
    ///             deletion.
    USDIMAGINGGL_API
    UsdImagingDelegate *_GetSceneDelegate() const;

    USDIMAGINGGL_API
    HdEngine *_GetHdEngine();

    USDIMAGINGGL_API
    HdxTaskController *_GetTaskController() const;

    USDIMAGINGGL_API
    HdSelectionSharedPtr _GetSelection() const;

protected:

    // Note that any of the fields below might become private
    // in the future and subclasses should use the above getters
    // to access them instead.

    HgiUniquePtr _hgi;
    // Similar for HdDriver.
    HdDriver _hgiDriver;

    VtValue _userFramebuffer;

protected:
    bool _displayUnloadedPrimsWithBounds;
    bool _gpuEnabled;
    HdPluginRenderDelegateUniqueHandle _renderDelegate;
    std::unique_ptr<HdRenderIndex> _renderIndex;

    SdfPath const _sceneDelegateId;

    std::unique_ptr<HdxTaskController> _taskController;
    HdxTaskControllerSceneIndexRefPtr _taskControllerSceneIndex;

    HdxSelectionTrackerSharedPtr _selTracker;
    HdRprimCollection _renderCollection;
    HdRprimCollection _intersectCollection;

    GlfSimpleLightingContextRefPtr _lightingContextForOpenGLState;

    // Data we want to live across render plugin switches:
    GfVec4f _selectionColor;
    bool _domeLightCameraVisibility;

    SdfPath _rootPath;
    SdfPathVector _excludedPrimPaths;
    SdfPathVector _invisedPrimPaths;
    bool _isPopulated;

private:
    // Registers app-managed scene indices with the scene index plugin registry.
    // This needs to be called once *before* the render index is constructed.
    static void _RegisterApplicationSceneIndices();

    // Creates and returns the scene globals scene index. This callback is
    // registered prior to render index construction and is invoked during
    // render index construction via
    // HdSceneIndexPluginRegistry::AppendSceneIndicesForRenderer(..).
    static HdSceneIndexBaseRefPtr
    _AppendSceneGlobalsSceneIndexCallback(
        const std::string &renderInstanceId,
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs);

    HdSceneIndexBaseRefPtr
    _AppendOverridesSceneIndices(
        const HdSceneIndexBaseRefPtr &inputScene);
    
    UsdImagingGLEngine_Impl::_AppSceneIndicesSharedPtr _appSceneIndices;

    void _DestroyHydraObjects();

    // Note that we'll only ever use one of _sceneIndex/_sceneDelegate
    // at a time.
    UsdImagingStageSceneIndexRefPtr _stageSceneIndex;
    HdNoticeBatchingSceneIndexRefPtr _postInstancingNoticeBatchingSceneIndex;
    UsdImagingSelectionSceneIndexRefPtr _selectionSceneIndex;
    UsdImagingRootOverridesSceneIndexRefPtr _rootOverridesSceneIndex;
    HdsiLegacyDisplayStyleOverrideSceneIndexRefPtr _displayStyleSceneIndex;
    HdsiPrimTypePruningSceneIndexRefPtr _materialPruningSceneIndex;
    HdsiPrimTypePruningSceneIndexRefPtr _lightPruningSceneIndex;
    HdSceneIndexBaseRefPtr _sceneIndex;
    
    std::unique_ptr<UsdImagingDelegate> _sceneDelegate;

    std::unique_ptr<HdEngine> _engine;

    bool _allowAsynchronousSceneProcessing = false;
    bool _enableUsdDrawModes = true;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_GL_ENGINE_H
