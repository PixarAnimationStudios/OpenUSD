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
//   * default: the buffer is allocated by the consumer's own Hgi. No interop.
//   * --vulkanSync: a Vulkan producer allocates the buffer on the CONSUMER's own
//     device but writes it as its own work, in an independent command buffer it
//     submits itself on the device's graphics queue. No import is needed (Storm
//     adopts the VkBuffer, the adopt route, since the logical device matches),
//     but the producer's write and the consumer's read are now distinct
//     submissions that need ordering, so the producer publishes a RAW/WAR native
//     binary-semaphore pair that Storm's draw waits on and signals.
//   * --vulkanInterop: a SECOND Vulkan device stands in for a
//     producer with its own device. It allocates the exportable memory, GL
//     imports it and writes the geometry, and Storm imports the OS memory
//     handle into its own device (the import route). The producer's VkBuffer
//     and native semaphores are deliberately NOT published: they belong to
//     another logical device, so only the OS handles are meaningful.
//
// Orthogonal to the producer topology, --copy publishes directBindable=false,
// which flips the consumption strategy from zero-copy direct bind to a GPU->GPU
// blit into Storm's aggregated vertex buffer. Combined with --vulkanInterop it
// is the interesting case: the buffer is IMPORTED from a foreign logical device
// and then blitted (rather than aliased), exercising the import + blit path that
// the adopt-route default and the direct-bind interop test do not.
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
#include "pxr/imaging/hd/extGpuSyncSchema.h"
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
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hd/xformSchema.h"

#include "pxr/imaging/hgi/buffer.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

// The --vulkanSync producer records and submits its own command buffer on the
// consumer's Vulkan device, which needs the concrete HgiVulkan types (there is
// no Hgi-level API for recording a raw producer submission). Only compiled into
// Vulkan-enabled builds; the --vulkanSync test is likewise only registered there.
#if defined(PXR_VULKAN_SUPPORT_ENABLED)
#include "pxr/imaging/hgiVulkan/buffer.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"
#endif

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
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

// Everything a producer publishes about one shared buffer: how the consumer can
// reach the memory, and the semaphores ordering access to it. Which fields are
// set depends on the producer topology -- a producer on the consumer's own
// device offers native handles, one on its own device offers OS handles.
struct _SharedBuffer
{
    uint64_t rawHandle = 0;

    uint64_t externalMemoryHandle = 0;
    size_t   memoryBlockSize = 0;
    size_t   memoryOffset = 0;
    bool     dedicated = false;

    uint64_t writeSemaphore = 0;
    uint64_t readSemaphore = 0;
    uint64_t externalWriteSemaphore = 0;
    uint64_t externalReadSemaphore = 0;

    // The producer's physical device; empty when it is the consumer's own.
    std::string deviceUuid;

    // The logical device the producer minted its native handles in. Published
    // whether or not it is the consumer's own: that is the whole point of the
    // field, since a consumer cannot otherwise tell the two cases apart.
    uint64_t logicalDeviceId = 0;
};

