//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_PLUGIN_HD_EMBREE_RENDER_DELEGATE_H
#define PXR_IMAGING_PLUGIN_HD_EMBREE_RENDER_DELEGATE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderThread.h"
#include "pxr/imaging/plugin/hdEmbree/renderer.h"
#include "pxr/base/tf/staticTokens.h"

#include <mutex>
#include <embree3/rtcore.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdEmbreeRenderParam;

#define HDEMBREE_RENDER_SETTINGS_TOKENS \
    (enableAmbientOcclusion)            \
    (enableSceneColors)                 \
    (ambientOcclusionSamples)           \
    (randomNumberSeed)

// Also: HdRenderSettingsTokens->convergedSamplesPerPixel

TF_DECLARE_PUBLIC_TOKENS(HdEmbreeRenderSettingsTokens, HDEMBREE_RENDER_SETTINGS_TOKENS);

///
/// \class HdEmbreeRenderDelegate
///
/// Render delegates provide renderer-specific functionality to the render
/// index, the main hydra state management structure. The render index uses
/// the render delegate to create and delete scene primitives, which include
/// geometry and also non-drawable objects. The render delegate is also
/// responsible for creating renderpasses, which know how to draw this
/// renderer's scene primitives.
///
/// Primitives in Hydra are split into Rprims (drawables), Sprims (state
/// objects like cameras and materials), and Bprims (buffer objects like
/// textures). The minimum set of primitives a renderer needs to support is
/// one Rprim (so the scene's not empty) and the "camera" Sprim, which is
/// required by HdxRenderTask, the task implementing basic hydra drawing.
///
/// A render delegate can report which prim types it supports via
/// GetSupportedRprimTypes() (and Sprim, Bprim), and well-behaved applications
/// won't call CreateRprim() (Sprim, Bprim) for prim types that aren't
/// supported. The core hydra prim types are "mesh", "basisCurves", and
/// "points", but a custom render delegate and a custom scene delegate could
/// add support for other prims such as implicit surfaces or volumes.
///
/// HdEmbree Rprims create embree geometry objects in the render delegate's
/// top-level embree scene; and HdEmbree's render pass draws by casting rays
/// into the top-level scene. The renderpass writes to renderbuffers, which
/// are then composited into the GL framebuffer.
///
/// The render delegate also has a hook for the main hydra execution algorithm
/// (HdEngine::Execute()): between HdRenderIndex::SyncAll(), which pulls new
/// scene data, and execution of tasks, the engine calls back to
/// CommitResources(). This can be used to commit GPU buffers, or as a place to
/// schedule a final BVH build (though Embree doesn't currenlty use it).
/// Importantly, no scene updates are processed after CommitResources().
///
class HdEmbreeRenderDelegate final : public HdRenderDelegate
{
public:
    /// Render delegate constructor. This method creates the RTC device and
    /// scene, and links embree error handling to hydra error handling.
    HdEmbreeRenderDelegate();
    /// Render delegate constructor. This method creates the RTC device and
    /// scene, and links embree error ahndling to hydra error handling.
    /// It also populates initial render settings.
    HdEmbreeRenderDelegate(HdRenderSettingsMap const& settingsMap);
    /// Render delegate destructor. This method destroys the RTC device and
    /// scene.
    ~HdEmbreeRenderDelegate() override;

    /// Return this delegate's render param.
    ///   \return A shared instance of HdEmbreeRenderParam.
    HdRenderParam *GetRenderParam() const override;

    /// Return a list of which Rprim types can be created by this class's
    /// CreateRprim.
    const TfTokenVector &GetSupportedRprimTypes() const override;
    /// Return a list of which Sprim types can be created by this class's
    /// CreateSprim.
    const TfTokenVector &GetSupportedSprimTypes() const override;
    /// Return a list of which Bprim types can be created by this class's
    /// CreateBprim.
    const TfTokenVector &GetSupportedBprimTypes() const override;

    /// Returns the HdResourceRegistry instance used by this render delegate.
    HdResourceRegistrySharedPtr GetResourceRegistry() const override;

    /// Returns a list of user-configurable render settings.
    /// This is a reflection API for the render settings dictionary; it need
    /// not be exhaustive, but can be used for populating application settings
    /// UI.
    HdRenderSettingDescriptorList
        GetRenderSettingDescriptors() const override;

    /// Return true to indicate that pausing and resuming are supported.
    bool IsPauseSupported() const override;

    /// Pause background rendering threads.
    bool Pause() override;

    /// Resume background rendering threads.
    bool Resume() override;

    /// Create a renderpass. Hydra renderpasses are responsible for drawing
    /// a subset of the scene (specified by the "collection" parameter) to the
    /// current framebuffer. This class creates objects of type
    /// HdEmbreeRenderPass, which draw using embree's raycasting API.
    ///   \param index The render index this renderpass will be bound to.
    ///   \param collection A specifier for which parts of the scene should
    ///                     be drawn.
    ///   \return An embree renderpass object.
    HdRenderPassSharedPtr CreateRenderPass(HdRenderIndex *index,
                HdRprimCollection const& collection) override;

    /// Create an instancer. Hydra instancers store data needed for an
    /// instanced object to draw itself multiple times.
    ///   \param delegate The scene delegate providing data for this
    ///                   instancer.
    ///   \param id The scene graph ID of this instancer, used when pulling
    ///             data from a scene delegate.
    ///   \return An embree instancer object.
    HdInstancer *CreateInstancer(HdSceneDelegate *delegate,
                                 SdfPath const& id) override;

    /// Destroy an instancer created with CreateInstancer.
    ///   \param instancer The instancer to be destroyed.
    void DestroyInstancer(HdInstancer *instancer) override;

