//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

// Verifies that Storm consumes an externally-owned GPU buffer published on a
// primvar via HdExtGpuBufferSchema (direct-bind / zero-copy) and renders it
// identically to the ordinary CPU-primvar path.
//
// Two scene workflows, selected by --instancing:
//   * basic (default): a single non-instanced cube whose *points* primvar is
//     backed by an external GPU buffer.
//   * --instancing: a grid of instanced cubes where BOTH the prototype *points*
//     and the instancer's per-instance *transforms* are external GPU buffers.
//     This is the two-axis instancing case (prototype primvar + instancer
//     primvar).
//
// Three producer topologies, orthogonal to the scene workflow:
//   * default: the application creates its own GL buffer and REGISTERS it with
//     the consumer's GL arena, which binds it and never deletes it. This is
//     the shape a GL-based viewport sharing buffers with Storm actually has,
//     and the arena needs no semaphores because one context orders the two.
//   * --vulkanSync: a Vulkan producer allocates on the CONSUMER's own device
//     and writes it as its own work, in an independent command buffer it
//     submits itself. The buffer is registered rather than imported -- same
//     logical device -- but the write and the read are now separate
//     submissions, ordered by the arena's semaphore pair: the producer signals
//     app-done, Storm's commit waits on it and signals hgi-done.
//   * --vulkanInterop: a SECOND Vulkan device stands in for a producer with its
//     own device. It allocates exportable memory out of its own arena; the
//     consumer's GL arena imports that allocation, and the producer writes the
//     geometry through the imported GL buffer. Nothing native is shared: a
//     VkBuffer from another logical device would mean nothing here, which is
//     why the arena is keyed on the producing device in the first place.
//
// Orthogonal to the producer topology, --copy publishes allowDirectBind=false,
// which flips the consumption strategy from zero-copy direct bind to a
// GPU-to-GPU blit into Storm's aggregated vertex buffer. Combined with
// --vulkanInterop it is the interesting case: the buffer is IMPORTED from a
// foreign logical device and then blitted rather than aliased, exercising the
// import-then-blit path that neither the default nor the direct-bind interop
// test covers.
//
// Lifetime, which the test deliberately models the way a real producer must:
// the scene description carries only a WEAK reference to each shared buffer, so
// the test holds the strong one for the duration of the run. Buffers the
// application created itself are deleted by the application; buffers the arena
// allocated or imported are reclaimed by the arena, once nothing references
// them and the GPU has retired the work that named them.
//
// The geometry is published through a HdRetainedSceneIndex inserted into the
// render index, because the schema lives as a data-source *child* of the
// primvar -- the legacy HdUnitTestDelegate emulation path would not carry it.
//
// Correctness check (self-comparing, no baseline image required): the test
// renders the SAME scene twice in one run -- once with ordinary CPU primvars,
// once with the external GPU buffers -- reads back both color images, and
// asserts they are pixel-identical. Because both images come from the same GPU
// in the same run, a correct GPU path is bit-for-bit equal to the CPU path; a
// wrong or silently-falling-back GPU path diverges and fails the test. This
// avoids committing a driver-specific baseline PNG.

#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/hd/extentSchema.h"
#include "pxr/imaging/hd/extGpuBufferSchema.h"
#include "pxr/imaging/hd/externalBuffer.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/meshTopologySchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/retainedSceneIndex.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hd/xformSchema.h"

#include "pxr/imaging/hgi/buffer.h"
#include "pxr/imaging/hgi/externalBuffer.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/imaging/hgiGL/externalBuffer.h"
#include "pxr/imaging/hgiGL/externalBufferArena.h"

// The --vulkanSync producer records and submits its own command buffer on the
// consumer's Vulkan device, which needs the concrete HgiVulkan types (there is
// no Hgi-level API for recording a raw producer submission). Only compiled into
// Vulkan-enabled builds; the --vulkanSync test is likewise only registered there.
#if defined(PXR_VULKAN_SUPPORT_ENABLED)
#include "pxr/imaging/hgiVulkan/buffer.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/externalBuffer.h"
#include "pxr/imaging/hgiVulkan/externalBufferArena.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiVulkan/semaphore.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"
#endif

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Cube prototype: 8 corners, 6 quad faces (matches the flow-viewport example).
const VtIntArray _faceVertexCounts  = {4, 4, 4, 4, 4, 4};
const VtIntArray _faceVertexIndices = {0, 1, 3, 2, 2, 3, 5, 4, 4, 5, 7, 6,
                                       6, 7, 1, 0, 1, 7, 5, 3, 6, 0, 2, 4};

VtVec3fArray
_CubePoints(float h)
{
    return VtVec3fArray{
        {-h, -h,  h}, { h, -h,  h}, {-h,  h,  h}, { h,  h,  h},
        {-h,  h, -h}, { h,  h, -h}, {-h, -h, -h}, { h, -h, -h}};
}

using _PointArrayDs = HdRetainedTypedSampledDataSource<VtVec3fArray>;
using _IntArrayDs   = HdRetainedTypedSampledDataSource<VtIntArray>;

// The AOV path the driver uses for color (see HdSt_TestDriverBase::_GetAovPath).
const SdfPath _colorAovId("/testDriver/aov_color");

// What a producer hands the consumer: the shared buffer itself. Everything the
// consumer used to be told separately -- native handle, OS memory handle, block
// size and offset, dedicated-ness, device UUID, logical device id, and a
// semaphore pair -- is now a detail of this object and of the arena that made
// it.
using _SharedBuffer = HgiExternalBufferSharedPtr;

// The external handle flavour this platform's interop uses. Never inferred
// from a handle value, which is why it travels alongside it.
HgiExternalHandleType
_PlatformHandleType()
{
#if defined(_WIN32)
    return HgiExternalHandleTypeOpaqueWin32;
#else
    return HgiExternalHandleTypeOpaqueFd;
#endif
}