// The external handle flavour this platform's Vulkan/GL interop uses.
TfToken
_ExternalHandleTypeToken()
{
#if defined(_WIN32)
    return HdExtGpuBufferSchemaTokens->opaqueWin32;
#else
    return HdExtGpuBufferSchemaTokens->opaqueFd;
#endif
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
    // as external GPU buffers created from `driver`'s Hgi and appended to
    // `buffers` (so the caller can free them); otherwise they are CPU primvars.
    void _BuildScene(HdSt_TestDriver *driver,
                     HdRetainedSceneIndexRefPtr &scene,
                     std::vector<HgiBufferHandle> &buffers,
                     bool gpuShare);

    // Build the cube primvars container (points + constant displayColor).
    HdContainerDataSourceHandle _BuildCubePrimvars(
        HdSt_TestDriver *driver,
        std::vector<HgiBufferHandle> &buffers,
        bool gpuShare);

    _SharedBuffer _MakeGpuBuffer(HdSt_TestDriver *driver,
                                 std::vector<HgiBufferHandle> &buffers,
                                 const void *data, size_t byteSize,
                                 uint32_t stride);

    // Allocate exportable memory on `producerHgi` (a second Vulkan device
    // standing in for a foreign-device producer), have GL import it and write
    // `data` into it, and create the RAW/WAR semaphore pair. The returned
    // descriptor offers the OS memory + semaphore handles for the consumer to
    // import, since the producer's VkBuffer is not interpretable on the
    // consumer's logical device.
    _SharedBuffer _MakeInteropBuffer(Hgi *producerHgi,
                                     std::vector<HgiBufferHandle> &buffers,
                                     std::vector<uint64_t> &semaphores,
                                     const void *data, size_t byteSize);

    // --vulkanSync producer: allocate the buffer on the consumer's own Vulkan
    // device, then write it by copying from a staging buffer in its own command
    // buffer, submitted independently on the device's graphics queue, signalling
    // a native WRITE semaphore. Storm adopts the VkBuffer (adopt route) and
    // orders its read against the producer's write through the RAW/WAR semaphore
    // pair. Only meaningful in Vulkan builds.
    _SharedBuffer _MakeVulkanSyncBuffer(HdSt_TestDriver *driver,
                                        std::vector<HgiBufferHandle> &buffers,
                                        const void *data, size_t byteSize,
                                        uint32_t stride);

    // Import `info`'s allocation into GL and write `data` into it through a
    // staging buffer. Returns the GL buffer aliasing the shared memory.
    uint32_t _GlImportAndWrite(const HgiInteropBufferInfo &info,
                               const void *data, size_t byteSize);

    // Import an OS semaphore handle into GL. Returns 0 if `osHandle` is 0.
    uint32_t _GlImportSemaphore(uint64_t osHandle);

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

    // Free any GL interop objects (memory objects + alias buffers) created by
    // the --vulkanInterop producer path.
    void _ReleaseGlInterop();

    GfVec3f _CameraTranslate() const {
        // Frame the single cube up close, the grid pulled back.
        return _instancing ? GfVec3f(0.0f, 0.0f, -30.0f)
                           : GfVec3f(0.0f, 0.0f, -8.0f);
    }

    // Interactive-mode driver (created lazily; not used by --offscreen).
    std::unique_ptr<HdSt_TestDriver> _driver;
    HdRetainedSceneIndexRefPtr _driverScene;
    std::vector<HgiBufferHandle> _driverBuffers;

    // Geometry parameters.
    bool _instancing = false;   // --instancing: grid of instanced cubes
    int _div = 3;               // grid is _div x _div cubes (instancing only)
    float _halfSize = 1.0f;
    float _spacing = 3.0f;

    // --vulkanSync: a Vulkan producer allocates the buffer on the CONSUMER's own
    // device and writes it in its own independent submission, signalling a native
    // WRITE semaphore. Storm adopts the VkBuffer (adopt route, logical device
    // matches) and its draw waits on that semaphore. Requires a Vulkan Hgi (run
    // with HGI_ENABLE_VULKAN=1).
    bool _vulkanSync = false;

    // --vulkanInterop: the memory is allocated on a second Vulkan device standing
    // in for a producer that owns its own device, so Storm has to import the OS
    // handle rather than adopt a VkBuffer.
    bool _vulkanInterop = false;

    // --copy: publish directBindable=false so the consumer copies (GPU->GPU
    // blit) the shared buffer into its own aggregated VBO instead of binding it
    // zero-copy. With --vulkanInterop this drives the import + blit path.
    bool _directBindable = true;

    HgiUniquePtr _producerHgi;
    std::vector<HgiBufferHandle> _producerBuffers;
    std::vector<uint64_t> _producerSemaphores;

    std::vector<uint32_t> _glInteropMemObjects;  // GL memory objects to delete
    std::vector<uint32_t> _glInteropBuffers;     // GL alias buffers to delete
    std::vector<uint32_t> _glInteropSemaphores;  // GL write semaphores to delete
    std::vector<uint32_t> _glReadSemaphores;     // GL read semaphores (WAR)
    std::vector<uint64_t> _extSemaphores;        // consumer-owned semaphores

    std::string _outputFilePath;  // --write: writes the GPU-shared image
    bool _writeCpu = false;       // --writeCpu: --write writes the CPU image
};

