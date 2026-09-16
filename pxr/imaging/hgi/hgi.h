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
#include "pxr/imaging/hgi/version.h"

#include <atomic>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HgiCapabilities;
class HgiIndirectCommandEncoder;

using HgiUniquePtr = std::unique_ptr<class Hgi>;

/// \enum HgiExternalHandleType
///
/// How an OS-shareable handle to external memory or an external semaphore
/// should be interpreted. The kind is never inferred from the handle value --
/// a Win32 NT handle and a POSIX fd are both small integers.
enum HgiExternalHandleType
{
    HgiExternalHandleTypeOpaqueWin32 = 0,
    HgiExternalHandleTypeOpaqueFd,
};

/// \enum HgiSemaphoreKind
///
/// Flavour of an external semaphore. OpenGL can only import binary
/// semaphores, so binary is the interop lowest common denominator.
enum HgiSemaphoreKind
{
    HgiSemaphoreKindBinary = 0,
    HgiSemaphoreKindTimeline,
};

/// \struct HgiInteropBufferInfo
///
/// Describes an interop-allocated buffer's external memory so another GPU API
/// (e.g. OpenGL via GL_EXT_memory_object) can import and alias the same memory.
/// Returned by Hgi::CreateInteropBuffer.
struct HgiInteropBufferInfo
{
    // OS-shareable handle to the memory (a Win32 NT handle cast to uint64, or an
    // fd). 0 if interop is unsupported. The importer owns/closes it per platform
    // convention (fd: importer takes ownership; Win32: caller should close).
    uint64_t externalHandle = 0;
    // Total size of the memory block the handle refers to, and the buffer's
    // offset within it -- both required to import the memory in the other API.
    size_t   memoryBlockSize = 0;
    size_t   memoryOffset = 0;
    // Whether the allocation is a dedicated memory object.
    bool     dedicated = false;
};