// A stable key standing for the GL context the application shares buffers from.
// An arena is keyed on the producing device or context; this test has a single
// context for its whole run, so any stable value will do.
uint64_t
_GlContextKey()
{
    static const int marker = 0;
    return reinterpret_cast<uint64_t>(&marker);
}

} // anonymous namespace

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing
{
public:
    My_TestGLDrawing()
    {
        SetCameraRotate(30.0f, 30.0f);
    }

    void InitTest() override {}   // all work happens in OffscreenTest / DrawTest
    void UninitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;

protected:
    void ParseArgs(int argc, char *argv[]) override;

private:
    // Build the scene into `scene`. When `gpuShare` is true the cube points
    // (and, for the instancing workflow, the instance transforms) are published
    // as buffers shared through an arena; otherwise they are CPU primvars.
    void _BuildScene(HdSt_TestDriver *driver,
                     HdRetainedSceneIndexRefPtr &scene,
                     bool gpuShare);

    // Build the cube primvars container (points + constant displayColor).
    HdContainerDataSourceHandle _BuildCubePrimvars(
        HdSt_TestDriver *driver,
        bool gpuShare);

    // Produce a buffer holding `data` by whichever topology the flags select,
    // and return it. Null on failure, which the caller reports.
    _SharedBuffer _MakeGpuBuffer(HdSt_TestDriver *driver,
                                 const void *data, size_t byteSize,
                                 uint32_t stride);

    // Default topology: the application allocates on the CONSUMER's own device
    // and REGISTERS the buffer, so the arena binds it and never deletes it.
    // Which API that is follows the consumer's backend -- a native handle only
    // means something to the API that minted it.
    _SharedBuffer _MakeNativeRegisteredBuffer(HdSt_TestDriver *driver,
                                              const void *data,
                                              size_t byteSize,
                                              uint32_t stride);

    // --vulkanInterop: a second Vulkan device stands in for a producer with
    // its own device. It allocates exportable memory out of its own arena and
    // writes the geometry there, and the consumer imports that allocation
    // through whichever arena matches its own backend.
    _SharedBuffer _MakeInteropBuffer(HdSt_TestDriver *driver,
                                     const void *data, size_t byteSize);

    // --vulkanSync: the producer allocates on the CONSUMER's own Vulkan device
    // and writes it in its own independent submission, signalling the arena's
    // app-done semaphore. Storm registers the VkBuffer -- no import, the device
    // matches -- and its commit waits on that semaphore.
    _SharedBuffer _MakeVulkanSyncBuffer(HdSt_TestDriver *driver,
                                        const void *data, size_t byteSize,
                                        uint32_t stride);

    bool _ConsumerIsVulkan(HdSt_TestDriver *driver) const;

    // Check the arena's reclaim contract: the buffer outlives the last outside
    // reference, and only collection releases it. Called while the driver and
    // arena are still alive, so the arena's own reference is the only thing
    // the assertions can be observing.
    void _CheckArenaReclaim(HdSt_TestDriver *driver,
                            HdRetainedSceneIndexRefPtr const &scene);

#if defined(PXR_VULKAN_SUPPORT_ENABLED)
    // Have the producer create its exportable semaphore pair and the consumer
    // import it, once per run. Returns false when either side cannot, which
    // is not a failure -- the caller then orders the write on the host
    // instead.
    bool _ShareInteropSemaphores(
        HgiVulkanExternalBufferArena *producerArena,
        HgiExternalBufferArena *consumerArena);

    // WAR check: wait, on the GPU, for the hgi-done semaphore the consumer
    // signals when it has finished reading -- the point at which a real
    // producer would be free to overwrite the buffer. Bounded, because a
    // consumer that never signals would otherwise hang the test rather than
    // fail it.
    void _VerifyHgiDoneSignal();
#endif

#if defined(PXR_VULKAN_SUPPORT_ENABLED)
    // Get-or-create the Vulkan arena for \p hgi, cached in \p slot. Used for
    // both the consumer's device and the producer's.
    HgiVulkanExternalBufferArena *_GetVulkanArena(
        Hgi *hgi,
        std::shared_ptr<HgiVulkanExternalBufferArena> *slot);

    // Write \p data into \p dst as the producer's own work on \p hgi's
    // device, optionally signalling \p signalSemaphore when it completes.
    bool _VulkanUpload(Hgi *hgi, VkBuffer dst,
                       const void *data, size_t byteSize,
                       VkSemaphore signalSemaphore);
#endif

    HgiGLExternalBufferArena *_GetGlArena(HdSt_TestDriver *driver);

    // Create the second (producer) Vulkan device for --vulkanInterop
    // and verify it resolves to the same physical device as the consumer's.
    bool _EnsureProducerHgi(HdSt_TestDriver *driver);

    // Free the producer device's resources and then the device itself.
    void _ReleaseProducer();

    HdContainerDataSourceHandle _WithExtGpuBuffer(
        HdSt_TestDriver *driver,
        const HdContainerDataSourceHandle &primvar,
        const _SharedBuffer &shared, size_t byteSize,
        HdTupleType elementType, size_t numElements);

    // Render the scene once (fresh driver) and read the color AOV back into
    // `out`. Optionally also writes the image to `writePath`.
    void _RenderToPixels(bool gpuShare, std::vector<uint8_t> &out,
                         int &width, int &height,
                         const std::string &writePath);

    GfVec3f _CameraTranslate() const {
        // Frame the single cube up close, the grid pulled back.
        return _instancing ? GfVec3f(0.0f, 0.0f, -30.0f)
                           : GfVec3f(0.0f, 0.0f, -8.0f);
    }

    // Interactive-mode driver (created lazily; not used by --offscreen).
    std::unique_ptr<HdSt_TestDriver> _driver;
    HdRetainedSceneIndexRefPtr _driverScene;

    // Geometry parameters.
    bool _instancing = false;   // --instancing: grid of instanced cubes
    int _div = 3;               // grid is _div x _div cubes (instancing only)
    float _halfSize = 1.0f;
    float _spacing = 3.0f;

    // --vulkanSync: a Vulkan producer allocates the buffer on the CONSUMER's
    // own device and writes it in its own independent submission, signalling
    // the arena's app-done semaphore. Storm registers the VkBuffer -- nothing
    // is imported, the device matches -- and its commit waits on that
    // semaphore. Requires a Vulkan Hgi (run with HGI_ENABLE_VULKAN=1).
    bool _vulkanSync = false;

    // --vulkanInterop: the memory is allocated on a second Vulkan device
    // standing in for a producer that owns its own device, so the consumer has
    // to import the OS memory handle rather than share a native handle.
    bool _vulkanInterop = false;

    // --copy: publish allowDirectBind=false so the consumer copies (a GPU-to-GPU
    // blit) the shared buffer into its own aggregated VBO instead of binding it
    // zero-copy. With --vulkanInterop this drives the import-then-blit path.
    bool _allowDirectBind = true;

    // The consumer's arena for application-shared buffers, created on first
    // use. Held so the buffers registered in it stay reachable.
    std::shared_ptr<HgiGLExternalBufferArena> _glArena;
#if defined(PXR_VULKAN_SUPPORT_ENABLED)
    std::shared_ptr<HgiVulkanExternalBufferArena> _vkArena;
#endif

    // Strong references to everything shared this run. This is the producer's
    // job: the scene description carries only weak references, so without
    // these the buffers would be reclaimed out from under the renderer.
    std::vector<HgiExternalBufferSharedPtr> _sharedBuffers;

    // Buffers the application itself created and therefore must delete: the
    // arena only registered them.
    std::vector<uint32_t> _appGlBuffers;
    std::vector<HgiBufferHandle> _appVkBuffers;

    // The second Vulkan device standing in for a producer that owns its own
    // device (--vulkanInterop), and what it allocated.
    HgiUniquePtr _producerHgi;
    std::vector<HgiExternalBufferSharedPtr> _producerBuffers;
#if defined(PXR_VULKAN_SUPPORT_ENABLED)
    std::shared_ptr<HgiVulkanExternalBufferArena> _producerArena;
    // The semaphore pair is per arena, so it is set up once per run. When the
    // import succeeds the producer signals app-done in its own submission and
    // the consumer's commit waits on it; when it does not, the producer orders
    // its write on the host instead and the test still checks the buffer
    // sharing itself.
    bool _interopSemaphoresShared = false;
    bool _interopSemaphoresActive = false;
    // A binary semaphore may carry only one pending signal, so exactly one
    // write signals it however many buffers are shared. The others are
    // ordered by the host fence every upload already waits on, which is the
    // real guarantee here; the semaphore is what exercises the handshake.
    bool _appDoneSignalled = false;
    // The pair, natively, from whichever side created it, plus that side's
    // Hgi -- the device whose queue the WAR check submits on.
    VkSemaphore _appDoneVkSemaphore = VK_NULL_HANDLE;
    VkSemaphore _hgiDoneVkSemaphore = VK_NULL_HANDLE;
    Hgi *_semaphoreOwnerHgi = nullptr;
#endif

    std::string _outputFilePath;  // --write: writes the GPU-shared image
    bool _writeCpu = false;       // --writeCpu: --write writes the CPU image
};

HgiGLExternalBufferArena *
My_TestGLDrawing::_GetGlArena(HdSt_TestDriver *driver)
{
    // Keyed on the producer's GL context. Everything here shares one context,
    // so the current one is the key -- and that also means the arena needs no
    // semaphores: commands run in order, which is all a semaphore would have
    // established.
    if (driver->GetHgi()->GetAPIName() != HgiTokens->OpenGL) {
        return nullptr;
    }
    if (!_glArena) {
        _glArena = driver->GetHgi()
            ->GetExternalBufferArena<HgiGLExternalBufferArena>(
                _GlContextKey());
    }
    return _glArena.get();
}

#if defined(PXR_VULKAN_SUPPORT_ENABLED)
HgiVulkanExternalBufferArena *
My_TestGLDrawing::_GetVulkanArena(Hgi *hgi,
                                  std::shared_ptr<HgiVulkanExternalBufferArena> *slot)
{
    if (!hgi || hgi->GetAPIName() != HgiTokens->Vulkan) {
        return nullptr;
    }
    if (!*slot) {
        HgiVulkanDevice *device =
            static_cast<HgiVulkan *>(hgi)->GetPrimaryDevice();
        *slot = hgi->GetExternalBufferArena<HgiVulkanExternalBufferArena>(
            reinterpret_cast<uint64_t>(device->GetVulkanDevice()));
    }
    return slot->get();
}
#endif

bool
My_TestGLDrawing::_ConsumerIsVulkan(HdSt_TestDriver *driver) const
{
    return driver->GetHgi()->GetAPIName() == HgiTokens->Vulkan;
}

_SharedBuffer
My_TestGLDrawing::_MakeNativeRegisteredBuffer(HdSt_TestDriver *driver,
                                              const void *data,
                                              size_t byteSize,
                                              uint32_t stride)
{
    // The plain case: the application allocates on the consumer's own device
    // and REGISTERS the buffer, so the arena binds and reads it but never
    // deletes it. Which arena that is follows the consumer's backend -- a
    // native handle only means something to the API that minted it, and there
    // is no arena that could bridge a GL name to a Vulkan consumer, because
    // OpenGL cannot export an allocation for another API to import.
#if defined(PXR_VULKAN_SUPPORT_ENABLED)
    if (_ConsumerIsVulkan(driver)) {
        Hgi *hgi = driver->GetHgi();
        HgiVulkanExternalBufferArena *arena =
            _GetVulkanArena(hgi, &_vkArena);
        if (!arena) {
            TF_RUNTIME_ERROR("This Vulkan device has no external memory "
                             "support");
            return nullptr;
        }

        HgiBufferDesc desc;
        desc.usage = HgiBufferUsageVertex;
        desc.byteSize = byteSize;
        desc.vertexStride = stride;
        desc.initialData = data;
        desc.debugName = "app vertex buffer";
        HgiBufferHandle buffer = hgi->CreateBuffer(desc);
        VkBuffer vkBuffer =
            static_cast<HgiVulkanBuffer *>(buffer.Get())->GetVulkanBuffer();

        // The handle keeps the application's buffer alive; the arena only
        // borrows it.
        _appVkBuffers.push_back(buffer);
        return arena->RegisterBuffer(
            vkBuffer, byteSize, HgiBufferUsageVertex | HgiBufferUsageStorage);
    }
#endif

    HgiGLExternalBufferArena *arena = _GetGlArena(driver);
    if (!arena) {
        TF_RUNTIME_ERROR("This Hgi cannot consume OpenGL buffers");
        return nullptr;
    }

    // The application's own GL buffer, created and owned by the application.
    GLuint glBuf = 0;
    glCreateBuffers(1, &glBuf);
    glNamedBufferData(glBuf, byteSize, data, GL_STATIC_DRAW);
    _appGlBuffers.push_back(glBuf);

    return arena->RegisterBuffer(
        glBuf, byteSize, HgiBufferUsageVertex | HgiBufferUsageStorage);
}