    /// Create a hydra Rprim, representing scene geometry. This class creates
    /// embree-specialized geometry containers like HdEmbreeMesh which map
    /// scene data to embree scene graph objects.
    ///   \param typeId The rprim type to create. This must be one of the types
    ///                 from GetSupportedRprimTypes().
    ///   \param rprimId The scene graph ID of this rprim, used when pulling
    ///                  data from a scene delegate.
    ///   \return An embree rprim object.
    HdRprim *CreateRprim(TfToken const& typeId,
                         SdfPath const& rprimId) override;

    /// Destroy an Rprim created with CreateRprim.
    ///   \param rPrim The rprim to be destroyed.
    void DestroyRprim(HdRprim *rPrim) override;

    /// Create a hydra Sprim, representing scene or viewport state like cameras
    /// or lights.
    ///   \param typeId The sprim type to create. This must be one of the types
    ///                 from GetSupportedSprimTypes().
    ///   \param sprimId The scene graph ID of this sprim, used when pulling
    ///                  data from a scene delegate.
    ///   \return An embree sprim object.
    HdSprim *CreateSprim(TfToken const& typeId,
                         SdfPath const& sprimId) override;

    /// Create a hydra Sprim using default values, and with no scene graph
    /// binding.
    ///   \param typeId The sprim type to create. This must be one of the types
    ///                 from GetSupportedSprimTypes().
    ///   \return An embree fallback sprim object.
    HdSprim *CreateFallbackSprim(TfToken const& typeId) override;

    /// Destroy an Sprim created with CreateSprim or CreateFallbackSprim.
    ///   \param sPrim The sprim to be destroyed.
    void DestroySprim(HdSprim *sPrim) override;

    /// Create a hydra Bprim, representing data buffers such as textures.
    ///   \param typeId The bprim type to create. This must be one of the types
    ///                 from GetSupportedBprimTypes().
    ///   \param bprimId The scene graph ID of this bprim, used when pulling
    ///                  data from a scene delegate.
    ///   \return An embree bprim object.
    HdBprim *CreateBprim(TfToken const& typeId,
                         SdfPath const& bprimId) override;

    /// Create a hydra Bprim using default values, and with no scene graph
    /// binding.
    ///   \param typeId The bprim type to create. This must be one of the types
    ///                 from GetSupportedBprimTypes().
    ///   \return An embree fallback bprim object.
    HdBprim *CreateFallbackBprim(TfToken const& typeId) override;

    /// Destroy a Bprim created with CreateBprim or CreateFallbackBprim.
    ///   \param bPrim The bprim to be destroyed.
    void DestroyBprim(HdBprim *bPrim) override;

    /// This function is called after new scene data is pulled during prim
    /// Sync(), but before any tasks (such as draw tasks) are run, and gives the
    /// render delegate a chance to transfer any invalidated resources to the
    /// rendering kernel.
    ///   \param tracker The change tracker passed to prim Sync().
    void CommitResources(HdChangeTracker *tracker) override;

    /// This function tells the scene which material variant to reference.
    /// Embree doesn't currently use materials but raytraced backends generally
    /// specify "full".
    ///   \return A token specifying which material variant this renderer
    ///           prefers.
    TfToken GetMaterialBindingPurpose() const override {
        return HdTokens->full;
    }

    /// This function returns the default AOV descriptor for a given named AOV.
    /// This mechanism lets the renderer decide things like what format
    /// a given AOV will be written as.
    ///   \param name The name of the AOV whose descriptor we want.
    ///   \return A descriptor specifying things like what format the AOV
    ///           output buffer should be.
    HdAovDescriptor
        GetDefaultAovDescriptor(TfToken const& name) const override;

    /// This function allows the renderer to report back some useful statistics
    /// that the application can display to the user.
    VtDictionary GetRenderStats() const override;

private:
    static const TfTokenVector SUPPORTED_RPRIM_TYPES;
    static const TfTokenVector SUPPORTED_SPRIM_TYPES;
    static const TfTokenVector SUPPORTED_BPRIM_TYPES;

    /// Resource registry used in this render delegate
    static std::mutex _mutexResourceRegistry;
    static std::atomic_int _counterResourceRegistry;
    static HdResourceRegistrySharedPtr _resourceRegistry;

    // This class does not support copying.
    HdEmbreeRenderDelegate(const HdEmbreeRenderDelegate &)             = delete;
    HdEmbreeRenderDelegate &operator =(const HdEmbreeRenderDelegate &) = delete;

    // Embree initialization routine.
    void _Initialize();

    // Handle for an embree "device", or library state.
    RTCDevice _rtcDevice;

    // Handle for the top-level embree scene, mirroring the Hydra scene.
    RTCScene _rtcScene;

    // A version counter for edits to _scene.
    std::atomic<int> _sceneVersion;

    // A shared HdEmbreeRenderParam object that stores top-level embree state;
    // passed to prims during Sync().
    std::shared_ptr<HdEmbreeRenderParam> _renderParam;

    // A background render thread for running the actual renders in. The
    // render thread object manages synchronization between the scene data
    // and the background-threaded renderer.
    HdRenderThread _renderThread;

    // An embree renderer object, to perform the actual raytracing.
    HdEmbreeRenderer _renderer;

    // A list of render setting exports.
    HdRenderSettingDescriptorList _settingDescriptors;

    // A callback that interprets embree error codes and injects them into
    // the hydra logging system.
    static void HandleRtcError(void* userPtr, RTCError code, const char *msg);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_HD_EMBREE_RENDER_DELEGATE_H
