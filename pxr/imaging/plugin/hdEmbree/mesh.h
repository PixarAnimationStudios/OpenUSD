//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_PLUGIN_HD_EMBREE_MESH_H
#define PXR_IMAGING_PLUGIN_HD_EMBREE_MESH_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/vertexAdjacency.h"
#include "pxr/base/gf/matrix4f.h"

#include "pxr/imaging/plugin/hdEmbree/meshSamplers.h"

#include <embree4/rtcore.h>
#include <embree4/rtcore_ray.h>

PXR_NAMESPACE_OPEN_SCOPE

struct HdEmbreePrototypeContext;
struct HdEmbreeInstanceContext;

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
/// updated embree geometry objects.  Rebuilding the top-level acceleration
/// datastructures is deferred to the start of HdEmbreeRender::Render().
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
    HdEmbreeMesh(SdfPath const& id);

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
    RTCGeometry _CreateEmbreeSubdivMesh(RTCScene scene, RTCDevice device);
    // Utility function to call rtcNewTriangleMesh and populate topology.
    RTCGeometry _CreateEmbreeTriangleMesh(RTCScene scene, RTCDevice device);

    // An embree intersection filter callback, for doing backface culling.
    static void _EmbreeCullFaces(const RTCFilterFunctionNArguments* args);

private:
    // Every HdEmbreeMesh is treated as instanced; if there's no instancer,
    // the prototype has a single identity instance. The prototype is stored
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

    // Embree recommends after creating one should hold onto the geometry
    //
    //      "However, it is generally recommended to store the geometry handle
    //       inside the application's geometry representation and look up the
    //       geometry handle from that representation directly.""
    //
    // found this to be necessary in the case where multiple threads were
    // commiting to the scene at the same time, and a geometry needed to be
    // referenced again while other threads were committing
    RTCGeometry _geometry;
    std::vector<RTCGeometry> _rtcInstanceGeometries;

    // This class does not support copying.
    HdEmbreeMesh(const HdEmbreeMesh&)             = delete;
    HdEmbreeMesh &operator =(const HdEmbreeMesh&) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_HD_EMBREE_MESH_H
