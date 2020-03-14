//
// Copyright 2017 Pixar
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
#ifndef PXR_IMAGING_PLUGIN_HD_EMBREE_MESH_H
#define PXR_IMAGING_PLUGIN_HD_EMBREE_MESH_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/vertexAdjacency.h"
#include "pxr/base/gf/matrix4f.h"

#include "pxr/imaging/plugin/hdEmbree/meshSamplers.h"

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdEmbreePrototypeContext;
class HdEmbreeInstanceContext;

/// \class HdEmbreeMesh
///
/// An HdEmbree representation of a subdivision surface or poly-mesh object.
/// This class is an example of a hydra Rprim, or renderable object, and it
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
/// updated embree geometry objects.  Rebuilding the acceleration datastructures
/// is deferred to HdEmbreeRenderDelegate::CommitResources(), which runs after
/// all prims have been updated. After running Sync() for each prim and
/// HdEmbreeRenderDelegate::CommitResources(), the scene should be ready for
/// rendering via ray queries.
///
/// An rprim's state is lazily populated in Sync(); matching this, Finalize()
/// does the heavy work of releasing state (such as handles into the top-level
/// embree scene), so that object population and existence aren't tied to
/// each other.
///
class HdEmbreeMesh final : public HdMesh {
public:
    HF_MALLOC_TAG_NEW("new HdEmbreeMesh");

    /// HdEmbreeMesh constructor.
    ///   \param id The scene-graph path to this mesh.
    ///   \param instancerId If specified, the HdEmbreeInstancer at this id uses
    ///                      this mesh as a prototype.
    HdEmbreeMesh(SdfPath const& id,
                 SdfPath const& instancerId = SdfPath());

    /// HdEmbreeMesh destructor.
    /// (Note: Embree resources are released in Finalize()).
    virtual ~HdEmbreeMesh() = default;

    /// Inform the scene graph which state needs to be downloaded in the
    /// first Sync() call: in this case, topology and points data to build
    /// the geometry object in the embree scene graph.
    ///   \return The initial dirty state this mesh wants to query.
    virtual HdDirtyBits GetInitialDirtyBitsMask() const override;

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
    ///   \param renderParam An HdEmbreeRenderParam object containing top-level
    ///                      embree state.
    ///   \param dirtyBits A specifier for which scene data has changed.
    ///   \param reprToken A specifier for which representation to draw with.
    ///
    virtual void Sync(HdSceneDelegate* sceneDelegate,
                      HdRenderParam*   renderParam,
                      HdDirtyBits*     dirtyBits,
                      TfToken const    &reprToken) override;

    /// Release any resources this class is holding onto: in this case,
    /// destroy the geometry object in the embree scene graph.
    ///   \param renderParam An HdEmbreeRenderParam object containing top-level
    ///                      embree state.
    virtual void Finalize(HdRenderParam *renderParam) override;

protected:
    // Initialize the given representation of this Rprim.
    // This is called prior to syncing the prim, the first time the repr
    // is used.
    //
    // reprToken is the name of the repr to initalize.
    //
    // dirtyBits is an in/out value.  It is initialized to the dirty bits
    // from the change tracker.  InitRepr can then set additional dirty bits
    // if additional data is required from the scene delegate when this
    // repr is synced.  InitRepr occurs before dirty bit propagation.
    //
    // See HdRprim::InitRepr()
    virtual void _InitRepr(TfToken const &reprToken,
                           HdDirtyBits *dirtyBits) override;

    // This callback from Rprim gives the prim an opportunity to set
    // additional dirty bits based on those already set.  This is done
    // before the dirty bits are passed to the scene delegate, so can be
    // used to communicate that extra information is needed by the prim to
    // process the changes.
    //
    // The return value is the new set of dirty bits, which replaces the bits
    // passed in.
    //
    // See HdRprim::PropagateRprimDirtyBits()
    virtual HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

private:
    // Helper functions for getting the prototype and instance contexts.
    // These don't do null checks, so the user is responsible for calling them
    // carefully.
    HdEmbreePrototypeContext* _GetPrototypeContext();
    HdEmbreeInstanceContext* _GetInstanceContext(RTCScene scene, size_t i);

