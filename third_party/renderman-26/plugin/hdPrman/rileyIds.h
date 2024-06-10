//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_PRIM_UTIL_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_PRIM_UTIL_H

#include "pxr/pxr.h"
#include "hdPrman/sceneIndexObserverApi.h"

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#include "pxr/imaging/hdsi/primManagingSceneIndexObserver.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \name Path conversions to riley ids
///
/// Conversion of a path or an array of paths to scene index prims
/// to a riley type such as riley::RenderTargetId or RenderOutputList.
/// @{

/// A (RAII) helper struct to retrieve riley prims managed by a prim managing
/// scene index observer and identified by a path from a data source.
///
/// The struct contains both the wrapping object of type PrimType 
/// (subclassing from HdPrman_RileyPrimBase) as well as the (non-RAII) riley
/// prim.
///
template<typename PrimType>
struct HdPrman_RileyId
{
    /// A riley id, e.g., riley::RenderTargetId. It is essentially just an
    /// integer.
    ///
    /// This will be passed to calls such as Riley::RenderTarget.
    /// It is the non-RAII object.
    ///
    using RileyType = typename PrimType::RileyId;

    /// Handle to subclass of HdPrman_RileyPrimBase
    using PrimHandle = std::shared_ptr<PrimType>;

    /// C'tor takes observerer managing the prims and data source
    /// identifying prims by paths.
    ///
    HdPrman_RileyId(
        const HdsiPrimManagingSceneIndexObserver * const observer,
        HdPathDataSourceHandle const &ds)
    {
        if (!ds) {
            return;
        }

        const SdfPath path = ds->GetTypedValue(0.0f);
        if (path.IsEmpty()) {
            return;
        }

        prim = observer->GetTypedPrim<PrimType>(path);
        if (prim) {
            rileyObject = prim->GetRileyId();
        }
    }

    /// The prims wrapping the riley prims, subclass of HdPrman_RileyPrimBase.
    PrimHandle prim;
    /// The rileyId.
    RileyType rileyObject;
};

/// A (RAII) helper struct to retrieve riley prims managed by a prim managing
/// scene index observer and identified by paths from a data source.
///
/// The struct contains both the wrapping objects of type PrimType 
/// (subclassing from HdPrman_RileyPrimBase) as well as the riley prim
/// ids packaged in (the non-RAII) riley::RenderOutputList or similar.
///
template<typename PrimType>
struct HdPrman_RileyIdList
{
    /// List of riley ids, e.g., riley::RenderOutputList.
    ///
    /// This will be passed to calls such as Riley::CreateRenderTarget.
    /// It is the non-RAII object.
    ///
    /// Example for a RileyType:
    /// struct RenderOutputList
    /// {
    ///      uint32_t count;
    ///      RenderOutputId const *ids; // raw-pointer making it non-RAII
    /// };
    using RileyType = typename PrimType::RileyIdList;

    /// Handle to subclass of HdPrman_RileyPrimBase
    using PrimHandle = std::shared_ptr<PrimType>;
    /// Corresponding riley id, e.g., riley::RenderOutputId
    using RileyId = typename PrimType::RileyId;

    /// C'tor takes observerer managing the prims and data source
    /// identifying prims by paths.
    ///
    HdPrman_RileyIdList(
        const HdsiPrimManagingSceneIndexObserver * const observer,
        HdPathArrayDataSourceHandle const &ds)
      : rileyObject{0, nullptr}
    {
        if (!ds) {
            return;
        }

        const VtArray<SdfPath> paths = ds->GetTypedValue(0.0f);
        prims.reserve(paths.size());
        rileyIds.reserve(paths.size());
        for (const SdfPath &path : paths) {
            const PrimHandle prim = observer->GetTypedPrim<PrimType>(path);
            prims.push_back(prim);
            if (prim) {
                rileyIds.push_back(prim->GetRileyId());
            }
        }
        rileyObject.count = static_cast<uint32_t>(rileyIds.size());
        rileyObject.ids = rileyIds.data();
    }

    /// The prims wrapping the riley prims, subclass of HdPrman_RileyPrimBase.
    std::vector<PrimHandle> prims;
    /// Corresponding riley ids, e.g., riley::RenderOutputId.
    /// Does not include (invalid) riley ids for invalid PrimHandle's.
    std::vector<RileyId> rileyIds;
    /// Same information as rileyIds but as, e.g., riley::RenderOutputList
    /// (with pointers pointing into rileyIds).
    RileyType rileyObject;
};

/// @}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // #ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#endif