#if defined(PXR_VULKAN_SUPPORT_ENABLED)
bool
My_TestGLDrawing::_VulkanUpload(Hgi *hgi, VkBuffer dst,
                                const void *data, size_t byteSize,
                                VkSemaphore signalSemaphore)
{
    HgiVulkanDevice *device =
        static_cast<HgiVulkan *>(hgi)->GetPrimaryDevice();
    VkDevice vkDevice = device->GetVulkanDevice();
    const uint32_t family = device->GetGfxQueueFamilyIndex();

    // HgiVulkan creates a single graphics queue and we deliberately do not
    // change that just for a test, so the producer records its OWN command
    // buffer and submits it independently on that same queue. The write and the
    // consumer's read are still separate submissions -- exactly what a real
    // separate-queue producer has. (The submit is serialized on the main
    // thread, so sharing Hgi's queue is safe.)
    VkQueue producerQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(vkDevice, family, 0, &producerQueue);

    HgiBufferDesc stgDesc;
    stgDesc.usage = HgiBufferUsageUpload;
    stgDesc.byteSize = byteSize;
    stgDesc.initialData = data;
    stgDesc.debugName = "producer staging";
    HgiBufferHandle staging = hgi->CreateBuffer(stgDesc);
    VkBuffer vkStaging =
        static_cast<HgiVulkanBuffer *>(staging.Get())->GetVulkanBuffer();

    // Transient pool and command buffer for the copy. Buffers are
    // VK_SHARING_MODE_EXCLUSIVE on the graphics family and the producer submits
    // on that same family, so no queue-family ownership transfer is needed
    // here; for an imported buffer the consumer's import records its own
    // acquire from VK_QUEUE_FAMILY_EXTERNAL.
    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandPoolCreateInfo pci = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pci.queueFamilyIndex = family;
    pci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    vkCreateCommandPool(vkDevice, &pci, nullptr, &pool);

    VkCommandBufferAllocateInfo cbai =
        {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cbai.commandPool = pool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    VkCommandBuffer cb = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(vkDevice, &cbai, &cb);

    VkCommandBufferBeginInfo bbi =
        {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb, &bbi);
    VkBufferCopy region = {};
    region.size = byteSize;
    vkCmdCopyBuffer(cb, vkStaging, dst, 1, &region);
    vkEndCommandBuffer(cb);

    VkSubmitInfo si = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cb;
    if (signalSemaphore != VK_NULL_HANDLE) {
        si.signalSemaphoreCount = 1;
        si.pSignalSemaphores = &signalSemaphore;
    }

    // The host fence wait lets us reclaim the transient pool and the staging
    // buffer. It does NOT consume a binary semaphore, so a consumer that waits
    // on signalSemaphore still gets a real GPU handshake; and where no
    // semaphore is passed, the wait is itself the ordering -- the write is
    // complete before the consumer is even told about the buffer.
    VkFence fence = VK_NULL_HANDLE;
    VkFenceCreateInfo fci = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    vkCreateFence(vkDevice, &fci, nullptr, &fence);
    vkQueueSubmit(producerQueue, 1, &si, fence);
    vkWaitForFences(vkDevice, 1, &fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(vkDevice, fence, nullptr);
    vkDestroyCommandPool(vkDevice, pool, nullptr);  // frees cb
    hgi->DestroyBuffer(&staging);
    return true;
}
#endif

#if defined(PXR_VULKAN_SUPPORT_ENABLED)
bool
My_TestGLDrawing::_ShareInteropSemaphores(
    HgiVulkanExternalBufferArena *producerArena,
    HgiExternalBufferArena *consumerArena)
{
    // One pair per arena, shared by every buffer in it, so this is done once
    // per run rather than per buffer.
    if (_interopSemaphoresShared) {
        return true;
    }
    _interopSemaphoresShared = true;

    // The PRODUCER creates the pair, because it is the one that can export:
    // it hands back OS handles, and the consumer imports them. That is the
    // opposite direction from --vulkanSync, where Hgi owns the semaphores and
    // the application uses them natively -- between the two tests both
    // directions of the contract are covered.
    uint64_t appDoneHandle = 0;
    uint64_t hgiDoneHandle = 0;
    if (!producerArena->CreateExportableSemaphores(
            HgiSemaphoreKindBinary, &appDoneHandle, &hgiDoneHandle)) {
        std::cout << "[extGpuBuffer] producer cannot export semaphores; "
                     "falling back to a host wait\n";
        return false;
    }

    // Binary only: OpenGL has no timeline form, and the consumer here may be
    // OpenGL. An import that cannot be done is not a failure of the test --
    // GL_EXT_semaphore may simply be absent -- so fall back to ordering the
    // write on the host instead.
    if (!consumerArena->ImportSemaphores(
            appDoneHandle, hgiDoneHandle,
            _PlatformHandleType(), HgiSemaphoreKindBinary)) {
        std::cout << "[extGpuBuffer] consumer cannot import semaphores; "
                     "falling back to a host wait\n";
        return false;
    }

    // The producer created them, so it holds them natively and is the side
    // that can wait on hgi-done.
    _appDoneVkSemaphore = static_cast<HgiVulkanSemaphore *>(
        producerArena->GetAppDoneSemaphore().get())->GetVulkanSemaphore();
    _hgiDoneVkSemaphore = static_cast<HgiVulkanSemaphore *>(
        producerArena->GetHgiDoneSemaphore().get())->GetVulkanSemaphore();
    _semaphoreOwnerHgi = _producerHgi.get();
    _interopSemaphoresActive = true;
    std::cout << "[extGpuBuffer] semaphore pair shared producer -> consumer\n";
    return true;
}
#endif

#if defined(PXR_VULKAN_SUPPORT_ENABLED)
void
My_TestGLDrawing::_VerifyHgiDoneSignal()
{
    if (_hgiDoneVkSemaphore == VK_NULL_HANDLE || !_semaphoreOwnerHgi) {
        return;
    }

    // Wait on the GPU for the signal the consumer emits when it has finished
    // reading -- the moment a real producer becomes free to overwrite the
    // buffer in place. Nothing else in the test covers that half of the
    // bracket: the app-done wait is verified by the image coming out right,
    // but a consumer that never signalled hgi-done would look identical.
    //
    // An empty submission is enough. All it does is wait, so the fence tells
    // us the semaphore was signalled and nothing more.
    HgiVulkanDevice *device =
        static_cast<HgiVulkan *>(_semaphoreOwnerHgi)->GetPrimaryDevice();
    VkDevice vkDevice = device->GetVulkanDevice();

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(vkDevice, device->GetGfxQueueFamilyIndex(), 0, &queue);

    const VkPipelineStageFlags waitStage =
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkSubmitInfo si = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = &_hgiDoneVkSemaphore;
    si.pWaitDstStageMask = &waitStage;

    VkFence fence = VK_NULL_HANDLE;
    VkFenceCreateInfo fci = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    vkCreateFence(vkDevice, &fci, nullptr, &fence);
    vkQueueSubmit(queue, 1, &si, fence);

    // Bounded, deliberately. An unsignalled binary semaphore would block for
    // ever, and a test that hangs says far less than one that fails.
    const uint64_t timeoutNs = 5ull * 1000ull * 1000ull * 1000ull;
    const VkResult res =
        vkWaitForFences(vkDevice, 1, &fence, VK_TRUE, timeoutNs);

    if (res == VK_SUCCESS) {
        vkDestroyFence(vkDevice, fence, nullptr);
        std::cout << "[extGpuBuffer] hgi-done signal observed (WAR ok)\n";
        _hgiDoneVkSemaphore = VK_NULL_HANDLE;
        return;
    }

    // Fatal rather than a reported error: the submission above is still
    // pending on a wait that will never complete, so ordinary teardown would
    // block in vkDeviceWaitIdle while destroying the semaphore. There is no
    // way to cancel a Vulkan submission, so the only honest options are to
    // hang or to stop here -- and this is a real deadlock in the consumer's
    // half of the bracket, not a flake worth recovering from.
    TF_FATAL_ERROR("The consumer never signalled the arena's hgi-done "
                   "semaphore (waited %.1fs). Storm's commit is expected to "
                   "encode that signal after reading the shared buffers.",
                   double(timeoutNs) / 1e9);
}
#endif

_SharedBuffer
My_TestGLDrawing::_MakeInteropBuffer(HdSt_TestDriver *driver,
                                     const void *data, size_t byteSize)
{
#if defined(PXR_VULKAN_SUPPORT_ENABLED)
    if (!_EnsureProducerHgi(driver)) {
        return nullptr;
    }

    // The producer allocates out of its OWN arena, which allocates exportable
    // and hands back everything an importer needs. Its VkBuffer is deliberately
    // not shared: it belongs to another logical device, so only the memory is
    // meaningful to anyone else.
    HgiVulkanExternalBufferArena *producerArena =
        _GetVulkanArena(_producerHgi.get(), &_producerArena);
    if (!producerArena) {
        TF_RUNTIME_ERROR("--vulkanInterop needs a Vulkan producer with "
                         "external memory support (run with "
                         "HGI_ENABLE_VULKAN=1)");
        return nullptr;
    }

    HgiExternalBufferSharedPtr producerBuffer =
        producerArena->AllocateBuffer(
            byteSize, HgiBufferUsageVertex | HgiBufferUsageStorage,
            "interop producer");
    if (!producerBuffer) {
        TF_RUNTIME_ERROR("--vulkanInterop could not allocate exportable "
                         "memory on the producer's device");
        return nullptr;
    }
    // Held for the run: the producer remains the owner of this allocation.
    _producerBuffers.push_back(producerBuffer);

    HgiVulkanExternalBuffer *producerVkBuffer =
        static_cast<HgiVulkanExternalBuffer *>(producerBuffer.get());
    HgiVulkanExternalBufferExportInfo const &info =
        producerVkBuffer->GetExportInfo();
    if (!info.externalHandle) {
        TF_RUNTIME_ERROR("--vulkanInterop producer could not export its "
                         "allocation");
        return nullptr;
    }

    // Resolve the consumer's arena before writing: it is both the import
    // target and the side that has to import the semaphore pair, and the write
    // below signals a semaphore only if that succeeded.
    //
    // Which arena it is is the whole point of the two-device topology: a
    // native VkBuffer from the producer's logical device would mean nothing
    // here, so what crosses is the memory.
    HgiExternalBufferArena *consumerArena = _ConsumerIsVulkan(driver)
        ? static_cast<HgiExternalBufferArena *>(
              _GetVulkanArena(driver->GetHgi(), &_vkArena))
        : static_cast<HgiExternalBufferArena *>(_GetGlArena(driver));
    if (!consumerArena) {
        TF_RUNTIME_ERROR("--vulkanInterop consumer has no arena to import "
                         "into");
        return nullptr;
    }

    // The producer creates the semaphore pair and exports it; the consumer
    // imports it. When that works the producer's write signals app-done and
    // the consumer's commit waits on it -- a real GPU handshake across devices,
    // and across APIs when the consumer is OpenGL. When it does not, the host
    // fence in _VulkanUpload orders the write instead, and the rest of the
    // test is unaffected.
    _ShareInteropSemaphores(producerArena, consumerArena);

    const bool signalAppDone =
        _interopSemaphoresActive && !_appDoneSignalled;
    if (!_VulkanUpload(_producerHgi.get(), producerVkBuffer->GetVulkanBuffer(),
                       data, byteSize,
                       signalAppDone ? _appDoneVkSemaphore : VK_NULL_HANDLE)) {
        return nullptr;
    }
    if (signalAppDone) {
        _appDoneSignalled = true;
        // Tell the consumer's arena there is a signal to wait on. Without it
        // the arena assumes the application is idle and skips the wait, which
        // is the right default but not what we are here to exercise.
        consumerArena->NotifyAppDone();
    }

    if (_ConsumerIsVulkan(driver)) {
        HgiVulkanExternalBufferArena *arena =
            static_cast<HgiVulkanExternalBufferArena *>(consumerArena);
        HgiVulkanImportBufferDesc importDesc;
        importDesc.externalHandle = info.externalHandle;
        importDesc.handleType = info.handleType;
        importDesc.memoryBlockSize = info.memoryBlockSize;
        importDesc.memoryOffset = info.memoryOffset;
        importDesc.byteSize = byteSize;
        importDesc.dedicated = info.dedicated;
        importDesc.usage = HgiBufferUsageVertex | HgiBufferUsageStorage;
        importDesc.debugName = "interop consumer";

        HgiExternalBufferSharedPtr imported = arena->ImportBuffer(importDesc);
        if (!imported) {
            TF_RUNTIME_ERROR("--vulkanInterop could not import the producer's "
                             "allocation into the consumer's device");
        }
        return imported;
    }

    HgiGLExternalBufferArena *glArena =
        static_cast<HgiGLExternalBufferArena *>(consumerArena);
    HgiGLImportBufferDesc importDesc;
    importDesc.externalHandle = info.externalHandle;
    importDesc.handleType = info.handleType;
    importDesc.memoryBlockSize = info.memoryBlockSize;
    importDesc.memoryOffset = info.memoryOffset;
    importDesc.byteSize = byteSize;
    importDesc.dedicated = info.dedicated;
    importDesc.usage = HgiBufferUsageVertex | HgiBufferUsageStorage;
    importDesc.debugName = "interop consumer";

    HgiExternalBufferSharedPtr imported = glArena->ImportBuffer(importDesc);
    if (!imported) {
        TF_RUNTIME_ERROR("--vulkanInterop could not import the producer's "
                         "allocation into OpenGL");
    }
    return imported;
#else
    (void)driver;
    (void)data;
    (void)byteSize;
    TF_RUNTIME_ERROR("--vulkanInterop requires a Vulkan-enabled build");
    return nullptr;
#endif
}

_SharedBuffer
My_TestGLDrawing::_MakeVulkanSyncBuffer(HdSt_TestDriver *driver,
                                        const void *data, size_t byteSize,
                                        uint32_t stride)
{
#if defined(PXR_VULKAN_SUPPORT_ENABLED)
    Hgi *hgi = driver->GetHgi();
    if (!_ConsumerIsVulkan(driver)) {
        TF_RUNTIME_ERROR("--vulkanSync needs a Vulkan Hgi (run with "
                         "HGI_ENABLE_VULKAN=1)");
        return nullptr;
    }
    HgiVulkanExternalBufferArena *arena = _GetVulkanArena(hgi, &_vkArena);
    if (!arena) {
        TF_RUNTIME_ERROR("--vulkanSync needs a Vulkan device with external "
                         "memory support");
        return nullptr;
    }

    // Destination vertex buffer on the consumer's own device, which Storm binds
    // directly: same logical device, so nothing is imported and only the write
    // ordering matters.
    HgiBufferDesc dstDesc;
    dstDesc.usage = HgiBufferUsageVertex;
    dstDesc.byteSize = byteSize;
    dstDesc.vertexStride = stride;
    dstDesc.debugName = "vulkanSync dst";
    HgiBufferHandle dst = hgi->CreateBuffer(dstDesc);
    VkBuffer vkDst =
        static_cast<HgiVulkanBuffer *>(dst.Get())->GetVulkanBuffer();

    // One semaphore pair for the whole arena rather than one per buffer. Both
    // sides are on this device, so the producer reads the native VkSemaphore
    // straight off the arena instead of exporting and re-importing it.
    // The pair belongs to the arena, not to a buffer, so create it once even
    // when several buffers are shared -- and a binary semaphore may carry only
    // one pending signal, so exactly one write signals it.
    if (!_interopSemaphoresShared) {
        _interopSemaphoresShared = true;
        uint64_t appDoneHandle = 0, hgiDoneHandle = 0;
        if (!arena->CreateExportableSemaphores(
                HgiSemaphoreKindBinary, &appDoneHandle, &hgiDoneHandle)) {
            TF_RUNTIME_ERROR("--vulkanSync could not create the arena's "
                             "semaphore pair");
            return nullptr;
        }
        // Here Hgi owns the pair and the application uses it natively, which
        // is the opposite direction from the interop test's import.
        _appDoneVkSemaphore = static_cast<HgiVulkanSemaphore *>(
            arena->GetAppDoneSemaphore().get())->GetVulkanSemaphore();
        _hgiDoneVkSemaphore = static_cast<HgiVulkanSemaphore *>(
            arena->GetHgiDoneSemaphore().get())->GetVulkanSemaphore();
        _semaphoreOwnerHgi = hgi;
        _interopSemaphoresActive = true;
    }

    // Write it as the producer's own work, signalling app-done; Storm's commit
    // waits on that before reading (RAW).
    const bool signalAppDone =
        _interopSemaphoresActive && !_appDoneSignalled;
    if (!_VulkanUpload(hgi, vkDst, data, byteSize,
                       signalAppDone ? _appDoneVkSemaphore : VK_NULL_HANDLE)) {
        return nullptr;
    }

    if (signalAppDone) {
        _appDoneSignalled = true;
        // Tell the arena there is a signal to wait on. Without this it cannot
        // tell "the application published something" from "the application is
        // idle", and has to assume idle -- rightly, since waiting on a binary
        // semaphore nobody will signal hangs the frame.
        arena->NotifyAppDone();
    }

    // Registered rather than adopted: this VkBuffer is the test's, and the
    // handle below keeps it alive until teardown.
    _appVkBuffers.push_back(dst);
    return arena->RegisterBuffer(
        vkDst, byteSize, HgiBufferUsageVertex | HgiBufferUsageStorage);
#else
    (void)driver;
    (void)data;
    (void)byteSize;
    (void)stride;
    TF_RUNTIME_ERROR("--vulkanSync requires a Vulkan-enabled build");
    return nullptr;
#endif
}

bool
My_TestGLDrawing::_EnsureProducerHgi(HdSt_TestDriver *driver)
{
    if (_producerHgi) {
        return true;
    }

    _producerHgi = Hgi::CreateNamedHgi(HgiTokens->Vulkan);
    if (!_producerHgi) {
        TF_RUNTIME_ERROR("--vulkanInterop could not create a second Vulkan "
                         "Hgi to stand in for the producer's device");
        return false;
    }
    (void)driver;
    return true;
}

void
My_TestGLDrawing::_ReleaseProducer()
{
    // Dropping the references is the whole of it. Each arena reclaims its own
    // buffers once nothing holds them and the GPU has retired the work that
    // named them, and the arena itself goes before the Hgi that owns it.
    _producerBuffers.clear();
#if defined(PXR_VULKAN_SUPPORT_ENABLED)
    _producerArena.reset();
#endif
    _producerHgi.reset();
}

_SharedBuffer
My_TestGLDrawing::_MakeGpuBuffer(HdSt_TestDriver *driver,
                                 const void *data, size_t byteSize,
                                 uint32_t stride)
{
    if (_vulkanInterop) {
        return _MakeInteropBuffer(driver, data, byteSize);
    }
    if (_vulkanSync) {
        return _MakeVulkanSyncBuffer(driver, data, byteSize, stride);
    }
    return _MakeNativeRegisteredBuffer(driver, data, byteSize, stride);
}

HdContainerDataSourceHandle
My_TestGLDrawing::_WithExtGpuBuffer(
    HdSt_TestDriver *driver,
    const HdContainerDataSourceHandle &primvar,
    const _SharedBuffer &shared, size_t byteSize,
    HdTupleType elementType, size_t numElements)
{
    using _SizeDs = HdRetainedTypedSampledDataSource<size_t>;
    using _BoolDs = HdRetainedTypedSampledDataSource<bool>;
    using _ResourceDs =
        HdRetainedTypedSampledDataSource<HdExternalBufferPtr>;

    (void)driver;
    (void)byteSize;

    // One value names the buffer, and it is WEAK: the scene description must
    // not be what keeps GPU memory alive. The test holds its own strong
    // reference for the duration of the run, which is the producer's job.
    HdExtGpuBufferSchema::Builder builder;
    builder
        .SetExternalResource(_ResourceDs::New(HdExternalBufferPtr(shared)))
        .SetNumElements(_SizeDs::New(numElements))
        .SetElementType(
            HdRetainedTypedSampledDataSource<HdTupleType>::New(elementType))
        .SetByteOffset(_SizeDs::New(0))
        .SetByteStride(_SizeDs::New(0))
        .SetAllowDirectBind(_BoolDs::New(_allowDirectBind));

    return HdOverlayContainerDataSource::New(
        primvar,
        HdRetainedContainerDataSource::New(
            HdExtGpuBufferSchema::GetSchemaToken(), builder.Build()));
}

HdContainerDataSourceHandle
My_TestGLDrawing::_BuildCubePrimvars(HdSt_TestDriver *driver,
                                     bool gpuShare)
{
    const VtVec3fArray points = _CubePoints(_halfSize);

    HdContainerDataSourceHandle pointsPrimvar;
    if (gpuShare) {
        // GPU-only: empty CPU value + extGpuBuffer child pointing at a real
        // GL buffer holding the 8 cube corners.
        const size_t byteSize = points.size() * sizeof(GfVec3f);
        const _SharedBuffer shared =
            _MakeGpuBuffer(driver, points.cdata(), byteSize,
                           sizeof(GfVec3f));
        if (!shared) {
            TF_RUNTIME_ERROR("Could not share the cube points");
            return nullptr;
        }
        // The producer's strong reference. Everything downstream is weak.
        _sharedBuffers.push_back(shared);
        HdContainerDataSourceHandle emptyValue =
            HdPrimvarSchema::Builder()
                .SetPrimvarValue(_PointArrayDs::New(VtVec3fArray()))
                .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
                    HdPrimvarSchemaTokens->vertex))
                .SetRole(HdPrimvarSchema::BuildRoleDataSource(
                    HdPrimvarSchemaTokens->point))
                .Build();
        pointsPrimvar = _WithExtGpuBuffer(
            driver, emptyValue, shared, byteSize,
            HdTupleType{HdTypeFloatVec3, 1}, points.size());
    } else {
        pointsPrimvar =
            HdPrimvarSchema::Builder()
                .SetPrimvarValue(_PointArrayDs::New(points))
                .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
                    HdPrimvarSchemaTokens->vertex))
                .SetRole(HdPrimvarSchema::BuildRoleDataSource(
                    HdPrimvarSchemaTokens->point))
                .Build();
    }

    HdContainerDataSourceHandle colorPrimvar =
        HdPrimvarSchema::Builder()
            .SetPrimvarValue(HdRetainedTypedSampledDataSource<VtVec3fArray>::New(
                VtVec3fArray{GfVec3f(0.2f, 0.7f, 0.9f)}))
            .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
                HdPrimvarSchemaTokens->constant))
            .SetRole(HdPrimvarSchema::BuildRoleDataSource(
                HdPrimvarSchemaTokens->color))
            .Build();

    return HdRetainedContainerDataSource::New(
        HdPrimvarsSchemaTokens->points, pointsPrimvar,
        HdTokens->displayColor,         colorPrimvar);
}

