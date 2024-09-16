//
// Copyright (c) 2022-2024, NVIDIA CORPORATION.
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

#ifndef HD_USD_WRITER_INSTANCER_H
#define HD_USD_WRITER_INSTANCER_H

#include "pxr/usdImaging/plugin/hdUsdWriter/utils.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/api.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/token.h"
#include "pxr/imaging/hd/instancer.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/hf/perfLog.h"
#include "pxr/pxr.h"
#include "pxr/usd/usd/prim.h"
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_unordered_set.h>

#include <mutex>
#include <unordered_map>
#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

struct HdUsdWriter_TbbTfTokenCompare
{
    static size_t hash(const TfToken& x)
    {
        return TfToken::HashFunctor()(x);
    }
    static bool equal(const TfToken& x, const TfToken& y)
    {
        return x == y;
    }
};

class HdUsdWriterInstancer : public HdInstancer
{
public:
    HF_MALLOC_TAG_NEW("new HdUsdWriterInstancer");

    /// Constructor.
    ///   \param delegate The scene delegate backing this instancer's data.
    ///   \param id The unique id of this instancer.
    HDUSDWRITER_API
    HdUsdWriterInstancer(HdSceneDelegate* delegate, SdfPath const& id);

    /// Destructor.
    HDUSDWRITER_API
    ~HdUsdWriterInstancer() override = default;

    /// Adds the prim to the instancer's collection.
    HDUSDWRITER_API
    virtual void AddInstancedPrim(const SdfPath& path);

    /// Removes the prim from the instancer's collection.
    HDUSDWRITER_API
    virtual void RemoveInstancedPrim(const SdfPath& path);

    /// Pull invalidated scene data and prepare/update the renderable representation.
    ///   \param sceneDelegate The data source for this geometry item.
    ///   \param dirtyBits A specifier for which scene data has changed.
    HDUSDWRITER_API
    void Sync(HdSceneDelegate* sceneDelegate, HdRenderParam* renderParam, HdDirtyBits* dirtyBits) override;

    /// Serialize the primitive to USD.
    ///
    ///   \param stage Reference to HdUsdWriter Stage Proxy.
    HDUSDWRITER_API
    virtual void SerializeToUsd(const UsdStagePtr &stage);

    /// Returns a valid USD prototype path grouped by original protoIndex from a Hydra Rprim ID
    /// e.g. `/instancer1.proto1_cube1_id2` -> `/instancer1/proto1/proto1_cube1_id2`
    static void GetPrototypePath(const SdfPath& rprimId,
                                 const SdfPath& instancerPath,
                                 SdfPath& protoPathOut);

private:
    HdUsdWriterOptional<GfMatrix4d> _transform;
    HdUsdWriterOptional<bool> _visible;

    using PrimvarMap =
        tbb::concurrent_hash_map<TfToken, HdUsdWriterPrimvar, HdUsdWriter_TbbTfTokenCompare>;
    using RprimSet = tbb::concurrent_unordered_set<SdfPath, SdfPath::Hash>;
    // Map of the latest primvar data for this instancer, keyed by
    // primvar name. Primvar values are VtValue, an any-type; they are
    // interpreted at consumption time (here, in ComputeInstanceTransforms).
    PrimvarMap _primvars;
    RprimSet _rPrims;

    template <typename T, typename F>
    void _GetPrimvar(const TfToken& name, F&& f) const
    {
        PrimvarMap::accessor pvAccessor;
        if (_primvars.find(pvAccessor, name) && pvAccessor->second.value.IsHolding<T>())
        {
            f(pvAccessor->second.value.UncheckedGet<T>());
        }
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif