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
    // \p reuseExisting republishes into the scene index already inserted in
    // the render index, instead of making a new one. Required for frame 2 on
    // the CPU path: a fresh index would never be inserted, so the render
    // index would keep drawing frame 1 and the baseline would be stale.
    void _BuildScene(HdSt_TestDriver *driver,
                     HdRetainedSceneIndexRefPtr &scene,
                     bool gpuShare,
                     bool reuseExisting = false);

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

    // ---- Frame 2: overwrite an already-shared buffer IN PLACE ----
    //
    // In place rather than republished, deliberately. A producer that
    // allocates a second buffer and publishes that has no write-after-read
    // hazard at all, because nothing ever rewrites bytes a submitted draw may
    // still be reading -- it is safe by construction and proves nothing.
    // Overwriting is what a viewport actually does for deforming geometry,
    // and the case the arena's hgi-done signal exists to make safe.
    //
    // \p stream indexes _producerStreams in creation order. Returns false on
    // failure; the ordering each topology can express differs, see below.
    bool _UpdateGpuBuffer(HdSt_TestDriver *driver, size_t stream,
                          const void *data, size_t byteSize);

    // GL producer: rewrites its own buffer on the context it shares with
    // Storm, where GL's in-order command stream orders the write after the
    // draw that read it, with no synchronization primitive at all.
    bool _UpdateNativeRegisteredBuffer(size_t stream,
                                       const void *data, size_t byteSize);

#if defined(PXR_VULKAN_SUPPORT_ENABLED)
    // Vulkan producer: rewrites the VkBuffer in its own submission. Waits on
    // the arena's hgi-done semaphore first, which is the whole point -- that
    // signal is the producer's only evidence Storm has finished reading, and
    // whether it is emitted before or after the draws is what these frames
    // are here to find out.
    bool _UpdateVulkanBuffer(HdSt_TestDriver *driver, size_t stream,
                             const void *data, size_t byteSize);
#endif

    // What the producer needs to rewrite a stream it already shared, parallel
    // to _sharedBuffers by construction order.
    //
    // Deliberately holds NO strong reference to the arena wrapper: the reclaim
    // test watches use_count, so a second owner here would mask exactly the
    // transition it asserts on.
    struct _ProducerStream
    {
        size_t   byteSize = 0;
        uint32_t glBuffer = 0;            // GL producer's own name
#if defined(PXR_VULKAN_SUPPORT_ENABLED)
        VkBuffer vkBuffer = VK_NULL_HANDLE;  // Vulkan producer's own buffer
        Hgi     *vkOwner  = nullptr;         // the Hgi whose device minted it
#endif
        // The CONSUMER's arena, whose epoch the producer opens after writing.
        HgiExternalBufferArena *consumerArena = nullptr;
    };
    std::vector<_ProducerStream> _producerStreams;

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

#endif

#if defined(PXR_VULKAN_SUPPORT_ENABLED)
    // Get-or-create the Vulkan arena for \p hgi, cached in \p slot. Used for
    // both the consumer's device and the producer's.
    HgiVulkanExternalBufferArena *_GetVulkanArena(
        Hgi *hgi,
        std::shared_ptr<HgiVulkanExternalBufferArena> *slot);

    // Write \p data into \p dst as the producer's own work on \p hgi's
    // device, optionally signalling \p signalSemaphore when it completes.
    // \p waitSemaphore, when given, is waited on by the copy submission --
    // the write-after-read half of the bracket. \p signalSemaphore is signalled
    // after it, the read-after-write half.
    bool _VulkanUpload(Hgi *hgi, VkBuffer dst,
                       const void *data, size_t byteSize,
                       VkSemaphore signalSemaphore,
                       VkSemaphore waitSemaphore = VK_NULL_HANDLE);
#endif

    HgiGLExternalBufferArena *_GetGlArena(HdSt_TestDriver *driver);

    // Record that this build, platform or device cannot do what the requested
    // mode needs, and let the run unwind quietly.
    //
    // Deliberately not an error. External-memory interop is a capability, not
    // a correctness property: a machine whose driver lacks
    // VK_KHR_external_memory_fd, or a GL stack without GL_EXT_memory_object_fd,
    // is simply not a machine this feature applies to, and failing there would
    // make the suite red for something no change to the code under test could
    // fix. A skip that says why is the honest result.
    //
    // Only the first reason is kept: everything after it is a consequence.
    //
    // The risk this accepts is that a genuine regression which happens to
    // present as "no arena" reads as a skip. That is why the sites calling
    // this are only the ones that answer "is the capability there", never the
    // ones that answer "did the operation work".
    void _MarkUnsupported(std::string const &why) {
        if (_unsupported.empty()) {
            _unsupported = why;
        }
    }
    std::string _unsupported;

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

    // Render TWO frames with different cube geometry and return both images.
    //
    // Two frames rather than one because sharing exists to be updated: a
    // buffer written once and never touched again has no write-after-read
    // hazard, needs no hgi-done signal to be correct, and is indistinguishable
    // from an ordinary Storm-owned VBO. Frame 2 overwrites the SAME buffer in
    // place, which is what a viewport does for deforming geometry.
    void _RenderToPixels(bool gpuShare,
                         std::vector<uint8_t> &frame1,
                         std::vector<uint8_t> &frame2,
                         int &width, int &height,
                         const std::string &writePath);

    // Read the color AOV into \p out. Forces a GPU sync.
    void _ReadColorAov(HdSt_TestDriver *driver, std::vector<uint8_t> &out,
                       int &width, int &height);

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

    // Frame 2's cube. Smaller on purpose: the extent published in frame 1 is
    // built from _halfSize and is not republished for the GPU path (only the
    // buffer contents change), so a frame 2 that grew could be clipped by a
    // stale extent and we would be debugging culling instead of ordering.
    float _halfSize2 = 0.6f;

    // The prim carrying the points primvar -- the cube, or the prototype when
    // instancing. Recorded by _BuildScene so frame 2 can dirty it.
    SdfPath _meshPrimPath;
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
            ->GetExternalBufferArena<HgiGLExternalBufferArena>();
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
        *slot = hgi->GetExternalBufferArena<HgiVulkanExternalBufferArena>();
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
                                              uint32_t /*stride*/)
{
    // The GL producer: the application creates its own GL buffer on the
    // consumer's context and REGISTERS it, so the arena binds and reads it but
    // never deletes it.
    //
    // GL only, deliberately. A Vulkan consumer used to be handled here as
    // well, by letting Hgi upload through HgiBufferDesc::initialData and
    // registering the resulting VkBuffer. That made Hgi the writer, so Hgi's
    // own staging barrier ordered the write and nothing ever overwrote the
    // buffer afterwards -- it exercised registration and binding while proving
    // nothing about a producer, and Vulkan orders nothing implicitly even on
    // one queue. Use --vulkanSync instead, where the producer submits its own
    // command buffer and the only thing ordering it against Storm's read is
    // the arena's semaphore pair.
    HgiGLExternalBufferArena *arena = _GetGlArena(driver);
    if (!arena) {
        TF_RUNTIME_ERROR(
            "This Hgi cannot consume OpenGL buffers. A Vulkan consumer "
            "needs --vulkanSync or --vulkanInterop; the default path is a "
            "GL producer, and no arena can bridge a GL name to a Vulkan "
            "consumer because OpenGL cannot export an allocation for "
            "another API to import.");
        return nullptr;
    }

    // The application's own GL buffer, created and owned by the application.
    GLuint glBuf = 0;
    glCreateBuffers(1, &glBuf);
    glNamedBufferData(glBuf, byteSize, data, GL_STATIC_DRAW);
    _appGlBuffers.push_back(glBuf);

    _SharedBuffer shared = arena->RegisterBuffer(
        glBuf, byteSize, HgiBufferUsageVertex | HgiBufferUsageStorage);
    if (shared) {
        // Recorded only on success, so _producerStreams stays index-aligned
        // with _sharedBuffers, which the caller pushes only on success too.
        _ProducerStream rec;
        rec.byteSize = byteSize;
        rec.glBuffer = glBuf;
        _producerStreams.push_back(rec);
    }
    return shared;
}