uint32_t
My_TestGLDrawing::_GlImportAndWrite(const HgiInteropBufferInfo &info,
                                    const void *data, size_t byteSize)
{
    // Import the Vulkan allocation into GL as a memory object + alias buffer
    // (see hgiInterop/vulkan.cpp for the reference recipe).
    GLuint memObj = 0;
    glCreateMemoryObjectsEXT(1, &memObj);
    GLint dedicated = info.dedicated ? GL_TRUE : GL_FALSE;
    glMemoryObjectParameterivEXT(
        memObj, GL_DEDICATED_MEMORY_OBJECT_EXT, &dedicated);
    glImportMemoryWin32HandleEXT(
        memObj, info.memoryBlockSize, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT,
        reinterpret_cast<void *>(
            static_cast<uintptr_t>(info.externalHandle)));

    GLuint glBuf = 0;
    glCreateBuffers(1, &glBuf);
    glNamedBufferStorageMemEXT(glBuf, byteSize, memObj, info.memoryOffset);

    // The producer writes the geometry via GL into the shared memory. The
    // external-memory buffer has immutable storage (no client-write flags), so
    // upload through a staging buffer + GPU-side copy rather than
    // glNamedBufferSubData.
    GLuint staging = 0;
    glCreateBuffers(1, &staging);
    glNamedBufferStorage(staging, byteSize, data, 0);
    glCopyNamedBufferSubData(staging, glBuf, 0, 0, byteSize);
    glDeleteBuffers(1, &staging);

    _glInteropMemObjects.push_back(memObj);
    _glInteropBuffers.push_back(glBuf);
    return glBuf;
}

uint32_t
My_TestGLDrawing::_GlImportSemaphore(uint64_t osHandle)
{
    if (!osHandle) {
        return 0;
    }
    GLuint glSem = 0;
    glGenSemaphoresEXT(1, &glSem);
    glImportSemaphoreWin32HandleEXT(
        glSem, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT,
        reinterpret_cast<void *>(static_cast<uintptr_t>(osHandle)));
    return glSem;
}

_SharedBuffer
My_TestGLDrawing::_MakeInteropBuffer(Hgi *producerHgi,
                                     std::vector<HgiBufferHandle> &buffers,
                                     std::vector<uint64_t> &semaphores,
                                     const void *data, size_t byteSize)
{
    _SharedBuffer shared;
    shared.logicalDeviceId = producerHgi->GetLogicalDeviceId();

    HgiInteropBufferInfo info;
    HgiBufferHandle vkBuf = producerHgi->CreateInteropBuffer(
        byteSize, HgiBufferUsageVertex | HgiBufferUsageStorage, &info);
    if (!vkBuf || info.externalHandle == 0) {
        TF_RUNTIME_ERROR("The --vulkanInterop mode needs a Vulkan Hgi with "
                         "external memory support (run with "
                         "HGI_ENABLE_VULKAN=1)");
        return shared;
    }

    // The VkBuffer belongs to the producer's logical device, so it is
    // meaningless to the consumer even though both resolve to the same physical
    // device. The OS-shareable allocation is the only route.
    //
    // On Win32 neither glImportMemoryWin32HandleEXT nor
    // VkImportMemoryWin32HandleInfoKHR takes ownership of an OPAQUE_WIN32
    // handle, so this one handle can serve both importers. (On Linux the fd
    // import DOES transfer ownership, so that would need a dup().)
    shared.externalMemoryHandle = info.externalHandle;
    shared.memoryBlockSize = info.memoryBlockSize;
    shared.memoryOffset = info.memoryOffset;
    shared.dedicated = info.dedicated;
    shared.deviceUuid = producerHgi->GetDeviceUuid();

    const GLuint glBuf = _GlImportAndWrite(info, data, byteSize);

    // Two exportable binary semaphores: a WRITE semaphore (the producer signals
    // it after the GL write, Storm waits before reading -- RAW) and a READ
    // semaphore (Storm signals it after reading, the producer waits before
    // overwriting -- WAR).
    uint64_t writeOsHandle = 0, readOsHandle = 0;
    const uint64_t writeSem =
        producerHgi->CreateExternalSemaphore(&writeOsHandle);
    const uint64_t readSem =
        producerHgi->CreateExternalSemaphore(&readOsHandle);
    if (writeSem) {
        semaphores.push_back(writeSem);
    }
    if (readSem) {
        semaphores.push_back(readSem);
    }
    const GLuint glWriteSem = _GlImportSemaphore(writeOsHandle);
    const GLuint glReadSem = _GlImportSemaphore(readOsHandle);

    if (glWriteSem) {
        // RAW: signal after the write (with a buffer barrier making the writes
        // available). Storm's consumer reads this from the schema and makes its
        // draw submit wait on it -- a GPU-side handshake.
        glSignalSemaphoreEXT(glWriteSem, 1, &glBuf, 0, nullptr, nullptr);
        glFlush();  // ensure the GL signal is submitted
        _glInteropSemaphores.push_back(glWriteSem);
        shared.externalWriteSemaphore = writeOsHandle;
    } else {
        // No external semaphore available: fall back to a coarse CPU sync.
        glFinish();
    }
    if (glReadSem) {
        _glReadSemaphores.push_back(glReadSem);
        shared.externalReadSemaphore = readOsHandle;
    }

    buffers.push_back(std::move(vkBuf));
    return shared;
}

