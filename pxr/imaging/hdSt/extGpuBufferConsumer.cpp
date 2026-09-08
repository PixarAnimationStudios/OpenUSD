//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/extGpuBufferConsumer.h"

#include "pxr/imaging/hdSt/extBufferDesc.h"
#include "pxr/imaging/hdSt/extGpuBufferArrayRange.h"
#include "pxr/imaging/hdSt/extGpuBufferSource.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/extGpuSyncSchema.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/imaging/hgi/hgi.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

// True when \p producerUuid names the physical device \p hgi renders on. An
// absent UUID on either side means "unknown", which is treated as a match so
// producers that predate the import cluster keep taking the adopt path.
static bool
_DeviceMatches(TfToken const &producerUuid, Hgi *hgi)
{
    if (producerUuid.IsEmpty()) {
        return true;
    }
    const std::string consumerUuid = hgi->GetDeviceUuid();
    return consumerUuid.empty() || producerUuid == consumerUuid;
}

// True when \p producerId names the logical device whose handle namespace this
// consumer shares, so a native handle minted there means the same thing here.
// A matching physical device is not enough: two logical devices on one GPU hand
// out unrelated handles, and adopting across them binds an arbitrary object. A 0
// on either side means "unknown" and is treated as a match, both for producers
// that predate the field and for backends with no logical device to report.
static bool
_LogicalDeviceMatches(uint64_t producerId, Hgi *hgi)
{
    if (producerId == 0) {
        return true;
    }
    const uint64_t consumerId = hgi->GetLogicalDeviceId();
    return consumerId == 0 || producerId == consumerId;
}

// Resolves one semaphore of a sync container to a handle usable on this
// backend, preferring the native handle when the producer shares our device and
// importing the OS handle otherwise. Returns 0 when neither is available.
static uint64_t
_ResolveSemaphore(
    HdExtGpuSyncSchema const &sync,
    HdStResourceRegistry *registry,
    Hgi *hgi,
    HdUInt64DataSourceHandle const &nativeDs,
    HdUInt64DataSourceHandle const &externalDs)
{
    const HdTokenDataSourceHandle uuidDs = sync.GetDeviceUuid();
    const TfToken producerUuid =
        uuidDs ? uuidDs->GetTypedValue(0.0f) : TfToken();
    const HdUInt64DataSourceHandle logicalIdDs = sync.GetLogicalDeviceId();
    const uint64_t producerLogicalId =
        logicalIdDs ? logicalIdDs->GetTypedValue(0.0f) : 0;

    // A native semaphore handle is namespaced exactly like a native buffer
    // handle, so it takes the same pair of checks before we use it.
    if (nativeDs && _DeviceMatches(producerUuid, hgi) &&
            _LogicalDeviceMatches(producerLogicalId, hgi)) {
        if (const uint64_t native = nativeDs->GetTypedValue(0.0f)) {
            return native;
        }
    }
    if (!externalDs) {
        return 0;
    }
    const uint64_t externalHandle = externalDs->GetTypedValue(0.0f);
    if (!externalHandle) {
        return 0;
    }

    const HdTokenDataSourceHandle handleTypeDs = sync.GetHandleType();
    const HdTokenDataSourceHandle kindDs = sync.GetKind();
    return registry->GetOrCreateImportedExtGpuSemaphore(
        externalHandle,
        handleTypeDs ? handleTypeDs->GetTypedValue(0.0f) : TfToken(),
        kindDs ? kindDs->GetTypedValue(0.0f) : TfToken());
}

HdContainerDataSourceHandle
HdSt_GetPrimDataSource(
    HdSceneDelegate *sceneDelegate,
    SdfPath const &id)
{
    HdSceneIndexBaseRefPtr si =
        sceneDelegate->GetRenderIndex().GetTerminalSceneIndex();
    if (!si) {
        return nullptr;
    }
    return si->GetPrim(id).dataSource;
}

HdExtGpuBufferSchema
HdSt_GetExtGpuBufferSchema(
    HdContainerDataSourceHandle const &primDataSource,
    TfToken const &name)
{
    if (!primDataSource) {
        return HdExtGpuBufferSchema(nullptr);
    }
    const HdPrimvarSchema pv =
        HdPrimvarsSchema::GetFromParent(primDataSource).GetPrimvar(name);
    return HdExtGpuBufferSchema::GetFromParent(pv.GetContainer());
}

HdExtGpuBufferSchema
HdSt_GetExtGpuBufferSchema(
    HdSceneDelegate *sceneDelegate,
    SdfPath const &id,
    TfToken const &name)
{
    return HdSt_GetExtGpuBufferSchema(
        HdSt_GetPrimDataSource(sceneDelegate, id), name);
}