bool
My_TestGLDrawing::_UpdateNativeRegisteredBuffer(size_t stream,
                                                const void *data,
                                                size_t byteSize)
{
    if (stream >= _producerStreams.size()) {
        TF_RUNTIME_ERROR("No producer stream %zu to update", stream);
        return false;
    }
    const _ProducerStream &s = _producerStreams[stream];
    if (s.glBuffer == 0) {
        TF_RUNTIME_ERROR("Producer stream %zu is not a GL buffer", stream);
        return false;
    }
    if (byteSize != s.byteSize) {
        // Resizing would be a republish, not an overwrite, and a republished
        // buffer has no write-after-read hazard to expose.
        TF_RUNTIME_ERROR("Frame 2 must overwrite stream %zu in place "
                         "(%zu bytes), not resize it to %zu",
                         stream, s.byteSize, byteSize);
        return false;
    }

    // No synchronization, and none is required. This is the application's own
    // buffer on the very context Storm draws in, and GL processes a single
    // context's commands in order -- so this write is ordered after the draw
    // that read the previous contents, with no primitive involved. That
    // guarantee is exactly why a GL arena defaults to having no semaphores,
    // and why this topology cannot exhibit the write-after-read hazard the
    // Vulkan producers below are here to probe.
    glNamedBufferSubData(s.glBuffer, 0, GLsizeiptr(byteSize), data);
    return true;
}

#if defined(PXR_VULKAN_SUPPORT_ENABLED)
bool
My_TestGLDrawing::_UpdateVulkanBuffer(HdSt_TestDriver *driver, size_t stream,
                                      const void *data, size_t byteSize)
{
    (void)driver;
    if (stream >= _producerStreams.size()) {
        TF_RUNTIME_ERROR("No producer stream %zu to update", stream);
        return false;
    }
    const _ProducerStream &s = _producerStreams[stream];
    if (s.vkBuffer == VK_NULL_HANDLE || !s.vkOwner) {
        TF_RUNTIME_ERROR("Producer stream %zu is not a VkBuffer", stream);
        return false;
    }
    if (byteSize != s.byteSize) {
        TF_RUNTIME_ERROR("Frame 2 must overwrite stream %zu in place "
                         "(%zu bytes), not resize it to %zu",
                         stream, s.byteSize, byteSize);
        return false;
    }

    // The bracket, in the order the API documents it: WAIT for hgi-done, then
    // write, then signal app-done.
    //
    // The wait is the whole experiment. hgi-done is the producer's only
    // evidence that the consumer has finished reading what it drew last
    // frame, so a correct signal makes this wait block until those draws
    // complete. A signal emitted before the draws are even submitted makes it
    // return immediately, and the overwrite below lands in a buffer those
    // draws are still reading. Nothing here departs from the documented
    // protocol -- which is the point: the producer does everything right.
    if (!_VulkanUpload(s.vkOwner, s.vkBuffer, data, byteSize,
                       _appDoneVkSemaphore, _hgiDoneVkSemaphore)) {
        return false;
    }

    // Open the next epoch so the consumer's commit encodes a wait for this
    // write rather than assuming the producer is idle.
    if (_appDoneVkSemaphore != VK_NULL_HANDLE && s.consumerArena) {
        s.consumerArena->NotifyAppDone();
    }
    return true;
}
#endif

#if defined(PXR_VULKAN_SUPPORT_ENABLED)
bool
My_TestGLDrawing::_VulkanUpload(Hgi *hgi, VkBuffer dst,
                                const void *data, size_t byteSize,
                                VkSemaphore signalSemaphore,
                                VkSemaphore waitSemaphore)
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
    // Gate the copy on the GPU rather than the host, so the overwrite becomes
    // runnable the instant the consumer claims it has finished reading. That
    // is the moment under test.
    const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    if (waitSemaphore != VK_NULL_HANDLE) {
        si.waitSemaphoreCount = 1;
        si.pWaitSemaphores = &waitSemaphore;
        si.pWaitDstStageMask = &waitStage;
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

    // Bounded only when we gated on a semaphore. Unbounded is right for an
    // ungated upload -- it always completes -- but a gated one is waiting on
    // the consumer, and a consumer that signals late or never must fail the
    // test rather than hang it. Fatal rather than reported: the submission
    // is still pending on a wait that may never complete, so teardown would
    // block in vkDeviceWaitIdle, and a Vulkan submission cannot be cancelled.
    const uint64_t timeoutNs = (waitSemaphore != VK_NULL_HANDLE)
        ? 5ull * 1000ull * 1000ull * 1000ull
        : UINT64_MAX;
    if (vkWaitForFences(vkDevice, 1, &fence, VK_TRUE, timeoutNs)
            != VK_SUCCESS) {
        TF_FATAL_ERROR(
            "The producer's write never became runnable (waited %.1fs on the "
            "arena's hgi-done semaphore). Either the consumer never signalled "
            "it, or it signalled too late for a producer that waits before "
            "writing -- which is where the API documents the wait belongs.",
            double(timeoutNs) / 1e9);
    }
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
    _appDoneVkSemaphore = producerArena->GetAppDoneVkSemaphore();
    _hgiDoneVkSemaphore = producerArena->GetHgiDoneVkSemaphore();
    _semaphoreOwnerHgi = _producerHgi.get();
    _interopSemaphoresActive = true;
    std::cout << "[extGpuBuffer] semaphore pair shared producer -> consumer\n";
    return true;
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
        _MarkUnsupported("--vulkanInterop needs a Vulkan producer with "
                         "external memory support "
                         "(VK_KHR_external_memory_fd / _win32)");
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
        _MarkUnsupported("--vulkanInterop consumer has no arena to import "
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

    // Recorded on success only, so _producerStreams stays index-aligned with
    // _sharedBuffers. What frame 2 rewrites is the PRODUCER's own VkBuffer --
    // the consumer's imported buffer is a different object aliasing the same
    // memory, and writing through it would be the consumer overwriting its
    // own input rather than a producer doing so.
    auto recordStream = [&](_SharedBuffer const &b) -> _SharedBuffer {
        if (b) {
            _ProducerStream rec;
            rec.byteSize = byteSize;
            rec.vkBuffer = producerVkBuffer->GetVulkanBuffer();
            rec.vkOwner = _producerHgi.get();
            rec.consumerArena = consumerArena;
            _producerStreams.push_back(rec);
        }
        return b;
    };

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
            _MarkUnsupported("--vulkanInterop could not import the producer's "
                             "allocation into the consumer's device");
        }
        return recordStream(imported);
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
        // The GL side of the capability question, which no Vulkan check can
        // answer: GL_EXT_memory_object_fd / _win32 may simply be absent.
        _MarkUnsupported("--vulkanInterop could not import the producer's "
                         "allocation into OpenGL (GL_EXT_memory_object_fd / "
                         "_win32 unavailable?)");
    }
    return recordStream(imported);
#else
    (void)driver;
    (void)data;
    (void)byteSize;
    _MarkUnsupported("--vulkanInterop requires a Vulkan-enabled build");
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
        _MarkUnsupported("--vulkanSync needs a Vulkan Hgi (run with "
                         "HGI_ENABLE_VULKAN=1)");
        return nullptr;
    }
    HgiVulkanExternalBufferArena *arena = _GetVulkanArena(hgi, &_vkArena);
    if (!arena) {
        _MarkUnsupported("--vulkanSync needs a Vulkan device with external "
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
        // Not exportable: the producer is on the consumer's own logical
        // device, so it reads the pair natively and no OS handle is needed.
        // Asking for exportable semaphores here would mint two handles and
        // throw them away.
        if (!arena->CreateSemaphores(HgiSemaphoreKindBinary)) {
            TF_RUNTIME_ERROR("--vulkanSync could not create the arena's "
                             "semaphore pair");
            return nullptr;
        }
        // Hgi owns the pair and the application uses it natively -- the
        // opposite direction from the interop cases, where the producer
        // creates and exports and the consumer imports.
        _appDoneVkSemaphore = arena->GetAppDoneVkSemaphore();
        _hgiDoneVkSemaphore = arena->GetHgiDoneVkSemaphore();
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
    _SharedBuffer shared = arena->RegisterBuffer(
        vkDst, byteSize, HgiBufferUsageVertex | HgiBufferUsageStorage);
    if (shared) {
        _ProducerStream rec;
        rec.byteSize = byteSize;
        rec.vkBuffer = vkDst;
        rec.vkOwner = hgi;
        rec.consumerArena = arena;
        _producerStreams.push_back(rec);
    }
    return shared;
#else
    (void)driver;
    (void)data;
    (void)byteSize;
    (void)stride;
    _MarkUnsupported("--vulkanSync requires a Vulkan-enabled build");
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
        _MarkUnsupported("--vulkanInterop could not create a second Vulkan "
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

bool
My_TestGLDrawing::_UpdateGpuBuffer(HdSt_TestDriver *driver, size_t stream,
                                   const void *data, size_t byteSize)
{
#if defined(PXR_VULKAN_SUPPORT_ENABLED)
    if (_vulkanInterop || _vulkanSync) {
        return _UpdateVulkanBuffer(driver, stream, data, byteSize);
    }
#endif
    return _UpdateNativeRegisteredBuffer(stream, data, byteSize);
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
            // Silent when the mode was already found unsupported: the null is
            // that verdict propagating, not a second, independent failure.
            if (_unsupported.empty()) {
                TF_RUNTIME_ERROR("Could not share the cube points");
            }
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
                              bool gpuShare,
                              bool reuseExisting)
{
    if (!reuseExisting || !scene) {
        scene = HdRetainedSceneIndex::New();
    }

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
        _meshPrimPath = cubePath;
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
            if (_unsupported.empty()) {
                TF_RUNTIME_ERROR("Could not share the instance transforms");
            }
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
    // The prototype is what carries the points primvar frame 2 rewrites.
    _meshPrimPath = cubePath;
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
    driver->GetHgi()->StartFrame();
    driver->Draw();
    driver->GetHgi()->EndFrame();

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

    // Collection releases it. A full frame per iteration as well as an
    // explicit collect, because an application reclaims as it renders and
    // that is the behaviour worth asserting.
    //
    // The explicit GarbageCollect() is not redundant with EndFrame, which also
    // sweeps the arenas. It is the assertion that reclamation does not *only*
    // happen on the frame hooks: a client that drives Hydra with its own task
    // list and calls GarbageCollect alone must still get its buffers back.
    //
    // Bounded: lagging a few frames is the safe direction, never releasing is
    // a leak.
    Hgi *hgi = driver->GetHgi();
    const int maxFrames = 8;
    int frames = 0;
    for (; frames < maxFrames && !weak.expired(); ++frames) {
        hgi->StartFrame();
        driver->Draw();
        hgi->EndFrame();
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
My_TestGLDrawing::_ReadColorAov(HdSt_TestDriver *driver,
                                std::vector<uint8_t> &out,
                                int &width, int &height)
{
    HdRenderBuffer *rb = dynamic_cast<HdRenderBuffer *>(
        driver->GetDelegate().GetRenderIndex().GetBprim(
            HdPrimTypeTokens->renderBuffer, _colorAovId));
    if (!rb) {
        TF_RUNTIME_ERROR("No color render buffer to read back");
        return;
    }
    width = rb->GetWidth();
    height = rb->GetHeight();
    const size_t bpp = HdDataSizeOfFormat(rb->GetFormat());
    const uint8_t *data = static_cast<const uint8_t *>(rb->Map());
    out.assign(data, data + size_t(width) * height * bpp);
    rb->Unmap();
}

void
My_TestGLDrawing::_RenderToPixels(bool gpuShare,
                                  std::vector<uint8_t> &frame1,
                                  std::vector<uint8_t> &frame2,
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

    // ---- Frame 1 ----
    //
    // Both halves of the bracket are ours to declare, because we are the host.
    // HdSt_TestDriver's task list has no HdxAovInputTask or HdxPresentTask, so
    // nothing emits these hooks for us -- which is how a real application is
    // supposed to drive Hgi anyway.
    //
    // StartFrame encodes the arena's app-done wait, ordering the producer's
    // writes ahead of the draws below. EndFrame encodes the hgi-done signal,
    // and it must come after Draw(): the reads of a directly bound buffer ARE
    // the draws, so a signal ahead of them would tell the producer the frame
    // had finished reading bytes it had not yet touched.
    driver->GetHgi()->StartFrame();
    driver->Draw();
    driver->GetHgi()->EndFrame();

    // ---- The producer overwrites, BEFORE frame 1 is read back ----
    //
    // Ordering here is the whole experiment, so it is worth being explicit
    // about why these two statements are in this order.
    //
    // Draw() does not wait for the GPU. It submits, and returns while frame
    // 1's draws may still be executing. The producer then does exactly what
    // the API documents: waits for hgi-done -- its only evidence that the
    // consumer has finished reading -- and overwrites the buffer. Reading the
    // AOV back afterwards forces the sync, so whatever the draws actually saw
    // is what lands in `frame1`.
    //
    // Put the readback first and the hazard disappears, because the readback
    // would drain frame 1 before the producer touched anything. That would be
    // a test that cannot fail rather than one that passes.
    if (gpuShare && !_producerStreams.empty()) {
        const VtVec3fArray pts2 = _CubePoints(_halfSize2);
        _UpdateGpuBuffer(driver.get(), /*stream*/0, pts2.cdata(),
                         pts2.size() * sizeof(GfVec3f));
    }

    if (!writePath.empty()) {
        driver->WriteToFile("color", writePath);
    }
    _ReadColorAov(driver.get(), frame1, width, height);

    // ---- Frame 2 ----
    //
    // The GPU path publishes nothing new: same buffer, same element count,
    // only different bytes. Dirtying the points locator is what makes Storm
    // re-read it, which the copy topologies need in order to re-blit. Under
    // direct binding it is redundant -- the range already points at this
    // buffer -- and that redundancy is itself worth having, because if frame
    // 2 came out wrong without it we would know the binding was not direct.
    if (gpuShare) {
        if (!_meshPrimPath.IsEmpty()) {
            scene->DirtyPrims({{_meshPrimPath,
                HdDataSourceLocatorSet(HdDataSourceLocator(
                    HdPrimvarsSchemaTokens->primvars,
                    HdPrimvarsSchemaTokens->points))}});
        }
    } else {
        // The CPU baseline shares nothing, so republishing with the frame 2
        // geometry is the cheapest way to get an image to compare against.
        //
        // Into the SAME scene index, not a new one: only the index inserted
        // into the render index back at setup is ever drawn, so a fresh one
        // would leave the baseline showing frame 1 while the GPU path moved
        // on -- which looks exactly like a GPU bug and is not one.
        const float saved = _halfSize;
        _halfSize = _halfSize2;
        _BuildScene(driver.get(), scene, /*gpuShare*/false,
                    /*reuseExisting*/true);
        _halfSize = saved;
        if (!_meshPrimPath.IsEmpty()) {
            scene->DirtyPrims({{_meshPrimPath,
                HdDataSourceLocatorSet(HdDataSourceLocator(
                    HdPrimvarsSchemaTokens->primvars,
                    HdPrimvarsSchemaTokens->points))}});
        }
    }

    driver->GetHgi()->StartFrame();
    driver->Draw();
    driver->GetHgi()->EndFrame();
    _ReadColorAov(driver.get(), frame2, width, height);

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

    std::vector<uint8_t> cpu1, cpu2, gpu1, gpu2;
    int cw = 0, ch = 0, gw = 0, gh = 0;

    _RenderToPixels(/*gpuShare*/false, cpu1, cpu2, cw, ch,
                    _writeCpu ? _outputFilePath : std::string());
    _RenderToPixels(/*gpuShare*/true, gpu1, gpu2, gw, gh,
                    _writeCpu ? std::string() : _outputFilePath);

    // Checked before any comparison, and that order matters. When sharing is
    // unavailable the GPU run silently falls back to CPU primvars, so the two
    // images match and the comparison below would PASS -- reporting success
    // for a run that exercised nothing. The skip has to pre-empt it.
    if (!_unsupported.empty()) {
        std::cout << "SKIPPED: " << _unsupported << std::endl;
        return;
    }

    if (cw != gw || ch != gh) {
        TF_RUNTIME_ERROR("CPU/GPU image dimensions differ: %dx%d vs %dx%d",
                         cw, ch, gw, gh);
        return;
    }

    // Frame 1 is the interesting one. If the consumer signalled hgi-done
    // before the draws that read the buffer, the producer's frame 2 overwrite
    // landed underneath those draws and frame 1 renders frame 2's geometry --
    // which is a large, unmistakable divergence rather than a subtle one,
    // because the two cubes differ in size.
    //
    // Frame 2 diverging instead would mean something else: either the update
    // never reached the consumer, or it was not re-read.
    struct _Pair { const char *name;
                   std::vector<uint8_t> const *cpu, *gpu; };
    const _Pair pairs[2] = { {"frame1", &cpu1, &gpu1},
                             {"frame2", &cpu2, &gpu2} };

    for (_Pair const &pr : pairs) {
        if (pr.cpu->empty() || pr.gpu->empty() ||
                pr.cpu->size() != pr.gpu->size()) {
            TF_RUNTIME_ERROR("%s: readback produced no comparable pixels "
                             "(cpu=%zu gpu=%zu)",
                             pr.name, pr.cpu->size(), pr.gpu->size());
            continue;
        }

        // Both images come from the same GPU in the same run, so a correct
        // GPU path is bit-identical. Allow a tiny per-channel slack only to
        // be safe.
        const int kTolerance = 2;
        size_t diffBytes = 0;
        int maxDiff = 0;
        for (size_t i = 0; i < pr.gpu->size(); ++i) {
            const int diff =
                std::abs(int((*pr.gpu)[i]) - int((*pr.cpu)[i]));
            if (diff > maxDiff) {
                maxDiff = diff;
            }
            if (diff > kTolerance) {
                ++diffBytes;
            }
        }

        const double diffFraction =
            double(diffBytes) / double(pr.gpu->size());
        std::cout << (_instancing ? "[instancing] " : "[basic] ")
                  << pr.name << " CPU vs GPU: maxDiff=" << maxDiff
                  << " diffBytes=" << diffBytes
                  << " (" << (diffFraction * 100.0) << "%)\n";

        if (diffFraction > 0.001) {
            TF_RUNTIME_ERROR("%s: GPU-shared render differs from the CPU "
                             "baseline: %zu bytes (%.3f%%) exceed tolerance, "
                             "maxDiff=%d",
                             pr.name, diffBytes, diffFraction * 100.0,
                             maxDiff);
        }
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
    _driver->GetHgi()->StartFrame();
    _driver->Draw();
    _driver->GetHgi()->EndFrame();
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
