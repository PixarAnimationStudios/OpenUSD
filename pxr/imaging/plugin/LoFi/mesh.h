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
//#include "pxr/imaging/hd/extComputation.h"
//#include "pxr/imaging/hd/extComputationUtils.h"
//#include "pxr/imaging/hd/vertexAdjacency.h"
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

    void InfosLog();

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


    HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

    /*
    void _UpdatePrimvarSources(HdSceneDelegate* sceneDelegate,
                               HdDirtyBits dirtyBits);

    TfTokenVector _UpdateComputedPrimvarSources(HdSceneDelegate* sceneDelegate, 
                                                  HdDirtyBits dirtyBits);
    */

    //void _PopulateTopology(HdSceneDelegate* sceneDelegate);

    // Get num points
    const inline int GetNumPoints() const{return _positions.size();};

    // Get num triangles
    const inline int GetNumTriangles() const{return _samples.size()/3;};

    // Get num samples
    const inline int GetNumSamples() const{return _samples.size();};

    // Get positions ptr
    const inline GfVec3f* GetPositionsPtr() const{return _positions.cdata();};

    // Get normals ptr
    const inline GfVec3f* GetNormalsPtr() const{return _normals.cdata();};

    // Get colors ptr
    const inline GfVec3f* GetColorsPtr() const{return _colors.cdata();};

    // Get samples ptr
    const inline GfVec3i* GetSamplesPtr() const{return _samples.cdata();};

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
    
    uint64_t                        _instanceId;
    GfMatrix4f                      _transform;
    VtArray<GfVec3f>                _positions;
    VtArray<GfVec3f>                _normals;
    VtArray<GfVec3f>                _colors;
    VtArray<GfVec2f>                _uvs;
    VtArray<GfVec3i>                _samples;
    LoFiVertexArraySharedPtr        _vertexArray;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_MESH_H
