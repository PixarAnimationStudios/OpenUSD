//
// Copyright 2023 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_PRIM_UTIL_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_PRIM_UTIL_H

#include "pxr/pxr.h"
#include "hdPrman/api.h"
#include "hdPrman/sceneIndexObserverApi.h"

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#include "pxr/imaging/hdsi/primManagingSceneIndexObserver.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

/// Support for std::optional<T>
///
/// Turns std::optional<T> into const T * which is how riley's
/// API represents optional values.
///
/// In particular, we can use std::optional<T> to provide
/// a value only if it has changed and convert that optional
/// with HdPrman_GetPtr to an argument of Riley::ModifyFoo
/// where a nullptr indicates that we do not change that
/// particular field of a riley prim.
///
template<typename T>
const T * HdPrman_GetPtr(const std::optional<T> &v)
{
    if (!v) {
        return nullptr;
    }
    return &(*v);
}

/// Some riley API expects non-RAII objects, e.g.,
/// riley::RenderOutputList has a raw pointer to an array of
/// riley::RenderOutputId's and its d'tor does not free that array.
///
/// In hdPrman, we wrap such an object in an RAII object T with
/// T::rileyObject being the non-RAII object pointing to, e.g., the
/// data of a std::vector<riley::RenderOutputId> in T.

///
/// Extract T::rileyObject as pointer from std::optional<T>.
///
/// Use as a helper to extract the non-RAII object from a std::optional<T>
/// where T is the RAII wrapper.
///
/// Similar to HdPrman_GetPtr, can be used as argument to Riley::ModifyFoo.
///
template<typename T>
auto HdPrman_GetPtrRileyObject(
        const std::optional<T> &v) -> decltype(&(v->rileyObject))
{
    if (!v) {
        return nullptr;
    }
    return &(v->rileyObject);
}

/// A (RAII) helper struct to retrieve riley prims managed by a prim managing
/// scene index observer and identified by paths from a data source.
///
/// The struct contains both the wrapping objects of type PrimType 
/// (subclassing from HdPrman_RileyPrimBase) as well as the riley prim
/// ids packaged in (the non-RAII) riley::RenderOutputList or similar.
///
template<typename PrimType>
struct HdPrman_RileyPrimArray
{
    /// List of riley ids, e.g., riley::RenderOutputList.
    ///
    /// This will be passed to calls such as Riley::RenderTarget.
    /// It is the non-RAII object.
    ///
    /// Example for a RileyObject:
    /// struct RenderOutputList
    /// {
    ///      uint32_t count;
    ///      RenderOutputId const *ids; // raw-pointer making it non-RAII
    /// };
    using RileyObject = typename PrimType::RileyIdList;

    /// Handle to subclass of HdPrman_RileyPrimBase
    using PrimHandle = std::shared_ptr<PrimType>;
    /// Corresponding riley id, e.g., riley::RenderOutputId
    using RileyId = typename PrimType::RileyId;

    /// C'tor takes observerer managing the prims and data source
    /// identifying prims by paths.
    ///
    HdPrman_RileyPrimArray(
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
    RileyObject rileyObject;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // #ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#endif
