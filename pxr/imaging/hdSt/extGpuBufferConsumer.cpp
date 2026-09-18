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

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/imaging/hgi/hgi.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

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
    if (!registry) {
        return nullptr;
    }
    Hgi *hgi = registry->GetHgi();
    if (!hgi) {
        HD_PERF_COUNTER_INCR(HdStPerfTokens->extGpuBufferFallbackCount);
        return nullptr;
    }

    // All the routing this used to do -- compare the producer's API against
    // ours, compare physical and logical devices, choose between adopting a
    // native handle and importing foreign memory, cache the import -- was
    // decided once when the application asked Hgi for an arena. What is left
    // here is to take a strong reference to the buffer and check it is ours.
    const std::optional<HdStExtGpuBufferDesc> desc =
        HdStExtGpuBufferDesc::FromSchema(schema, hgi);
    if (!desc) {
        HD_PERF_COUNTER_INCR(HdStPerfTokens->extGpuBufferFallbackCount);
        return nullptr;
    }

    // Storm is about to read this arena's buffers, so its synchronization has
    // to bracket the commit that does so. Recorded here, at the one routing
    // choke point; issued in HdStResourceRegistry::Commit, which is where the
    // reads actually happen -- Sync is much too early, since nothing has been
    // recorded yet for a wait to sit in front of.
    registry->RegisterExtGpuBufferArena(desc->GetArena());

    return std::make_shared<HdStExtGpuBufferSource>(name, *desc);
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

    // Every source has to be one we may bind directly. A single ordinary
    // source, or one the producer only permitted us to copy, sends the whole
    // set through the usual aggregation path -- mixing the two in one range is
    // what the alias range's coding errors are there to catch.
    for (HdBufferSourceSharedPtr const &source : sources) {
        HdStExtGpuBufferSource const *extSource =
            HdSt_GetExtGpuBufferSource(source);
        if (!extSource || !extSource->GetDescriptor().allowDirectBind) {
            return nullptr;
        }
    }

    // Rebind an existing range in place where we can: replacing it would mark
    // every draw batch that names it dirty.
    if (existingBar) {
        auto *aliasRange = dynamic_cast<HdStExtGpuBufferArrayRange *>(
            existingBar.get());
        if (aliasRange && aliasRange->UpdateExternalResources(sources)) {
            return existingBar;
        }
    }

    auto aliasBAR = std::make_shared<HdStExtGpuBufferArrayRange>(registry);
    for (HdBufferSourceSharedPtr const &source : sources) {
        HdStExtGpuBufferSource const *extSource =
            HdSt_GetExtGpuBufferSource(source);
        aliasBAR->SetExternalResource(
            source->GetName(), extSource->GetDescriptor());
    }
    return aliasBAR;
}

PXR_NAMESPACE_CLOSE_SCOPE