    // Populate the embree geometry object based on scene data.
    void _PopulateRtMesh(HdSceneDelegate *sceneDelegate,
                         RTCScene scene,
                         RTCDevice device,
                         HdDirtyBits *dirtyBits,
                         HdMeshReprDesc const &desc);

    // Populate _primvarSourceMap (our local cache of primvar data) based on
    // authored scene data.
    // Primvars will be turned into samplers in _PopulateRtMesh,
    // through the help of the _CreatePrimvarSampler() method.
    void _UpdatePrimvarSources(HdSceneDelegate* sceneDelegate,
                               HdDirtyBits dirtyBits);

    // Populate _primvarSourceMap with primvars that are computed.
    // Return the names of the primvars that were successfully updated.
    TfTokenVector _UpdateComputedPrimvarSources(HdSceneDelegate* sceneDelegate,
                                                HdDirtyBits dirtyBits);

    // Populate a single primvar, with given name and data, in the prototype
    // context. Overwrites the current mapping for the name, if necessary.
    // This function's main purpose is to resolve the (interpolation, refined)
    // tuple into the concrete primvar sampler type.
    void _CreatePrimvarSampler(TfToken const& name, VtValue const& data,
                               HdInterpolation interpolation,
                               bool refined);

    // Utility function to call rtcNewSubdivisionMesh and populate topology.
    void _CreateEmbreeSubdivMesh(RTCScene scene);
    // Utility function to call rtcNewTriangleMesh and populate topology.
    void _CreateEmbreeTriangleMesh(RTCScene scene);

    // An embree intersection filter callback, for doing backface culling.
    static void _EmbreeCullFaces(void *userData, RTCRay &ray);

private:
    // Every HdEmbreeMesh is treated as instanced; if there's no instancer,
    // the prototype has a single identity istance. The prototype is stored
    // as _rtcMeshId, in _rtcMeshScene.
    unsigned _rtcMeshId;
    RTCScene _rtcMeshScene;
    // Each instance of the mesh in the top-level scene is stored in
    // _rtcInstanceIds.
    std::vector<unsigned> _rtcInstanceIds;

    // Cached scene data. VtArrays are reference counted, so as long as we
    // only call const accessors keeping them around doesn't incur a buffer
    // copy.
    HdMeshTopology _topology;
    GfMatrix4f _transform;
    VtVec3fArray _points;

    // Derived scene data:
    // - _triangulatedIndices holds a triangulation of the source topology,
    //   which can have faces of arbitrary arity.
    // - _trianglePrimitiveParams holds a mapping from triangle index (in
    //   the triangulated topology) to authored face index.
    // - _computedNormals holds per-vertex normals computed as an average of
    //   adjacent face normals.
    VtVec3iArray _triangulatedIndices;
    VtIntArray _trianglePrimitiveParams;
    VtVec3fArray _computedNormals;

    // Derived scene data. Hd_VertexAdjacency is an acceleration datastructure
    // for computing per-vertex smooth normals. _adjacencyValid indicates
    // whether the datastructure has been rebuilt with the latest topology,
    // and _normalsValid indicates whether _computedNormals has been
    // recomputed with the latest points data.
    Hd_VertexAdjacency _adjacency;
    bool _adjacencyValid;
    bool _normalsValid;

    // Draw styles.
    bool _refined;
    bool _smoothNormals;
    bool _doubleSided;
    HdCullStyle _cullStyle;

    // A local cache of primvar scene data. "data" is a copy-on-write handle to
    // the actual primvar buffer, and "interpolation" is the interpolation mode
    // to be used. This cache is used in _PopulateRtMesh to populate the
    // primvar sampler map in the prototype context, which is used for shading.
    struct PrimvarSource {
        VtValue data;
        HdInterpolation interpolation;
    };
    TfHashMap<TfToken, PrimvarSource, TfToken::HashFunctor> _primvarSourceMap;

    // An object used to manage allocation of embree user vertex buffers to
    // primvars.
    HdEmbreeRTCBufferAllocator _embreeBufferAllocator;

    // This class does not support copying.
    HdEmbreeMesh(const HdEmbreeMesh&)             = delete;
    HdEmbreeMesh &operator =(const HdEmbreeMesh&) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_HD_EMBREE_MESH_H