_SharedBuffer
My_TestGLDrawing::_MakeVulkanSyncBuffer(HdSt_TestDriver *driver,
                                        std::vector<HgiBufferHandle> &buffers,
                                        const void *data, size_t byteSize,
                                        uint32_t stride)
{
    _SharedBuffer shared;
#if defined(PXR_VULKAN_SUPPORT_ENABLED)
    Hgi *hgi = driver->GetHgi();
    if (hgi->GetAPIName() != HgiTokens->Vulkan) {
        TF_RUNTIME_ERROR("--vulkanSync needs a Vulkan Hgi (run with "
                         "HGI_ENABLE_VULKAN=1)");
        return shared;
    }
    HgiVulkan *hgiVk = static_cast<HgiVulkan *>(hgi);
    HgiVulkanDevice *device = hgiVk->GetPrimaryDevice();
    VkDevice vkDevice = device->GetVulkanDevice();
    const uint32_t family = device->GetGfxQueueFamilyIndex();

    // HgiVulkan creates a single graphics queue and we deliberately do not change
    // that just for a test, so the producer records its OWN command buffer and
    // submits it independently on that same queue (index 0). The producer's write
    // and the consumer's read are still separate submissions ordered by the
    // semaphore below -- exactly what a real separate-queue producer would need.
    // (The submit is serialized on the main thread here, so sharing the queue
    // Hgi owns is safe.)
    VkQueue producerQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(vkDevice, family, 0, &producerQueue);

    // Destination vertex buffer on the consumer's own device. Storm adopts this
    // VkBuffer directly through rawHandle (the logical device matches), so no
    // memory import is involved -- only the write ordering matters.
    HgiBufferDesc dstDesc;
    dstDesc.usage = HgiBufferUsageVertex;
    dstDesc.byteSize = byteSize;
    dstDesc.vertexStride = stride;
    dstDesc.debugName = "vulkanSync dst";
    HgiBufferHandle dst = hgi->CreateBuffer(dstDesc);
    VkBuffer vkDst =
        static_cast<HgiVulkanBuffer *>(dst.Get())->GetVulkanBuffer();
    shared.rawHandle = dst->GetRawResource();

    // Host-visible staging buffer holding the geometry; the producer copies it
    // into the device-local vertex buffer on its own queue.
    HgiBufferDesc stgDesc;
    stgDesc.usage = HgiBufferUsageUpload;
    stgDesc.byteSize = byteSize;
    stgDesc.initialData = data;
    stgDesc.debugName = "vulkanSync staging";
    HgiBufferHandle staging = hgi->CreateBuffer(stgDesc);
    VkBuffer vkStaging =
        static_cast<HgiVulkanBuffer *>(staging.Get())->GetVulkanBuffer();

    // Transient command pool/buffer recording the producer's copy. Buffers are
    // VK_SHARING_MODE_EXCLUSIVE on the graphics family, but because the producer
    // submits on that same family no queue-family ownership transfer is needed;
    // the signal/wait semaphore alone makes the write available and visible.
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
    vkCmdCopyBuffer(cb, vkStaging, vkDst, 1, &region);
    vkEndCommandBuffer(cb);

    // RAW/WAR binary semaphores, native to this device. Producer and consumer
    // share one logical device, so the raw VkSemaphore handles travel through
    // the schema unchanged (no export). Encoded the same way HgiVulkan decodes
    // them, so the existing DestroyExternalSemaphore cleanup applies.
    VkSemaphoreCreateInfo sci = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkSemaphore vkWriteSem = VK_NULL_HANDLE;
    VkSemaphore vkReadSem = VK_NULL_HANDLE;
    vkCreateSemaphore(vkDevice, &sci, nullptr, &vkWriteSem);
    vkCreateSemaphore(vkDevice, &sci, nullptr, &vkReadSem);
    shared.writeSemaphore =
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(vkWriteSem));
    shared.readSemaphore =
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(vkReadSem));
    shared.logicalDeviceId = hgi->GetLogicalDeviceId();
    _extSemaphores.push_back(shared.writeSemaphore);
    _extSemaphores.push_back(shared.readSemaphore);

    // Submit the copy on the producer's queue, signalling the WRITE semaphore;
    // Storm's draw waits on it before reading (RAW). The host fence wait only
    // lets us reclaim the transient pool + staging buffer -- it does NOT consume
    // the binary semaphore, so the consumer still gets a genuine GPU handshake.
    VkSubmitInfo si = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cb;
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = &vkWriteSem;

    VkFence fence = VK_NULL_HANDLE;
    VkFenceCreateInfo fci = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    vkCreateFence(vkDevice, &fci, nullptr, &fence);
    vkQueueSubmit(producerQueue, 1, &si, fence);
    vkWaitForFences(vkDevice, 1, &fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(vkDevice, fence, nullptr);
    vkDestroyCommandPool(vkDevice, pool, nullptr);  // frees cb
    hgi->DestroyBuffer(&staging);

    // Storm adopts vkDst; keep the owning handle alive until teardown.
    buffers.push_back(std::move(dst));
    (void)vkDst;
#else
    (void)driver;
    (void)buffers;
    (void)data;
    (void)byteSize;
    (void)stride;
    TF_RUNTIME_ERROR("--vulkanSync requires a Vulkan-enabled build");
#endif
    return shared;
}

