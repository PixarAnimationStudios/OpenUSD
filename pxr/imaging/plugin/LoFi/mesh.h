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
    VtArray<GfVec3f>                _normals;
    VtArray<GfVec3f>                _colors;
    VtArray<GfVec2f>                _uvs;
    VtArray<GfVec3i>                _samples;
    LoFiVertexArraySharedPtr        _vertexArray;
    GfVec3f                         _displayColor;
    bool                            _varyingColor;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_MESH_H
