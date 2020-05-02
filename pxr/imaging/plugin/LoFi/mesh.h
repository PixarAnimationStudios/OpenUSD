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
#include "pxr/imaging/plugin/LoFi/halfedge.h"
#include "pxr/imaging/plugin/LoFi/binding.h"
#include "pxr/imaging/plugin/LoFi/vertexBuffer.h"
#include "pxr/imaging/plugin/LoFi/vertexArray.h"
#include "pxr/imaging/plugin/LoFi/resourceRegistry.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include <memory>


PXR_NAMESPACE_OPEN_SCOPE


static float CONTOUR_TEST_POSITIONS[9] = {
  -4,0,0,
  0,4,0,
  4,0,0
};

class LoFiOctree;
class LoFiDualMesh;
/// \class LoFiMesh
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

    const LoFiAdjacency* GetAdjacency() const {return &_adjacency;};
    const GfVec3f* GetTriangleNormalsPtr() const { return _triangleNormals.cdata();};
    const GfVec3f* GetPositionsPtr() const {return _positions.cdata();};
    const GfVec3f* GetNormalsPtr() const {return _normals.cdata();};
    const VtArray<GfVec3f>& GetTriangleNormals() const { return _triangleNormals;};
    const VtArray<GfVec3f>& GetPositions() const {return _positions;};
    const VtArray<GfVec3f>& GetNormals() const {return _normals;};
    size_t GetNumPoints(){return _positions.size();};
    size_t GetNumTriangles(){return _samples.size()/3;};
    
protected:
    void _InitRepr(
        TfToken const &reprToken,
        HdDirtyBits *dirtyBits) override;

    void _PopulateMesh( HdSceneDelegate*              sceneDelegate,
                        HdDirtyBits*                  dirtyBits,
                        TfToken const                 &reprToken,
                        LoFiResourceRegistrySharedPtr registry);

    LoFiVertexBufferState _PopulatePrimvar( HdSceneDelegate* sceneDelegate,
                                            HdInterpolation interpolation,
                                            LoFiAttributeChannel channel,
                                            const VtValue& value,
                                            LoFiResourceRegistrySharedPtr registry);

    void _PopulateBinder(LoFiResourceRegistrySharedPtr registry);


    HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

    // This class does not support copying.
    LoFiMesh(const LoFiMesh&) = delete;
    LoFiMesh &operator =(const LoFiMesh&) = delete;

private:
    enum DirtyBits : HdDirtyBits {
        DirtySmoothNormals  = HdChangeTracker::CustomBitsBegin,
        DirtyFlatNormals    = (DirtySmoothNormals << 1),
        DirtyIndices        = (DirtyFlatNormals   << 1),
        DirtyHullIndices    = (DirtyIndices       << 1),
        DirtyPointsIndices  = (DirtyHullIndices   << 1)
    };
    
    VtArray<GfVec3f>                _positions;
    VtArray<GfVec3f>                _triangleNormals;
    VtArray<GfVec3f>                _normals;
    VtArray<GfVec3f>                _colors;
    VtArray<GfVec2f>                _uvs;
    VtArray<GfVec3i>                _samples;
    LoFiAdjacency                   _adjacency;
    LoFiVertexArraySharedPtr        _vertexArray;
    LoFiVertexArraySharedPtr        _contourArray;
    LoFiDualMesh*                   _dualMesh;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_MESH_H