bool
My_TestGLDrawing::_EnsureProducerHgi(HdSt_TestDriver *driver)
{
    if (_producerHgi) {
        return true;
    }

    _producerHgi = Hgi::CreateNamedHgi(HgiTokens->Vulkan);
    if (!_producerHgi) {
        TF_RUNTIME_ERROR("--vulkanInterop could not create a second "
                         "Vulkan Hgi to stand in for the producer's device");
        return false;
    }

    // The point of the two-device topology: opaque external handles are only
    // importable on the physical device that exported them, so a producer that
    // picked a different GPU must fail loudly instead of rendering garbage.
    const std::string producerUuid = _producerHgi->GetDeviceUuid();
    const std::string consumerUuid = driver->GetHgi()->GetDeviceUuid();
    std::cout << "[extGpuBuffer] producer device = " << producerUuid
              << "\n[extGpuBuffer] consumer device = " << consumerUuid
              << std::endl;
    if (producerUuid.empty() || producerUuid != consumerUuid) {
        TF_RUNTIME_ERROR("--vulkanInterop requires both Vulkan "
                         "devices to resolve to the same physical device "
                         "(producer '%s' vs consumer '%s')",
                         producerUuid.c_str(), consumerUuid.c_str());
        _producerHgi.reset();
        return false;
    }
    return true;
}

void
My_TestGLDrawing::_ReleaseProducer()
{
    if (_producerHgi) {
        for (HgiBufferHandle &b : _producerBuffers) {
            _producerHgi->DestroyBuffer(&b);
        }
        for (uint64_t sem : _producerSemaphores) {
            _producerHgi->DestroyExternalSemaphore(sem);
        }
    }
    _producerBuffers.clear();
    _producerSemaphores.clear();
    _producerHgi.reset();
}

_SharedBuffer
My_TestGLDrawing::_MakeGpuBuffer(HdSt_TestDriver *driver,
                                 std::vector<HgiBufferHandle> &buffers,
                                 const void *data, size_t byteSize,
                                 uint32_t stride)
{
    if (_vulkanInterop) {
        if (!_EnsureProducerHgi(driver)) {
            return _SharedBuffer();
        }
        return _MakeInteropBuffer(_producerHgi.get(), _producerBuffers,
                                  _producerSemaphores, data, byteSize);
    }
    if (_vulkanSync) {
        return _MakeVulkanSyncBuffer(driver, buffers, data, byteSize, stride);
    }

    HgiBufferDesc desc;
    desc.usage = HgiBufferUsageVertex;
    desc.byteSize = byteSize;
    desc.vertexStride = stride;   // Hgi requires this for vertex buffers
    desc.initialData = data;

    HgiBufferHandle buffer = driver->GetHgi()->CreateBuffer(desc);

    _SharedBuffer shared;
    shared.rawHandle = buffer->GetRawResource();
    shared.logicalDeviceId = driver->GetHgi()->GetLogicalDeviceId();
    buffers.push_back(std::move(buffer));
    return shared;
}

void
My_TestGLDrawing::_ReleaseGlInterop()
{
    for (uint32_t s : _glInteropSemaphores) {
        glDeleteSemaphoresEXT(1, &s);
    }
    for (uint32_t s : _glReadSemaphores) {
        glDeleteSemaphoresEXT(1, &s);
    }
    for (uint32_t b : _glInteropBuffers) {
        glDeleteBuffers(1, &b);
    }
    for (uint32_t m : _glInteropMemObjects) {
        glDeleteMemoryObjectsEXT(1, &m);
    }
    _glInteropSemaphores.clear();
    _glReadSemaphores.clear();
    _glInteropBuffers.clear();
    _glInteropMemObjects.clear();
}