HdBufferSourceSharedPtr
HdSt_TryCreateExtGpuBufferSource(
    TfToken const &name,
    HdExtGpuBufferSchema const &schema,
    HdStResourceRegistry *registry)
{
    if (!schema || !schema.IsComplete()) {
        return nullptr;
    }

    std::optional<HdStExtGpuBufferDesc> hdDescOpt =
        HdStExtGpuBufferDesc::FromSchema(schema);
    if (!hdDescOpt) {
        return nullptr;
    }
    HdStExtGpuBufferDesc &hdDesc = *hdDescOpt;

    // Bounds check (if size known), before committing to any GPU work.
    // numElements and tupleType are already guaranteed non-degenerate by
    // IsComplete/FromSchema.
    if (hdDesc.rawHandleByteSize > 0) {
        size_t elemSize = HdDataSizeOfTupleType(hdDesc.tupleType);
        size_t stride = (hdDesc.byteStride > 0)
            ? hdDesc.byteStride : elemSize;
        size_t requiredEnd =
            hdDesc.byteOffset + hdDesc.numElements * stride;
        if (requiredEnd > hdDesc.rawHandleByteSize) {
            HD_PERF_COUNTER_INCR(
                HdStPerfTokens->extGpuBufferFallbackCount);
            return nullptr;
        }
    }

    Hgi *hgi = registry->GetHgi();
    if (!hgi) {
        HD_PERF_COUNTER_INCR(HdStPerfTokens->extGpuBufferFallbackCount);
        return nullptr;
    }

    // Route between the producer's two ways of naming the buffer. backendApi
    // carries the same token Hgi reports (HgiTokens->OpenGL / Vulkan / Metal),
    // so we compare directly against GetAPIName() -- no local token mapping
    // needed.
    const bool apiMatches = (hdDesc.backendApi == hgi->GetAPIName());
    const bool deviceMatches = _DeviceMatches(hdDesc.deviceUuid, hgi);
    const bool namespaceMatches =
        _LogicalDeviceMatches(hdDesc.logicalDeviceId, hgi);

    if (apiMatches && deviceMatches && namespaceMatches &&
            hdDesc.rawHandle != 0) {
        // 1. Adopt: the handle names an object this backend can bind as-is.
    } else if (deviceMatches && hdDesc.CanImport()) {
        // 2. Import: the producer's memory lives elsewhere (another logical
        //    device, or another API whose object handle means nothing here), so
        //    build our own buffer aliasing that memory. Note this route ignores
        //    backendApi -- memory is memory, whoever allocated it -- but not
        //    deviceMatches: an opaque handle is only importable on the physical
        //    device that exported it, and a doomed import would otherwise be
        //    retried (uncached, since failures are not cached) every Sync.
        //    Both consumption strategies are served from the imported buffer:
        //    a directBindable stream is aliased and bound zero-copy, while a
        //    non-directBindable stream is GPU-to-GPU blitted into Storm's
        //    aggregated VBO. The blit still avoids the CPU round trip this path
        //    exists to remove -- for a foreign-device producer, which has no
        //    cheap CPU copy of its own, that is cheaper than the CPU fallback,
        //    not more expensive.
        hdDesc.importedBuffer =
            registry->GetOrCreateImportedExtGpuBuffer(hdDesc);
        if (!hdDesc.importedBuffer) {
            HD_PERF_COUNTER_INCR(HdStPerfTokens->extGpuBufferFallbackCount);
            return nullptr;
        }
    } else {
        // 3. Neither route is open to us.
        HD_PERF_COUNTER_INCR(HdStPerfTokens->extGpuBufferFallbackCount);
        return nullptr;
    }

    // If the producer published a sync object (the "sync" child of the schema),
    // order its writes before this consumer's reads (RAW): make the next queue
    // submission wait on the producer's write semaphore. Enqueued here at Sync
    // time, the pending wait rides on the draw's submit. Only honored when the
    // semaphores belong to the active backend.
    if (HdExtGpuSyncSchema sync =
            HdExtGpuSyncSchema::GetFromParent(schema.GetContainer())) {
        const HdTokenDataSourceHandle apiDs = sync.GetBackendApi();
        if (apiDs && apiDs->GetTypedValue(0.0f) == hgi->GetAPIName()) {
            const uint64_t writeSem = _ResolveSemaphore(
                sync, registry, hgi,
                sync.GetWriteSemaphore(), sync.GetExternalWriteSemaphore());
            if (writeSem) {
                hgi->QueueWaitExternalSemaphore(writeSem);
            }
            // WAR: signal the read semaphore after this consumer's reads
            // complete, so the producer may wait before overwriting the buffer.
            const uint64_t readSem = _ResolveSemaphore(
                sync, registry, hgi,
                sync.GetReadSemaphore(), sync.GetExternalReadSemaphore());
            if (readSem) {
                hgi->QueueSignalExternalSemaphore(readSem);
            }
        }
    }

    // On the adopt route the source owns its non-owning HgiBuffer wrapper
    // internally; on the import route it just carries the registry's buffer.
    return std::make_shared<HdStExtGpuBufferSource>(name, hdDesc);
}

HdBufferArrayRangeSharedPtr
HdSt_TryCreateExtGpuBufferAliasBAR(
    HdBufferSourceSharedPtrVector const &sources,
    HdStResourceRegistry *registry,
    HdBufferArrayRangeSharedPtr const &existingBar)
{
    if (sources.empty()) {
        return nullptr;
    }

    // All sources must be direct-bindable external GPU sources.
    for (auto const &src : sources) {
        auto const *extSrc =
            dynamic_cast<HdStExtGpuBufferSource const *>(src.get());
        if (!extSrc) {
            return nullptr;
        }
        if (!extSrc->GetDescriptor().directBindable) {
            return nullptr;
        }
    }

    // Fast path: reuse existing alias BAR in-place.
    if (existingBar) {
        auto *aliasRange = dynamic_cast<HdStExtGpuBufferArrayRange *>(
            existingBar.get());
        if (aliasRange) {
            if (aliasRange->UpdateExternalResources(sources)) {
                return existingBar;
            }
            if (aliasRange->MergeExternalResources(sources)) {
                return existingBar;
            }
        }
    }

    // Slow path: create a new alias BAR.
    auto aliasBAR = std::make_shared<HdStExtGpuBufferArrayRange>(
        registry);

    for (auto const &src : sources) {
        auto const *extSrc =
            static_cast<HdStExtGpuBufferSource const *>(src.get());
        auto const &hdDesc = extSrc->GetDescriptor();

        aliasBAR->SetExternalResource(src->GetName(), hdDesc);
    }

    return aliasBAR;
}

PXR_NAMESPACE_CLOSE_SCOPE
