//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_HGI_H
#define PXR_IMAGING_HGI_HGI_H

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/blitCmds.h"
#include "pxr/imaging/hgi/buffer.h"
#include "pxr/imaging/hgi/computeCmds.h"
#include "pxr/imaging/hgi/computeCmdsDesc.h"
#include "pxr/imaging/hgi/graphicsCmds.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgi/graphicsPipeline.h"
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgi/sampler.h"
#include "pxr/imaging/hgi/shaderFunction.h"
#include "pxr/imaging/hgi/shaderProgram.h"
#include "pxr/imaging/hgi/texture.h"
#include "pxr/imaging/hgi/types.h"

#include <atomic>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HgiCapabilities;
class HgiIndirectCommandEncoder;

using HgiUniquePtr = std::unique_ptr<class Hgi>;


/// \class Hgi
///
/// Hydra Graphics Interface.
/// Hgi is used to communicate with one or more physical gpu devices.
///
/// Hgi provides API to create/destroy resources that a gpu device owns.
/// The lifetime of resources is not managed by Hgi, so it is up to the caller
/// to destroy resources and ensure those resources are no longer used.
///
/// Commands are recorded in 'HgiCmds' objects and submitted via Hgi.
///
/// Thread-safety:
///
/// Modern graphics APIs like Metal and Vulkan are designed with multi-threading
/// in mind. We want to try and take advantage of this where possible.
/// However we also wish to continue to support OpenGL for the time being.
///
/// In an application where OpenGL is involved, when we say "main thread" we 
/// mean the thread on which the gl-context is bound.
///
/// Each Hgi backend should at minimum support the following:
///
/// * Single threaded Hgi::SubmitCmds on main thread.
/// * Single threaded Hgi::Resource Create*** / Destroy*** on main thread.
/// * Multi threaded recording of commands in Hgi***Cmds objects.
/// * A Hgi***Cmds object should be creatable on the main thread, recorded
///   into with one secondary thread (only one thread may use a Cmds object) and
///   submitted via the main thread.
///
/// Each Hgi backend is additionally encouraged to support:
///
/// * Multi threaded support for resource creation and destruction.
///
/// We currently do not rely on these additional multi-threading features in
/// Hydra / Storm where we still wish to run OpenGL. In Hydra we make sure to
/// use the main-thread for resource creation and command submission.
/// One day we may wish to switch this to be multi-threaded so new Hgi backends
/// are encouraged to support it.
///
/// Pseudo code what should minimally be supported:
///
///     vector<HgiGraphicsCmds> cmds
///
///     for num_threads
///         cmds.push_back( Hgi->CreateGraphicsCmds() )
///
///     parallel_for i to num_threads
///         cmds[i]->SetViewport()
///         cmds[i]->Draw()
///
///     for i to num_threads
///         hgi->SubmitCmds( cmds[i] )
///
class Hgi
{
public:
    HGI_API
    Hgi();

    HGI_API
    virtual ~Hgi();

    /// Submit one HgiCmds objects.
    /// Once the cmds object is submitted it cannot be re-used to record cmds.
    /// A call to SubmitCmds would usually result in the hgi backend submitting
    /// the cmd buffers of the cmds object(s) to the device queue.
    /// Derived classes can override _SubmitCmds to customize submission.
    /// Thread safety: This call is not thread-safe. Submission must happen on
    /// the main thread so we can continue to support the OpenGL platform. 
    /// See notes above.
    HGI_API
    void SubmitCmds(
        HgiCmds* cmds, 
        HgiSubmitWaitType wait = HgiSubmitWaitTypeNoWait);

    /// *** DEPRECATED *** Please use: CreatePlatformDefaultHgi
    HGI_API
    static Hgi* GetPlatformDefaultHgi();

    /// Helper function to return a Hgi object for the current platform.
    /// For example on Linux this may return HgiGL while on macOS HgiMetal.
    /// Caller, usually the application, owns the lifetime of the Hgi object and
    /// the object is destroyed when the caller drops the unique ptr.
    /// Thread safety: Not thread safe.
    HGI_API
    static HgiUniquePtr CreatePlatformDefaultHgi();

    /// Helper function to return a Hgi object of choice supported by current 
    /// platform and build configuration.
    /// For example, on macOS, this may allow HgiMetal only.
    /// If the Hgi device specified is not available on the current platform,
    /// this function will fail and return nullptr. 
    /// If an empty token is provided, the default Hgi type (see
    /// CreatePlatformDefaultHgi) will be created.
    /// Supported TfToken values are OpenGL, Metal, Vulkan, or an empty token;
    /// if not using an empty token, the caller is expected to use a token from 
    /// HgiTokens.
    /// Caller, usually the application, owns the lifetime of the Hgi object and
    /// the object is destroyed when the caller drops the unique ptr.
    /// Thread safety: Not thread safe.
    HGI_API
    static HgiUniquePtr CreateNamedHgi(const TfToken& hgiToken);