HdContainerDataSourceHandle
My_TestGLDrawing::_WithExtGpuBuffer(
    HdSt_TestDriver *driver,
    const HdContainerDataSourceHandle &primvar,
    const _SharedBuffer &shared, size_t byteSize,
    HdTupleType elementType, size_t numElements)
{
    using _U64Ds  = HdRetainedTypedSampledDataSource<uint64_t>;
    using _SizeDs = HdRetainedTypedSampledDataSource<size_t>;
    using _BoolDs = HdRetainedTypedSampledDataSource<bool>;
    using _TokenDs = HdRetainedTypedSampledDataSource<TfToken>;

    // backendApi must equal the consumer's Hgi->GetAPIName() (HgiTokens->OpenGL
    // for HgiGL) -- that is exactly what a real producer's _GetCurrentBackendApi
    // publishes.
    HdExtGpuBufferSchema::Builder builder;
    builder
        .SetBackendApi(_TokenDs::New(driver->GetHgi()->GetAPIName()))
        .SetRawHandleByteSize(_SizeDs::New(byteSize))
        .SetNumElements(_SizeDs::New(numElements))
        .SetElementType(
            HdRetainedTypedSampledDataSource<HdTupleType>::New(elementType))
        .SetByteOffset(_SizeDs::New(0))
        .SetByteStride(_SizeDs::New(0))
        .SetDirectBindable(_BoolDs::New(_directBindable));

    if (shared.rawHandle) {
        builder.SetRawHandle(_U64Ds::New(shared.rawHandle));
    }
    if (shared.externalMemoryHandle) {
        builder
            .SetExternalMemoryHandle(_U64Ds::New(shared.externalMemoryHandle))
            .SetExternalHandleType(
                HdExtGpuBufferSchema::BuildExternalHandleTypeDataSource(
                    _ExternalHandleTypeToken()))
            .SetMemoryBlockSize(_SizeDs::New(shared.memoryBlockSize))
            .SetMemoryOffset(_SizeDs::New(shared.memoryOffset))
            .SetDedicated(_BoolDs::New(shared.dedicated));
    }
    if (!shared.deviceUuid.empty()) {
        builder.SetDeviceUuid(_TokenDs::New(TfToken(shared.deviceUuid)));
    }
    if (shared.logicalDeviceId) {
        builder.SetLogicalDeviceId(_U64Ds::New(shared.logicalDeviceId));
    }
    HdContainerDataSourceHandle extGpuBuffer = builder.Build();

    // If the producer created sync semaphores for this buffer, publish them as
    // the extGpuBuffer's "sync" child so Storm's consumer orders the producer's
    // write before its read (RAW, write semaphore) and signals when its read
    // completes (WAR, read semaphore).
    const bool hasExternalSem = shared.externalWriteSemaphore ||
                                shared.externalReadSemaphore;
    if (shared.writeSemaphore || shared.readSemaphore || hasExternalSem) {
        HdExtGpuSyncSchema::Builder syncBuilder;
        syncBuilder.SetBackendApi(
            _TokenDs::New(driver->GetHgi()->GetAPIName()));
        if (shared.writeSemaphore) {
            syncBuilder.SetWriteSemaphore(_U64Ds::New(shared.writeSemaphore));
        }
        if (shared.readSemaphore) {
            syncBuilder.SetReadSemaphore(_U64Ds::New(shared.readSemaphore));
        }
        if (shared.externalWriteSemaphore) {
            syncBuilder.SetExternalWriteSemaphore(
                _U64Ds::New(shared.externalWriteSemaphore));
        }
        if (shared.externalReadSemaphore) {
            syncBuilder.SetExternalReadSemaphore(
                _U64Ds::New(shared.externalReadSemaphore));
        }
        // The implementation only supports binary semaphores; state it
        // explicitly rather than relying on the "absent means binary" default.
        syncBuilder.SetKind(_TokenDs::New(HdExtGpuSyncSchemaTokens->binary));
        if (hasExternalSem) {
            // External handles need their kind spelled out for the importer.
            syncBuilder.SetHandleType(_TokenDs::New(_ExternalHandleTypeToken()));
        }
        if (!shared.deviceUuid.empty()) {
            syncBuilder.SetDeviceUuid(
                _TokenDs::New(TfToken(shared.deviceUuid)));
        }
        if (shared.logicalDeviceId) {
            syncBuilder.SetLogicalDeviceId(
                _U64Ds::New(shared.logicalDeviceId));
        }
        extGpuBuffer = HdOverlayContainerDataSource::New(
            extGpuBuffer,
            HdRetainedContainerDataSource::New(
                HdExtGpuSyncSchema::GetSchemaToken(), syncBuilder.Build()));
    }

    return HdOverlayContainerDataSource::New(
        primvar,
        HdRetainedContainerDataSource::New(
            HdExtGpuBufferSchema::GetSchemaToken(), extGpuBuffer));
}

