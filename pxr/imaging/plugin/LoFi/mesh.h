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

protected:
    void _InitRepr(
        TfToken const &reprToken,
        HdDirtyBits *dirtyBits) override;

    void _PopulateMesh( HdSceneDelegate*              sceneDelegate,
                        HdDirtyBits*                  dirtyBits,
                        TfToken const                 &reprToken,
                        LoFiResourceRegistrySharedPtr registry);

    void _PopulatePrimvar(HdSceneDelegate* sceneDelegate,
                          HdInterpolation interpolation,
                          LoFiVertexBufferChannelBits channel,
                          const VtValue& value,
                          bool needReallocate);
  

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
    const inline int GetNumTriangles() const{return _triangles.size();};

    // Get positions ptr
    const inline GfVec3f* GetPositionsPtr() const{return _positions.cdata();};

    // Get normals ptr
    const inline GfVec3f* GetNormalsPtr() const{return _normals.cdata();};

    // Get colors ptr
    const inline GfVec3f* GetColorsPtr() const{return _colors.cdata();};

     // Get indices ptr
    const inline int* GetIndicesPtr() const{return _triangles.cdata();};

    // Get samples ptr
    const inline int* GetSamplesPtr() const{return _samples.cdata();};

    // This class does not support copying.
    LoFiMesh(const LoFiMesh&) = delete;
    LoFiMesh &operator =(const LoFiMesh&) = delete;

private:
    uint64_t                        _instanceId;
    GfMatrix4f                      _transform;
    VtArray<GfVec3f>                _positions;
    VtArray<GfVec3f>                _normals;
    VtArray<GfVec3f>                _colors;
    VtArray<GfVec2f>                _uvs;
    VtArray<int>                    _triangles;
    VtArray<int>                    _samples;
    LoFiVertexArraySharedPtr        _vertexArray;

    // A local cache of primvar scene data. "data" is a copy-on-write handle to
    // the actual primvar buffer, and "interpolation" is the interpolation mode
    // to be used. This cache is used in _PopulateRtMesh to populate the
    // primvar sampler map in the prototype context, which is used for shading.
    struct PrimvarSource {
        VtValue data;
        HdInterpolation interpolation;
    };
    TfHashMap<TfToken, PrimvarSource, TfToken::HashFunctor> _primvarSourceMap;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_MESH_H