void
My_TestGLDrawing::_BuildScene(HdSt_TestDriver *driver,
                              HdRetainedSceneIndexRefPtr &scene,
                              bool gpuShare)
{
    scene = HdRetainedSceneIndex::New();

    const SdfPath cubePath("/cube");
    const SdfPath instancerPath("/instancer");

    HdContainerDataSourceHandle primvarsDs =
        _BuildCubePrimvars(driver, gpuShare);

    HdContainerDataSourceHandle meshDs =
        HdMeshSchema::Builder()
            .SetTopology(HdMeshTopologySchema::Builder()
                .SetFaceVertexCounts(_IntArrayDs::New(_faceVertexCounts))
                .SetFaceVertexIndices(_IntArrayDs::New(_faceVertexIndices))
                .Build())
            .Build();

    const GfRange3d cubeRange({-_halfSize, -_halfSize, -_halfSize},
                              { _halfSize,  _halfSize,  _halfSize});
    HdContainerDataSourceHandle extentDs =
        HdExtentSchema::Builder()
            .SetMin(HdRetainedTypedSampledDataSource<GfVec3d>::New(
                cubeRange.GetMin()))
            .SetMax(HdRetainedTypedSampledDataSource<GfVec3d>::New(
                cubeRange.GetMax()))
            .Build();

    if (!_instancing) {
        // ---- Basic: one non-instanced cube -------------------------------
        HdContainerDataSourceHandle cubeDs =
            HdRetainedContainerDataSource::New(
                HdXformSchemaTokens->xform,
                HdXformSchema::Builder()
                    .SetMatrix(HdRetainedTypedSampledDataSource<GfMatrix4d>::New(
                        GfMatrix4d(1.0)))
                    .Build(),
                HdExtentSchemaTokens->extent, extentDs,
                HdMeshSchemaTokens->mesh, meshDs,
                HdPrimvarsSchemaTokens->primvars, primvarsDs);
        scene->AddPrims({{cubePath, HdPrimTypeTokens->mesh, cubeDs}});
        return;
    }

    // ---- Instancing: prototype cube + instancer --------------------------
    HdContainerDataSourceHandle instancedByDs =
        HdInstancedBySchema::Builder()
            .SetPaths(HdRetainedTypedSampledDataSource<VtArray<SdfPath>>::New(
                VtArray<SdfPath>({instancerPath})))
            .Build();

    HdContainerDataSourceHandle cubeDs =
        HdRetainedContainerDataSource::New(
            HdXformSchemaTokens->xform,
            HdXformSchema::Builder()
                .SetMatrix(HdRetainedTypedSampledDataSource<GfMatrix4d>::New(
                    GfMatrix4d(1.0)))
                .SetResetXformStack(
                    HdRetainedTypedSampledDataSource<bool>::New(true))
                .Build(),
            HdExtentSchemaTokens->extent, extentDs,
            HdMeshSchemaTokens->mesh, meshDs,
            HdPrimvarsSchemaTokens->primvars, primvarsDs,
            HdInstancedBySchema::GetSchemaToken(), instancedByDs);

    // Instance transforms: a grid of translations centered on the origin.
    // GfMatrix4f (float) layout -- the GPU path skips the CPU double->float
    // conversion, so the shared buffer must already be float mat4.
    const int numInstances = _div * _div;
    VtMatrix4dArray matricesD(numInstances);          // CPU baseline path
    std::vector<GfMatrix4f> matricesF(numInstances);  // GPU buffer
    const float offset = (_div - 1) * 0.5f * _spacing;
    for (int y = 0; y < _div; ++y) {
        for (int x = 0; x < _div; ++x) {
            const int i = x + y * _div;
            const GfVec3d t(x * _spacing - offset, y * _spacing - offset, 0.0);
            GfMatrix4d m(1.0);
            m.SetTranslate(t);
            matricesD[i] = m;
            matricesF[i] = GfMatrix4f(m);
        }
    }

    VtIntArray prototypeIndices(numInstances);
    for (int i = 0; i < numInstances; ++i) {
        prototypeIndices[i] = i;
    }
    HdDataSourceBaseHandle instanceIndicesDs =
        HdRetainedTypedSampledDataSource<VtIntArray>::New(prototypeIndices);
    HdVectorDataSourceHandle instanceIndicesVec =
        HdRetainedSmallVectorDataSource::New(1, &instanceIndicesDs);

    HdContainerDataSourceHandle instancerTopologyDs =
        HdInstancerTopologySchema::Builder()
            .SetPrototypes(
                HdRetainedTypedSampledDataSource<VtArray<SdfPath>>::New(
                    {cubePath}))
            .SetInstanceIndices(instanceIndicesVec)
            .Build();

    HdContainerDataSourceHandle xformPrimvar;
    if (gpuShare) {
        const size_t byteSize = numInstances * sizeof(GfMatrix4f);
        const _SharedBuffer shared =
            _MakeGpuBuffer(driver, matricesF.data(), byteSize,
                           sizeof(GfMatrix4f));
        if (!shared) {
            TF_RUNTIME_ERROR("Could not share the instance transforms");
            return;
        }
        _sharedBuffers.push_back(shared);
        HdContainerDataSourceHandle emptyValue =
            HdPrimvarSchema::Builder()
                .SetPrimvarValue(
                    HdRetainedTypedSampledDataSource<VtMatrix4dArray>::New(
                        VtMatrix4dArray()))
                .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
                    HdPrimvarSchemaTokens->instance))
                .Build();
        xformPrimvar = _WithExtGpuBuffer(
            driver, emptyValue, shared, byteSize,
            HdTupleType{HdTypeFloatMat4, 1}, numInstances);
    } else {
        xformPrimvar =
            HdPrimvarSchema::Builder()
                .SetPrimvarValue(
                    HdRetainedTypedSampledDataSource<VtMatrix4dArray>::New(
                        matricesD))
                .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
                    HdPrimvarSchemaTokens->instance))
                .Build();
    }

    HdContainerDataSourceHandle instancerPrimvarsDs =
        HdRetainedContainerDataSource::New(
            HdInstancerTokens->instanceTransforms, xformPrimvar);

    HdContainerDataSourceHandle instancerDs =
        HdRetainedContainerDataSource::New(
            HdInstancerTopologySchema::GetSchemaToken(), instancerTopologyDs,
            HdPrimvarsSchema::GetSchemaToken(), instancerPrimvarsDs);

    scene->AddPrims({
        {cubePath, HdPrimTypeTokens->mesh, cubeDs},
        {instancerPath, HdInstancerTokens->instancer, instancerDs}});
}