/// \struct HgiExternalMemoryBufferDesc
///
/// Describes a foreign memory allocation to import and wrap in a buffer of this
/// backend, so the backend can read memory another device allocated. The
/// producer-side counterpart is HgiInteropBufferInfo.
struct HgiExternalMemoryBufferDesc
{
    // OS-shareable handle naming the memory ALLOCATION (not a buffer object).
    // The caller retains ownership on Win32; on Linux the import takes over the
    // fd, so pass a dup() if the handle is needed again.
    uint64_t externalHandle = 0;
    // How to interpret externalHandle. Callers must set this explicitly.
    HgiExternalHandleType handleType = HgiExternalHandleTypeOpaqueWin32;
    // Size of the whole memory block the handle refers to. The import allocates
    // the full block even when only byteSize of it is used here.
    size_t memoryBlockSize = 0;
    // Offset of the buffer within that block.
    size_t memoryOffset = 0;
    // Size of the buffer to create at memoryOffset.
    size_t byteSize = 0;
    // Must match how the producer allocated the memory or the import fails.
    bool dedicated = false;
    HgiBufferUsage usage = 0;
    std::string debugName;
};


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
    HgiTextureHandle CreateTexture(HgiTextureDesc const & desc);

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
    HgiTextureViewHandle CreateTextureView(
        HgiTextureViewDesc const & desc);

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
    HgiBufferHandle CreateBuffer(HgiBufferDesc const & desc);

    /// Wrap an externally-owned native GPU buffer in a NON-OWNING HgiBuffer so
    /// this backend can bind it without copying. \p rawHandle is interpreted per
    /// backend (a GL buffer name, a VkBuffer, ...). The returned handle must NOT
    /// free the underlying native resource when destroyed via DestroyBuffer.
    /// Backends that cannot adopt a foreign handle return an empty handle; the
    /// caller then falls back to its own generic wrapper. Default: empty handle.
    HGI_API
    virtual HgiBufferHandle CreateExternalBuffer(
        uint64_t rawHandle, size_t byteSize, HgiBufferUsage usage);

    /// Allocate a buffer whose memory is EXPORTABLE to other GPU APIs (interop).
    /// Returns a real, owning HgiBuffer and, in \p outInfo, the external-memory
    /// description another API can import to alias the same memory. Backends
    /// without interop support return an empty handle and leave \p outInfo at
    /// its defaults. Default: unsupported.
    HGI_API
    virtual HgiBufferHandle CreateInteropBuffer(
        size_t byteSize, HgiBufferUsage usage, HgiInteropBufferInfo* outInfo);

    /// IMPORT memory another device (or API) allocated as exportable, and wrap
    /// it in a real, OWNING buffer of this backend: the returned handle owns the
    /// buffer object and the imported memory reference, but not the underlying
    /// allocation, which stays alive as long as any importer holds it. Use this
    /// when the producer's memory does not live on this backend's device, so
    /// CreateExternalBuffer's adopt path is not available. Backends that cannot
    /// import return an empty handle. Default: unsupported.
    HGI_API
    virtual HgiBufferHandle CreateBufferFromExternalMemory(
        HgiExternalMemoryBufferDesc const& desc);

    /// IMPORT a semaphore created by another device (or API) from its OS handle,
    /// returning a backend-native handle usable with QueueWait/QueueSignal and
    /// DestroyExternalSemaphore, or 0 if unsupported. This is the cross-device
    /// counterpart of CreateExternalSemaphore, which creates one locally.
    /// \p externalHandle ownership follows the platform convention (fd: taken;
    /// Win32: retained by the caller). Default: unsupported.
    HGI_API
    virtual uint64_t ImportExternalSemaphore(
        uint64_t externalHandle,
        HgiExternalHandleType handleType,
        HgiSemaphoreKind kind);

    /// Returns the physical device this backend renders on, as a 32-character
    /// lowercase hex encoding of its 16-byte UUID; empty when the backend has no
    /// such identity. Opaque external handles are only importable on the device
    /// that exported them, so producers and consumers compare this to decide
    /// between adopting a native handle, importing, and giving up.
    HGI_API
    virtual std::string GetDeviceUuid() const;

    /// Returns an id for the LOGICAL device this backend renders through --
    /// unique within this process, 0 when the backend has no such object. Where
    /// GetDeviceUuid names the GPU, this names the device object whose handle
    /// namespace a native resource handle belongs to; the two differ when two
    /// logical devices drive one GPU, and that is exactly when adopting a
    /// foreign native handle would bind an unrelated object. Backends without a
    /// logical device (GL, whose namespace is the context share group) return 0,
    /// which callers treat as "unknown". Default: 0.
    HGI_API
    virtual uint64_t GetLogicalDeviceId() const;

    /// Create a binary semaphore whose signal state is EXPORTABLE to other GPU
    /// APIs. Returns a backend-native handle (0 if unsupported) and, in
    /// \p outExternalHandle, an OS handle (Win32 NT handle / fd) the other API
    /// imports. Used to order a producer's writes in another API against this
    /// backend's reads of a shared buffer. Default: unsupported.
    HGI_API
    virtual uint64_t CreateExternalSemaphore(uint64_t* outExternalHandle);

    /// Destroy a semaphore returned by CreateExternalSemaphore.
    HGI_API
    virtual void DestroyExternalSemaphore(uint64_t semaphore);

    /// Make this backend's NEXT queue submission wait on \p semaphore (a native
    /// handle from CreateExternalSemaphore) before its commands execute, so a
    /// draw does not read a buffer the producer has not finished writing (RAW).
    /// Default: no-op.
    HGI_API
    virtual void QueueWaitExternalSemaphore(uint64_t semaphore);

    /// Make this backend's NEXT queue submission signal \p semaphore after its
    /// commands complete, so a producer in another API may wait on it before
    /// overwriting a shared buffer this backend just read (WAR). Default: no-op.
    HGI_API
    virtual void QueueSignalExternalSemaphore(uint64_t semaphore);

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
    HgiResourceBindingsHandle CreateResourceBindings(
        HgiResourceBindingsDesc const& desc);

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

    /// Perform any necessary garbage collection, if applicable. This can be
    /// used to flush pending deletes immediately after unloading assets, for
    /// example. Note that as some clients may not call this, Hgi
    /// implementations should find other opportunities to garbage collect as
    /// well (e.g. EndFrame).
    HGI_API
    virtual void GarbageCollect() = 0;

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

    HGI_API
    virtual HgiTextureHandle _CreateTexture(HgiTextureDesc const & desc) = 0;

    HGI_API
    virtual HgiTextureViewHandle _CreateTextureView(
        HgiTextureViewDesc const & desc) = 0;

    HGI_API
    virtual HgiBufferHandle _CreateBuffer(HgiBufferDesc const & desc) = 0;

    HGI_API
    virtual HgiResourceBindingsHandle _CreateResourceBindings(
        HgiResourceBindingsDesc const& desc) = 0;

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