    /// Determine if Hgi instance can run on current hardware.
    /// Thread safety: This call is thread safe.
    HGI_API
    virtual bool IsBackendSupported() const = 0;

    /// Constructs a temporary Hgi object and calls the object's 
    /// IsBackendSupported() function.
    /// A token can optionally be provided to specify a specific Hgi backend to 
    /// create. Supported TfToken values are OpenGL, Metal, Vulkan, or an empty 
    /// token; if not using an empty token, the caller is expected to use a 
    /// token from HgiTokens. 
    /// An empty token will check support for creating the platform default Hgi.
    /// An invalid token will result in this function returning false.
    /// Thread safety: Not thread safe.
    HGI_API
    static bool IsSupported(const TfToken& hgiToken = TfToken());

    /// Returns a GraphicsCmds object (for temporary use) that is ready to
    /// record draw commands. GraphicsCmds is a lightweight object that
    /// should be re-acquired each frame (don't hold onto it after EndEncoding).
    /// Thread safety: Each Hgi backend must ensure that a Cmds object can be
    /// created on the main thread, recorded into (exclusively) by one secondary
    /// thread and be submitted on the main thread. See notes above.
    HGI_API
    virtual HgiGraphicsCmdsUniquePtr CreateGraphicsCmds(
        HgiGraphicsCmdsDesc const& desc) = 0;

    /// Returns a BlitCmds object (for temporary use) that is ready to execute
    /// resource copy commands. BlitCmds is a lightweight object that
    /// should be re-acquired each frame (don't hold onto it after EndEncoding).
    /// Thread safety: Each Hgi backend must ensure that a Cmds object can be
    /// created on the main thread, recorded into (exclusively) by one secondary
    /// thread and be submitted on the main thread. See notes above.
    HGI_API
    virtual HgiBlitCmdsUniquePtr CreateBlitCmds() = 0;

    /// Returns a ComputeCmds object (for temporary use) that is ready to
    /// record dispatch commands. ComputeCmds is a lightweight object that
    /// should be re-acquired each frame (don't hold onto it after EndEncoding).
    /// Thread safety: Each Hgi backend must ensure that a Cmds object can be
    /// created on the main thread, recorded into (exclusively) by one secondary
    /// thread and be submitted on the main thread. See notes above.
    HGI_API
    virtual HgiComputeCmdsUniquePtr CreateComputeCmds(
        HgiComputeCmdsDesc const& desc) = 0;

    /// Create a texture in rendering backend.
    /// Thread safety: Creation must happen on main thread. See notes above.
    HGI_API
    virtual HgiTextureHandle CreateTexture(HgiTextureDesc const & desc) = 0;

    /// Destroy a texture in rendering backend.
    /// Thread safety: Destruction must happen on main thread. See notes above.
    HGI_API
    virtual void DestroyTexture(HgiTextureHandle* texHandle) = 0;

    /// Create a texture view in rendering backend.
    /// A texture view aliases another texture's data.
    /// It is the responsibility of the client to ensure that the sourceTexture
    /// is not destroyed while the texture view is in use.
    /// Thread safety: Creation must happen on main thread. See notes above.
    HGI_API
    virtual HgiTextureViewHandle CreateTextureView(
        HgiTextureViewDesc const & desc) = 0;

    /// Destroy a texture view in rendering backend.
    /// This will destroy the view's texture, but not the sourceTexture that
    /// was aliased by the view. The sourceTexture data remains unchanged.
    /// Thread safety: Destruction must happen on main thread. See notes above.
    HGI_API
    virtual void DestroyTextureView(HgiTextureViewHandle* viewHandle) = 0;

    /// Create a sampler in rendering backend.
    /// Thread safety: Creation must happen on main thread. See notes above.
    HGI_API
    virtual HgiSamplerHandle CreateSampler(HgiSamplerDesc const & desc) = 0;

    /// Destroy a sampler in rendering backend.
    /// Thread safety: Destruction must happen on main thread. See notes above.
    HGI_API
    virtual void DestroySampler(HgiSamplerHandle* smpHandle) = 0;

    /// Create a buffer in rendering backend.
    /// Thread safety: Creation must happen on main thread. See notes above.
    HGI_API
    virtual HgiBufferHandle CreateBuffer(HgiBufferDesc const & desc) = 0;

    /// Destroy a buffer in rendering backend.
    /// Thread safety: Destruction must happen on main thread. See notes above.
    HGI_API
    virtual void DestroyBuffer(HgiBufferHandle* bufHandle) = 0;