void
My_TestGLDrawing::_CheckArenaReclaim(HdSt_TestDriver *driver,
                                     HdRetainedSceneIndexRefPtr const &scene)
{
    if (_sharedBuffers.empty()) {
        return;
    }
    const HgiExternalBufferWeakPtr weak = _sharedBuffers.front();

    // Make Storm let go first, while we still hold our own reference. Order
    // matters: a collection triggered by this render must find the buffer
    // still referenced, so that the only thing the assertions below can be
    // observing is the arena's own reference.
    HdSceneIndexObserver::RemovedPrimEntries removed;
    removed.emplace_back(SdfPath("/cube"));
    if (_instancing) {
        removed.emplace_back(SdfPath("/instancer"));
    }
    scene->RemovePrims(removed);
    driver->Draw();

    // Now the producer lets go too. Nothing outside the arena references the
    // buffer, and nothing has been collected since.
    _sharedBuffers.clear();
    _producerBuffers.clear();

    // A reference count reaching zero is NOT what frees an external buffer.
    // If it were, a producer could not tell "the renderer has let go" from
    // "the GPU is finished", and recycling on the former is a use-after-free.
    if (weak.expired()) {
        TF_RUNTIME_ERROR("The shared buffer was destroyed as soon as the last "
                         "outside reference dropped. The arena is supposed to "
                         "hold its own until GarbageCollect, so that expiry "
                         "means the GPU is done with it too.");
        return;
    }

    // Collection releases it. A frame per iteration as well as a collect,
    // because an application reclaims as it renders and that is the behaviour
    // worth asserting -- and because this driver's task list has no
    // HdxAovInputTask or HdxPresentTask, so Hgi::StartFrame/EndFrame are never
    // called here at all. Reclamation must not depend on hooks a client may
    // never invoke.
    //
    // Bounded: lagging a few frames is the safe direction, never releasing is
    // a leak.
    Hgi *hgi = driver->GetHgi();
    const int maxFrames = 8;
    int frames = 0;
    for (; frames < maxFrames && !weak.expired(); ++frames) {
        driver->Draw();
        hgi->GarbageCollect();
    }

    if (!weak.expired()) {
        // Which of the two it is comes straight off the arena: a buffer that
        // reached the pending list was stamped and is waiting on the retire
        // test; one still counted as live was never a candidate, so something
        // is holding a reference.
        HgiExternalBufferArena *arena = nullptr;
        if (_glArena) {
            arena = _glArena.get();
        }
#if defined(PXR_VULKAN_SUPPORT_ENABLED)
        if (_vkArena) {
            arena = _vkArena.get();
        }
#endif
        const HgiExternalBufferArenaUsage usage =
            arena ? arena->GetUsage() : HgiExternalBufferArenaUsage();
        TF_RUNTIME_ERROR("The shared buffer was still alive after %d frames "
                         "with nothing referencing it. arena live=%zu "
                         "pending=%zu, buffer use_count=%ld. pending>0 means "
                         "the retire test never passes; live>0 means something "
                         "still holds a reference.",
                         maxFrames, usage.numBuffers, usage.numPendingDestroy,
                         long(weak.use_count()));
        return;
    }

    std::cout << "[extGpuBuffer] arena reclaimed the buffer after "
              << frames << " frame(s)\n";
}

void
My_TestGLDrawing::_RenderToPixels(bool gpuShare, std::vector<uint8_t> &out,
                                  int &width, int &height,
                                  const std::string &writePath)
{
    const int w = GetWidth(), h = GetHeight();

#if defined(PXR_VULKAN_SUPPORT_ENABLED)
    // Each pass builds its own driver, hence its own arena and its own
    // semaphore pair; the CPU pass shares nothing and sets none of this.
    _interopSemaphoresShared = false;
    _interopSemaphoresActive = false;
    _appDoneSignalled = false;
    _appDoneVkSemaphore = VK_NULL_HANDLE;
    _hgiDoneVkSemaphore = VK_NULL_HANDLE;
    _semaphoreOwnerHgi = nullptr;
#endif

    auto driver = std::make_unique<HdSt_TestDriver>(HdReprTokens->hull);

    // Report the backend Storm actually runs on (the harness always creates a
    // GL context for its window, so the GL vendor banner is NOT the Storm Hgi).
    if (gpuShare) {
        std::cout << "[extGpuBuffer] Storm Hgi backend = "
                  << driver->GetHgi()->GetAPIName().GetString() << std::endl;
    }

    HdRetainedSceneIndexRefPtr scene;
    _BuildScene(driver.get(), scene, gpuShare);

    // Feed the geometry through a scene index so the extGpuBuffer child
    // survives to the terminal scene index the consumer reads from.
    driver->GetDelegate().GetRenderIndex().InsertSceneIndex(
        scene, SdfPath::AbsoluteRootPath());

    driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    driver->SetClearDepth(1.0f);
    driver->SetupAovs(w, h);
    driver->SetCamera(GetViewMatrix(), GetProjectionMatrix(),
                      CameraUtilFraming(GfRect2i(GfVec2i(0, 0), w, h)));

    // RAW ordering needs nothing from the test: Storm's commit encodes the
    // arena's wait before it reads any shared buffer, and its signal after.
    driver->Draw();

    if (!writePath.empty()) {
        driver->WriteToFile("color", writePath);
    }

    // Read the color AOV back into `out`.
    HdRenderBuffer *rb = dynamic_cast<HdRenderBuffer *>(
        driver->GetDelegate().GetRenderIndex().GetBprim(
            HdPrimTypeTokens->renderBuffer, _colorAovId));
    if (!rb) {
        TF_RUNTIME_ERROR("No color render buffer to read back");
    } else {
        width = rb->GetWidth();
        height = rb->GetHeight();
        const size_t bpp = HdDataSizeOfFormat(rb->GetFormat());
        const uint8_t *data = static_cast<const uint8_t *>(rb->Map());
        out.assign(data, data + size_t(width) * height * bpp);
        rb->Unmap();
    }

#if defined(PXR_VULKAN_SUPPORT_ENABLED)
    // Now that the frame has been rendered and read back, the consumer must
    // have signalled that it is finished reading.
    _VerifyHgiDoneSignal();
#endif

    // With the image already compared, the shared buffers are free to go --
    // which is the point at which the arena's reclaim contract is observable.
    if (gpuShare) {
        _CheckArenaReclaim(driver.get(), scene);
    }

    // Let go of everything shared this run, in the order the ownership rules
    // require.
    //
    // The consumer's references go first (with the driver, below). Ours go
    // here: dropping them is all a producer has to do -- no arena call, no Hgi
    // call. The arena notices the count fall on its next sweep and reclaims
    // each buffer once the GPU has retired the work that named it, which is
    // the only point at which reclaiming is safe.
    _sharedBuffers.clear();

    // Buffers the application created itself are the application's to delete;
    // the arena only registered them and never would. This is the contract a
    // pooled viewport buffer needs, and getting it wrong is a double free.
    for (uint32_t glBuf : _appGlBuffers) {
        glDeleteBuffers(1, &glBuf);
    }
    _appGlBuffers.clear();

    Hgi *hgi = driver->GetHgi();
    for (HgiBufferHandle &b : _appVkBuffers) {
        hgi->DestroyBuffer(&b);
    }
    _appVkBuffers.clear();

    // The consumer imported the producer's memory, so drop the whole consumer
    // -- and with it its arena, which owns those imports -- before the
    // producer device that allocated the memory.
    _glArena.reset();
#if defined(PXR_VULKAN_SUPPORT_ENABLED)
    _vkArena.reset();
#endif
    driver.reset();
    _ReleaseProducer();
}

