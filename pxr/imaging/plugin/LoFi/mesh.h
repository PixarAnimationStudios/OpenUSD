//
// Copyright 2020 benmalartre
//
// unlicensed
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_MESH_H
#define PXR_IMAGING_PLUGIN_LOFI_MESH_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/extComputationUtils.h"
#include "pxr/imaging/hd/vertexAdjacency.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"


PXR_NAMESPACE_OPEN_SCOPE

class LoFiScene;
/// \class LoFiMesh
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
class LoFiMesh final : public HdMesh 
{
public:
    HF_MALLOC_TAG_NEW("new LoFiMesh");

    /// LoFiMesh constructor
    LoFiMesh(SdfPath const& id, SdfPath const& instancerId = SdfPath());

    /// LoFiMesh destructor.
    ~LoFiMesh() override = default;

    HdDirtyBits GetInitialDirtyBitsMask() const override;

    void Sync(HdSceneDelegate* sceneDelegate,
              HdRenderParam*   renderParam,
              HdDirtyBits*     dirtyBits,
              TfToken const    &reprToken) override;

protected:
    void _InitRepr(
        TfToken const &reprToken,
        HdDirtyBits *dirtyBits) override;

    HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

    void _PopulateLoFiMesh(HdSceneDelegate* sceneDelegate,
                          LoFiScene*         scene,
                          HdDirtyBits*     dirtyBits,
                          HdMeshReprDesc const &desc);

    TfTokenVector _UpdateComputedPrimvarSources(HdSceneDelegate* sceneDelegate, 
                                                  HdDirtyBits dirtyBits);

    // Get num points
    const inline int GetNumPoints() const{return _points.size();};

    // Get num triangles
    const inline int GetNumTriangles() const{return _triangles.size();};

    // Get positions ptr
    const inline GfVec3f* GetPositionsPtr() const{return _points.cdata();};

    // Get normals ptr
    const inline GfVec3i* GetNormalsPtr() const{return _normals.cdata();};

     // Get indices ptr
    const inline GfVec3i* GetIndicesPtr() const{return _triangles.cdata();};

    // Get colors ptr
    const inline GfVec3i* GetIndicesPtr() const{return _colors.cdata();};

    // This class does not support copying.
    LoFiMesh(const LoFiMesh&) = delete;
    LoFiMesh &operator =(const LoFiMesh&) = delete;

private:

    // Cached scene data. VtArrays are reference counted, so as long as we
    // only call const accessors keeping them around doesn't incur a buffer
    // copy.
    HdMeshTopology                  _topology;
    GfMatrix4f                      _transform;
    VtArray<GfVec3f>                _points;
    VtArray<GfVec3f>                _normals;
    VtArray<GfVec3f>                _colors;
    VtVec3iArray                    _triangles;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_MESH_H
