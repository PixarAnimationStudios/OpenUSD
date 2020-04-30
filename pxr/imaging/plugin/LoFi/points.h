//
// Copyright 2020 benmalartre
//
// unlicensed
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_POINTS_H
#define PXR_IMAGING_PLUGIN_LOFI_POINTS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/points.h"
#include "pxr/imaging/hd/sceneDelegate.h"

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

/// \class LoFiPoints
///
class LoFiPoints final : public HdPoints 
{

public:
    HF_MALLOC_TAG_NEW("new LoFiPoints");

    /// LoFiPoints constructor
    LoFiPoints(SdfPath const& id, SdfPath const& instancerId = SdfPath());

    /// LoFiPoints destructor.
    ~LoFiPoints() override = default;

    HdDirtyBits GetInitialDirtyBitsMask() const override;

    void Sync(HdSceneDelegate* sceneDelegate,
              HdRenderParam*   renderParam,
              HdDirtyBits*     dirtyBits,
              TfToken const    &reprToken) override;

protected:
    void _InitRepr(
        TfToken const &reprToken,
        HdDirtyBits *dirtyBits) override;

    void _PopulatePoints( HdSceneDelegate*              sceneDelegate,
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
    LoFiPoints(const LoFiPoints&) = delete;
    LoFiPoints &operator =(const LoFiPoints&) = delete;

private:
    
    uint64_t                        _instanceId;
    uint64_t                        _numPoints;
    VtArray<GfVec3f>                _points;
    VtArray<float>                  _widths;
    VtArray<GfVec3f>                _normals;
    VtArray<GfVec3f>                _colors;
    VtArray<GfVec2f>                _uvs;
    VtArray<int>                    _samples;
    LoFiTopology                    _topology;
    LoFiVertexArraySharedPtr        _vertexArray;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_POINTS_H