void
My_TestGLDrawing::OffscreenTest()
{
    SetCameraTranslate(_CameraTranslate());

    std::vector<uint8_t> cpuPixels, gpuPixels;
    int cw = 0, ch = 0, gw = 0, gh = 0;

    _RenderToPixels(/*gpuShare*/false, cpuPixels, cw, ch,
                    _writeCpu ? _outputFilePath : std::string());
    _RenderToPixels(/*gpuShare*/true, gpuPixels, gw, gh,
                    _writeCpu ? std::string() : _outputFilePath);

    if (cpuPixels.empty() || gpuPixels.empty()) {
        TF_RUNTIME_ERROR("Readback produced no pixels (cpu=%zu gpu=%zu)",
                         cpuPixels.size(), gpuPixels.size());
        return;
    }
    if (cw != gw || ch != gh || cpuPixels.size() != gpuPixels.size()) {
        TF_RUNTIME_ERROR("CPU/GPU image dimensions differ: "
                         "%dx%d (%zu) vs %dx%d (%zu)",
                         cw, ch, cpuPixels.size(), gw, gh, gpuPixels.size());
        return;
    }

    // Both images come from the same GPU in the same run, so a correct GPU
    // path is bit-identical. Allow a tiny per-channel slack only to be safe.
    const int kTolerance = 2;
    size_t diffBytes = 0;
    int maxDiff = 0;
    for (size_t i = 0; i < gpuPixels.size(); ++i) {
        const int d = std::abs(int(gpuPixels[i]) - int(cpuPixels[i]));
        if (d > maxDiff) {
            maxDiff = d;
        }
        if (d > kTolerance) {
            ++diffBytes;
        }
    }

    const double diffFraction = double(diffBytes) / double(gpuPixels.size());
    std::cout << (_instancing ? "[instancing] " : "[basic] ")
              << "CPU vs GPU: maxDiff=" << maxDiff
              << " diffBytes=" << diffBytes
              << " (" << (diffFraction * 100.0) << "%)\n";

    // Any GPU-shared image that meaningfully diverges from the CPU render is a
    // failure (a silent fallback renders blank -> ~100% divergence).
    if (diffFraction > 0.001) {
        TF_RUNTIME_ERROR("GPU-shared render differs from CPU baseline: "
                         "%zu bytes (%.3f%%) exceed tolerance, maxDiff=%d",
                         diffBytes, diffFraction * 100.0, maxDiff);
    }
}

void
My_TestGLDrawing::DrawTest()
{
    // Interactive mode: show the GPU-shared scene.
    if (!_driver) {
        SetCameraTranslate(_CameraTranslate());
        _driver = std::make_unique<HdSt_TestDriver>(HdReprTokens->hull);
        _BuildScene(_driver.get(), _driverScene,
                    /*gpuShare*/true);
        _driver->GetDelegate().GetRenderIndex().InsertSceneIndex(
            _driverScene, SdfPath::AbsoluteRootPath());
        _driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
        _driver->SetClearDepth(1.0f);
        _driver->SetupAovs(GetWidth(), GetHeight());
    }

    _driver->SetCamera(GetViewMatrix(), GetProjectionMatrix(),
                       CameraUtilFraming(
                           GfRect2i(GfVec2i(0, 0), GetWidth(), GetHeight())));
    _driver->UpdateAovDimensions(GetWidth(), GetHeight());
    _driver->Draw();
}

void
My_TestGLDrawing::UninitTest()
{
    _sharedBuffers.clear();
    for (uint32_t glBuf : _appGlBuffers) {
        glDeleteBuffers(1, &glBuf);
    }
    _appGlBuffers.clear();
    if (_driver) {
        Hgi *hgi = _driver->GetHgi();
        for (HgiBufferHandle &b : _appVkBuffers) {
            hgi->DestroyBuffer(&b);
        }
    }
    _appVkBuffers.clear();
    // Same ordering rule as _RenderToPixels: the consumer's imports reference
    // the producer's memory, so the consumer goes first.
    _glArena.reset();
#if defined(PXR_VULKAN_SUPPORT_ENABLED)
    _vkArena.reset();
#endif
    _driver.reset();
    _ReleaseProducer();
}

void
My_TestGLDrawing::Present(uint32_t framebuffer)
{
    if (_driver) {
        _driver->Present(GetWidth(), GetHeight(), framebuffer);
    }
}

void
My_TestGLDrawing::ParseArgs(int argc, char *argv[])
{
    for (int i = 0; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--write") {
            _outputFilePath = argv[++i];
        } else if (arg == "--div") {
            _div = atoi(argv[++i]);
        } else if (arg == "--instancing") {
            _instancing = true;
        } else if (arg == "--vulkanSync") {
            _vulkanSync = true;
        } else if (arg == "--vulkanInterop") {
            _vulkanInterop = true;
        } else if (arg == "--copy") {
            _allowDirectBind = false;
        } else if (arg == "--writeCpu") {
            _writeCpu = true;   // --write emits the CPU image instead of GPU
        }
    }
}

int
main(int argc, char *argv[])
{
    TfErrorMark mark;

    My_TestGLDrawing driver;
    driver.RunTest(argc, argv);

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    }
    std::cout << "FAILED" << std::endl;
    return EXIT_FAILURE;
}
