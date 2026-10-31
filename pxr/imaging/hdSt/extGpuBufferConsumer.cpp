//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/extGpuBufferConsumer.h"

#include "pxr/imaging/hdSt/debugCodes.h"
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

#include <algorithm>
#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

bool
HdSt_HasExtGpuBufferArenas(HdStResourceRegistry *registry)
{
    Hgi *hgi = registry ? registry->GetHgi() : nullptr;
    return hgi && hgi->HasExternalBufferArenas();
}

HdContainerDataSourceHandle
HdSt_GetPrimDataSource(
    HdSceneDelegate *sceneDelegate,
    SdfPath const &id,
    HdStResourceRegistry *registry)
{
    // The whole feature switched off in one atomic load, for every
    // application that never shares a buffer. An arena has to exist before a
    // producer can publish into one, so no arena means no prim can be
    // carrying one -- and the GetPrim below would be pure cost, once per
    // prim, per Sync, in perpetuity.
    if (!HdSt_HasExtGpuBufferArenas(registry)) {
        return nullptr;
    }

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

HdBufferSourceSharedPtr
HdSt_TryCreateExtGpuBufferSource(
    TfToken const &name,
    HdExtGpuBufferSchema const &schema,
    HdStResourceRegistry *registry)
{
    // No external buffer published for this primvar: the ordinary CPU path,
    // not a fallback.
    if (!schema || !registry) {
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

    // No arena registration here. Storm no longer brackets its own access:
    // the wait and the signal are encoded by Hgi from StartFrame/EndFrame,
    // over every arena it owns, so there is nothing for this routing choke
    // point to record.
    return std::make_shared<HdStExtGpuBufferSource>(name, *desc);
}

HdBufferArrayRangeSharedPtr
HdSt_TryCreateExtGpuBufferAliasBAR(
    HdBufferSourceSharedPtrVector const &sources,
    HdStResourceRegistry *registry,
    HdBufferArrayRangeSharedPtr const &existingBar,
    HdBufferSpecVector const &removedSpecs,
    SdfPath const &primId)
{
    auto *aliasRange = existingBar
        ? dynamic_cast<HdStExtGpuBufferArrayRange *>(existingBar.get())
        : nullptr;

    if (sources.empty()) {
        if (!aliasRange) {
            return nullptr;
        }
        // Nothing changed value, but the caller still reaches the registry
        // when the primvar descriptors may have changed. The registry would
        // migrate a direct range into a copied one there, snapshotting the
        // external buffers once, so settle it here instead.
        bool removesBoundResource = false;
        for (HdBufferSpec const &spec : removedSpecs) {
            if (aliasRange->GetResource(spec.name)) {
                removesBoundResource = true;
                break;
            }
        }
        if (!removesBoundResource) {
            TF_DEBUG(HDST_EXT_GPU_BUFFER).Msg(
                "[ExtGpuBuffer] %s: kept direct range %p (no sources, "
                "nothing bound removed)\n",
                primId.GetText(), static_cast<void *>(existingBar.get()));
            return existingBar;
        }
        TfTokenVector removedNames;
        removedNames.reserve(removedSpecs.size());
        for (HdBufferSpec const &spec : removedSpecs) {
            removedNames.push_back(spec.name);
        }
        auto trimmedBAR =
            std::make_shared<HdStExtGpuBufferArrayRange>(registry);
        trimmedBAR->AdoptResources(*aliasRange, removedNames);
        if (trimmedBAR->GetResources().empty()) {
            return nullptr;
        }
        TF_DEBUG(HDST_EXT_GPU_BUFFER).Msg(
            "[ExtGpuBuffer] %s: new direct range %p (0 sources, %zu carried "
            "over), dropping removed primvars from direct range %p\n",
            primId.GetText(), static_cast<void *>(trimmedBAR.get()),
            trimmedBAR->GetResources().size(),
            static_cast<void *>(existingBar.get()));
        return trimmedBAR;
    }

    // Every source has to be one we may bind directly. A single ordinary
    // source, one the producer only permitted us to copy, or one whose layout
    // Storm cannot bind in place sends the whole set through the usual
    // aggregation path -- mixing the two in one range is what the alias
    // range's coding errors are there to catch.
    for (HdBufferSourceSharedPtr const &source : sources) {
        HdStExtGpuBufferSource const *extSource =
            HdSt_GetExtGpuBufferSource(source);
        if (!extSource) {
            // Only worth reporting when an external source is being dragged
            // onto the copy path by this one.
            if (TfDebug::IsEnabled(HDST_EXT_GPU_BUFFER)) {
                for (HdBufferSourceSharedPtr const &other : sources) {
                    if (HdSt_GetExtGpuBufferSource(other)) {
                        TfDebug::Helper().Msg(
                            "[ExtGpuBuffer] %s: direct bind refused, '%s' "
                            "is a CPU source (%zu elements), so external "
                            "'%s' is copied with it (existing range %p)\n",
                            primId.GetText(), source->GetName().GetText(),
                            source->GetNumElements(),
                            other->GetName().GetText(),
                            static_cast<void *>(existingBar.get()));
                        break;
                    }
                }
            }
            return nullptr;
        }
        HdStExtGpuBufferDesc const &desc = extSource->GetDescriptor();
        if (!desc.IsDirectBindable()) {
            TF_DEBUG(HDST_EXT_GPU_BUFFER).Msg(
                "[ExtGpuBuffer] %s: direct bind refused, '%s' cannot be bound "
                "in place (allowDirectBind=%d offset=%zu stride=%zu "
                "elementSize=%zu); copying\n",
                primId.GetText(), source->GetName().GetText(),
                int(desc.allowDirectBind), desc.byteOffset, desc.byteStride,
                HdDataSizeOfTupleType(desc.tupleType));
            return nullptr;
        }
    }

    // Sources carry only what changed this sync. Everything else the existing
    // range binds stays bound unless the prim no longer has it.
    TfTokenVector replacedNames;
    replacedNames.reserve(sources.size() + removedSpecs.size());
    for (HdBufferSourceSharedPtr const &source : sources) {
        replacedNames.push_back(source->GetName());
    }
    for (HdBufferSpec const &spec : removedSpecs) {
        replacedNames.push_back(spec.name);
    }
    auto isReplaced = [&replacedNames](TfToken const &name) {
        return std::find(replacedNames.begin(), replacedNames.end(), name) !=
            replacedNames.end();
    };

    if (existingBar && !aliasRange && existingBar->IsValid()) {
        // An ordinary range still holding data this sync does not replace --
        // a static CPU primvar next to animated external points, say -- has
        // to stay on the aggregation path, or that data is lost.
        HdBufferSpecVector existingSpecs;
        existingBar->GetBufferSpecs(&existingSpecs);
        for (HdBufferSpec const &spec : existingSpecs) {
            if (!isReplaced(spec.name)) {
                TF_DEBUG(HDST_EXT_GPU_BUFFER).Msg(
                    "[ExtGpuBuffer] %s: direct bind refused, existing copied "
                    "range %p still holds '%s', which this sync does not "
                    "replace; copying\n",
                    primId.GetText(), static_cast<void *>(existingBar.get()),
                    spec.name.GetText());
                return nullptr;
            }
        }
    }

    if (aliasRange) {
        bool removesBoundResource = false;
        for (HdBufferSpec const &spec : removedSpecs) {
            if (aliasRange->GetResource(spec.name)) {
                removesBoundResource = true;
                break;
            }
        }
        // Rebind in place where we can: replacing the range marks every draw
        // batch that names it dirty. Dropping a resource changes what the
        // shader binds, so that has to go through a new range.
        if (!removesBoundResource &&
                aliasRange->UpdateExternalResources(sources)) {
            TF_DEBUG(HDST_EXT_GPU_BUFFER).Msg(
                "[ExtGpuBuffer] %s: rebound direct range %p in place "
                "(%zu sources)\n",
                primId.GetText(), static_cast<void *>(existingBar.get()),
                sources.size());
            return existingBar;
        }
    }

    auto aliasBAR = std::make_shared<HdStExtGpuBufferArrayRange>(registry);
    if (aliasRange) {
        aliasBAR->AdoptResources(*aliasRange, replacedNames);
    }
    const size_t numAdopted = aliasBAR->GetResources().size();
    for (HdBufferSourceSharedPtr const &source : sources) {
        HdStExtGpuBufferSource const *extSource =
            HdSt_GetExtGpuBufferSource(source);
        aliasBAR->SetExternalResource(
            source->GetName(), extSource->GetDescriptor());
    }
    TF_DEBUG(HDST_EXT_GPU_BUFFER).Msg(
        "[ExtGpuBuffer] %s: new direct range %p (%zu sources, %zu carried "
        "over), replacing %s range %p\n",
        primId.GetText(), static_cast<void *>(aliasBAR.get()),
        sources.size(), numAdopted,
        !existingBar ? "no" : (aliasRange ? "direct" : "copied"),
        static_cast<void *>(existingBar.get()));
    return aliasBAR;
}

void
HdSt_ReportExtGpuBufferCopiedForComputations(
    HdBufferSourceSharedPtrVector const &sources,
    SdfPath const &primId)
{
    if (!TfDebug::IsEnabled(HDST_EXT_GPU_BUFFER)) {
        return;
    }
    for (HdBufferSourceSharedPtr const &source : sources) {
        if (HdSt_GetExtGpuBufferSource(source)) {
            TfDebug::Helper().Msg(
                "[ExtGpuBuffer] %s: direct bind skipped, GPU computations "
                "are queued on this range; external '%s' is copied\n",
                primId.GetText(), source->GetName().GetText());
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
