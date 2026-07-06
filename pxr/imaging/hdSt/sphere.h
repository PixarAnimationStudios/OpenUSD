//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_SPHERE_H
#define PXR_IMAGING_HD_ST_SPHERE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/drawingCoord.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/array.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdStSphere
///
/// Represents a sphere prim that can be rendered natively by Storm.
///
class HdStSphere final : public HdRprim
{
public:
    HF_MALLOC_TAG_NEW("new HdStSphere");

    HDST_API
    HdStSphere(SdfPath const& id);

    HDST_API
    ~HdStSphere() override;

    HDST_API
    TfTokenVector const & GetBuiltinPrimvarNames() const override;

    HDST_API
    void UpdateRenderTag(HdSceneDelegate *delegate,
                         HdRenderParam *renderParam) override;

    HDST_API
    void Sync(HdSceneDelegate *delegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits,
              TfToken const   &reprToken) override;

    HDST_API
    void Finalize(HdRenderParam *renderParam) override;

    HDST_API
    HdDirtyBits GetInitialDirtyBitsMask() const override;

protected:
    HDST_API
    void _InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits) override;

    HDST_API
    HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

    void _UpdateRepr(HdSceneDelegate *sceneDelegate,
                     HdRenderParam *renderParam,
                     TfToken const &reprToken,
                     HdDirtyBits *dirtyBitsState);

    void _UpdateDrawItem(HdSceneDelegate *sceneDelegate,
                         HdRenderParam *renderParam,
                         HdStDrawItem *drawItem,
                         HdDirtyBits *dirtyBits);

    void _UpdateDrawItemGeometricShader(HdSceneDelegate *sceneDelegate,
                                        HdRenderParam *renderParam,
                                        HdStDrawItem *drawItem);

    void _UpdateShadersForAllReprs(HdSceneDelegate *sceneDelegate,
                                   HdRenderParam *renderParam,
                                   bool updateMaterialNetworkShader,
                                   bool updateGeometricShader);

    void _UpdateMaterialTagsForAllReprs(HdSceneDelegate *sceneDelegate,
                                        HdRenderParam *renderParam);

    void _PopulateCustomConstantPrimvars(HdSceneDelegate *sceneDelegate,
                                         HdRenderParam *renderParam,
                                         HdStDrawItem *drawItem);
        
    void _PopulateIndexBuffer(HdSceneDelegate *sceneDelegate,
                                HdRenderParam *renderParam,
                                HdStDrawItem *drawItem);

private:
    HdReprSharedPtr _sphereDefaultRepr;

    HdCullStyle _cullStyle;

    // In the future, we might want to create a map for the vertex counts
    // required for different reprs (such as wireframes)
    const uint32_t _vertexCount;

    bool _doubleSided : 1;
    bool _displayOpacityFromInstancer : 1;
    bool _displayOpacityFromPrimvars : 1;
    bool _displayInOverlay : 1;
    bool _occludedSelectionShowsThrough : 1;

    enum DrawingCoord {
        InstancePrimvar = HdDrawingCoord::CustomSlotsBegin
    };
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_SPHERE_H