HdContainerDataSourceHandle
My_TestGLDrawing::_BuildCubePrimvars(HdSt_TestDriver *driver,
                                     std::vector<HgiBufferHandle> &buffers,
                                     bool gpuShare)
{
    const VtVec3fArray points = _CubePoints(_halfSize);

    HdContainerDataSourceHandle pointsPrimvar;
    if (gpuShare) {
        // GPU-only: empty CPU value + extGpuBuffer child pointing at a real
        // GL buffer holding the 8 cube corners.
        const size_t byteSize = points.size() * sizeof(GfVec3f);
        const _SharedBuffer shared =
            _MakeGpuBuffer(driver, buffers, points.cdata(), byteSize,
                           sizeof(GfVec3f));
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
                              std::vector<HgiBufferHandle> &buffers,
                              bool gpuShare)
{
    scene = HdRetainedSceneIndex::New();

    const SdfPath cubePath("/cube");
    const SdfPath instancerPath("/instancer");

    HdContainerDataSourceHandle primvarsDs =
        _BuildCubePrimvars(driver, buffers, gpuShare);

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
            _MakeGpuBuffer(driver, buffers, matricesF.data(), byteSize,
                           sizeof(GfMatrix4f));
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
My_TestGLDrawing::_RenderToPixels(bool gpuShare, std::vector<uint8_t> &out,
                                  int &width, int &height,
                                  const std::string &writePath)
{
    const int w = GetWidth(), h = GetHeight();

    auto driver = std::make_unique<HdSt_TestDriver>(HdReprTokens->hull);

    // Report the backend Storm actually runs on (the harness always creates a
    // GL context for its window, so the GL vendor banner is NOT the Storm Hgi).
    if (gpuShare) {
        std::cout << "[extGpuBuffer] Storm Hgi backend = "
                  << driver->GetHgi()->GetAPIName().GetString() << std::endl;
    }

    HdRetainedSceneIndexRefPtr scene;
    std::vector<HgiBufferHandle> buffers;
    _BuildScene(driver.get(), scene, buffers, gpuShare);

    // Feed the geometry through a scene index so the extGpuBuffer child
    // survives to the terminal scene index the consumer reads from.
    driver->GetDelegate().GetRenderIndex().InsertSceneIndex(
        scene, SdfPath::AbsoluteRootPath());

    driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    driver->SetClearDepth(1.0f);
    driver->SetupAovs(w, h);
    driver->SetCamera(GetViewMatrix(), GetProjectionMatrix(),
                      CameraUtilFraming(GfRect2i(GfVec2i(0, 0), w, h)));

    // RAW ordering is now driven by the schema: Storm's consumer reads the
    // extGpuBuffer's "sync" child and enqueues the wait during Sync, so the
    // test no longer calls QueueWaitExternalSemaphore itself.
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

    // WAR verification: the producer waits (on the GPU) for each read semaphore
    // the consumer signalled after its read, then finishes -- this is where a
    // real producer would gate its next overwrite of the shared buffer. A
    // missing consumer signal (broken WAR wiring) would hang glFinish.
    if (!_glReadSemaphores.empty()) {
        for (uint32_t glReadSem : _glReadSemaphores) {
            glWaitSemaphoreEXT(
                glReadSem,
                static_cast<uint32_t>(_glInteropBuffers.size()),
                _glInteropBuffers.data(), 0, nullptr, nullptr);
        }
        glFinish();
    }

    // Release the GL alias objects (which reference the shared memory) BEFORE
    // destroying the Vulkan buffer that owns that memory.
    _ReleaseGlInterop();

    // The external GPU buffers are non-owning in Storm; free the real GPU
    // resources we allocated (before this driver's Hgi is destroyed).
    Hgi *hgi = driver->GetHgi();
    for (HgiBufferHandle &b : buffers) {
        hgi->DestroyBuffer(&b);
    }
    for (uint64_t sem : _extSemaphores) {
        hgi->DestroyExternalSemaphore(sem);
    }
    _extSemaphores.clear();

    // The consumer's resource registry holds the buffers it imported from the
    // producer's memory, so drop the whole consumer before the producer device
    // that allocated that memory.
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
        _BuildScene(_driver.get(), _driverScene, _driverBuffers,
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
    _ReleaseGlInterop();
    if (_driver) {
        Hgi *hgi = _driver->GetHgi();
        for (HgiBufferHandle &b : _driverBuffers) {
            hgi->DestroyBuffer(&b);
        }
        for (uint64_t sem : _extSemaphores) {
            hgi->DestroyExternalSemaphore(sem);
        }
    }
    _driverBuffers.clear();
    _extSemaphores.clear();
    // Same ordering rule as _RenderToPixels: the consumer's imports reference
    // the producer's memory, so the consumer goes first.
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
            _directBindable = false;
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