    /// Create a new shader function.
    /// Thread safety: Creation must happen on main thread. See notes above.
    HGI_API
    virtual HgiShaderFunctionHandle CreateShaderFunction(
        HgiShaderFunctionDesc const& desc) = 0;

    /// Destroy a shader function.
    /// Thread safety: Destruction must happen on main thread. See notes above.
    HGI_API
    virtual void DestroyShaderFunction(
        HgiShaderFunctionHandle* shaderFunctionHandle) = 0;

    /// Create a new shader program.
    /// Thread safety: Creation must happen on main thread. See notes above.
    HGI_API
    virtual HgiShaderProgramHandle CreateShaderProgram(
        HgiShaderProgramDesc const& desc) = 0;

    /// Destroy a shader program.
    /// Note that this does NOT automatically destroy the shader functions in
    /// the program since shader functions may be used by more than one program.
    /// Thread safety: Destruction must happen on main thread. See notes above.
    HGI_API
    virtual void DestroyShaderProgram(
        HgiShaderProgramHandle* shaderProgramHandle) = 0;

    /// Create a new resource binding object.
    /// Thread safety: Creation must happen on main thread. See notes above.
    HGI_API
    virtual HgiResourceBindingsHandle CreateResourceBindings(
        HgiResourceBindingsDesc const& desc) = 0;

    /// Destroy a resource binding object.
    /// Thread safety: Destruction must happen on main thread. See notes above.
    HGI_API
    virtual void DestroyResourceBindings(
        HgiResourceBindingsHandle* resHandle) = 0;

    /// Create a new graphics pipeline state object.
    /// Thread safety: Creation must happen on main thread. See notes above.
    HGI_API
    virtual HgiGraphicsPipelineHandle CreateGraphicsPipeline(
        HgiGraphicsPipelineDesc const& pipeDesc) = 0;

    /// Destroy a graphics pipeline state object.
    /// Thread safety: Destruction must happen on main thread. See notes above.
    HGI_API
    virtual void DestroyGraphicsPipeline(
        HgiGraphicsPipelineHandle* pipeHandle) = 0;

    /// Create a new compute pipeline state object.
    /// Thread safety: Creation must happen on main thread. See notes above.
    HGI_API
    virtual HgiComputePipelineHandle CreateComputePipeline(
        HgiComputePipelineDesc const& pipeDesc) = 0;

    /// Destroy a compute pipeline state object.
    /// Thread safety: Destruction must happen on main thread. See notes above.
    HGI_API
    virtual void DestroyComputePipeline(HgiComputePipelineHandle* pipeHandle)=0;

    /// Return the name of the api (e.g. "OpenGL").
    /// Thread safety: This call is thread safe.
    HGI_API
    virtual TfToken const& GetAPIName() const = 0;

    /// Returns the device-specific capabilities structure.
    /// Thread safety: This call is thread safe.
    HGI_API
    virtual HgiCapabilities const* GetCapabilities() const = 0;

    /// Returns the device-specific indirect command buffer encoder
    /// or nullptr if not supported.
    /// Thread safety: This call is thread safe.
    HGI_API
    virtual HgiIndirectCommandEncoder* GetIndirectCommandEncoder() const = 0;

    /// Optionally called by client app at the start of a new rendering frame.
    /// We can't rely on StartFrame for anything important, because it is up to
    /// the external client to (optionally) call this and they may never do.
    /// Hydra doesn't have a clearly defined start or end frame.
    /// This can be helpful to insert GPU frame debug markers.
    /// Thread safety: Not thread safe. Should be called on the main thread.
    HGI_API
    virtual void StartFrame() = 0;

    /// Optionally called at the end of a rendering frame.
    /// Please read the comments in StartFrame.
    /// Thread safety: Not thread safe. Should be called on the main thread.
    HGI_API
    virtual void EndFrame() = 0;

protected:
    // Returns a unique id for handle creation.
    // Thread safety: Thread-safe atomic increment.
    HGI_API
    uint64_t GetUniqueId();

    // Calls Submit on provided Cmds.
    // Derived classes can override this function if they need customize the
    // command submission. The default implementation calls cmds->_Submit().
    HGI_API
    virtual bool _SubmitCmds(
        HgiCmds* cmds, HgiSubmitWaitType wait);

private:
    Hgi & operator=(const Hgi&) = delete;
    Hgi(const Hgi&) = delete;

    std::atomic<uint64_t> _uniqueIdCounter;
};


///
/// Hgi factory for plugin system
///
class HgiFactoryBase : public TfType::FactoryBase {
public:
    virtual Hgi* New() const = 0;
};

template <class T>
class HgiFactory : public HgiFactoryBase {
public:
    Hgi* New() const {
        return new T;
    }
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
