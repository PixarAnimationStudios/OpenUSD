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

#ifndef HD_USD_WRITER_MESH_H
#define HD_USD_WRITER_MESH_H

#include "pxr/usdImaging/plugin/hdUsdWriter/pointBased.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/api.h"

#include "pxr/pxr.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdUsdWriterMesh
///
/// This class is an example of a Hydra Rprim, or renderable object, and it
/// gets created on a call to HdRenderIndex::InsertRprim() with a type of
/// HdPrimTypeTokens->mesh.
///
/// The prim object's main function is to bridge the scene description and the
/// renderable representation. The Hydra image generation algorithm will call
/// HdRenderIndex::SyncAll() before any drawing; this, in turn, will call
/// Sync() for each mesh with new data.
///
/// Sync() is passed a set of dirtyBits, indicating which scene buffers are
/// dirty. It uses these to pull all of the new scene data and constructs
/// updated geometry objects.
///
/// An rprim's state is lazily populated in Sync(); matching this, Finalize()
/// can do the heavy work of releasing state (such as handles into the top-level
/// scene), so that object population and existence aren't tied to each other.
///
class HdUsdWriterMesh : public HdUsdWriterPointBased<HdMesh>
{
public:
    HF_MALLOC_TAG_NEW("new HdUsdWriterMesh");

    /// HdUsdWriterMesh constructor.
    ///   \param id The scene-graph path to this mesh.
    ///   \param writeExtent Whether or not to handle extent.
    HDUSDWRITER_API
    HdUsdWriterMesh(SdfPath const& id, bool writeExtent);

    /// HdUsdWriterMesh destructor.
    HDUSDWRITER_API
    ~HdUsdWriterMesh() override = default;

    /// Inform the scene graph which state needs to be downloaded in the
    /// first Sync() call: in this case, topology and points data to build
    /// the geometry object in the scene graph.
    ///   \return The initial dirty state this mesh wants to query.
    HDUSDWRITER_API
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    /// Propagates dirty bits.
    /// See HdRprim::_PropagateDirtyBits.
    HDUSDWRITER_API
    HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

    /// Pull invalidated scene data and prepare/update the renderable
    /// representation.
    ///
    /// This function is told which scene data to pull through the
    /// dirtyBits parameter. The first time it's called, dirtyBits comes
    /// from _GetInitialDirtyBits(), which provides initial dirty state,
    /// but after that it's driven by invalidation tracking in the scene
    /// delegate.
    ///
    /// The contract for this function is that the prim can only pull on scene
    /// delegate buffers that are marked dirty. Scene delegates can and do
    /// implement just-in-time data schemes that mean that pulling on clean
    /// data will be at best incorrect, and at worst a crash.
    ///
    /// This function is called in parallel from worker threads, so it needs
    /// to be threadsafe; calls into HdSceneDelegate are ok.
    ///
    /// Reprs are used by hydra for controlling per-item draw settings like
    /// flat/smooth shaded, wireframe, refined, etc.
    ///   \param sceneDelegate The data source for this geometry item.
    ///   \param renderParam State.
    ///   \param dirtyBits A specifier for which scene data has changed.
    ///   \param reprToken A specifier for which representation to draw with.
    ///
    HDUSDWRITER_API
    void Sync(HdSceneDelegate* sceneDelegate,
              HdRenderParam* renderParam,
              HdDirtyBits* dirtyBits,
              TfToken const& reprToken) override;

    /// Serialize the primitive to USD.
    ///
    ///   \param stage Reference to HdUsdWriter Stage Proxy.
    HDUSDWRITER_API
    virtual void SerializeToUsd(const UsdStagePtr &stage);

protected:
    // This class does not support copying.
    HdUsdWriterMesh(const HdUsdWriterMesh&) = delete;
    HdUsdWriterMesh& operator=(const HdUsdWriterMesh&) = delete;

    HdUsdWriterOptional<HdMeshTopology> _topology;
    HdUsdWriterOptional<GfRange3d> _extent;
    HdUsdWriterOptional<HdDisplayStyle> _displayStyle;
    HdUsdWriterOptional<bool> _doubleSided;

    struct HdUsdWriterSkelGeom
    {
        VtVec3fArray restPoints;
        GfMatrix4d geomBindingTransform;
        VtFloatArray jointWeights;
        VtIntArray jointIndices;
        int numInfluencesPerPoint;
        bool hasConstantInfluences;
        TfToken skinningMethod;
        VtFloatArray skinningBlendWeights;
        bool hasConstantSkinningBlendWeights;
        bool isSkelMesh;
    };

    struct HdUsdWriterSkelAnimXformValues
    {
        VtMatrix4fArray skinningXforms;
        GfMatrix4d primWorldToLocal;
        GfMatrix4d skelLocalToWorld;
    };

    HdUsdWriterOptional<HdUsdWriterSkelGeom> _skelGeom;
    HdUsdWriterOptional<HdUsdWriterSkelAnimXformValues> _skelAnimXformValues;

    bool _writeExtent = false;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif