//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_POINTS_H
#define PXR_IMAGING_HD_ST_POINTS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/drawingCoord.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/points.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/array.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdStPoints
///
/// Points.
///
class HdStPoints final : public HdPoints
{
public:
    HF_MALLOC_TAG_NEW("new HdStPoints");

    HDST_API
    HdStPoints(SdfPath const& id);

    HDST_API
    ~HdStPoints() override;

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

    void _PopulateVertexPrimvars(HdSceneDelegate *sceneDelegate,
                                 HdRenderParam *renderParam,
                                 HdStDrawItem *drawItem,
                                 HdDirtyBits *dirtyBitsState);

private:
    HdReprSharedPtr _smoothHullRepr;

    bool _displayOpacity;
    bool _displayInOverlay;

    enum DrawingCoord {
        InstancePrimvar = HdDrawingCoord::CustomSlotsBegin
    };

    void _UpdateDrawItem(HdSceneDelegate *sceneDelegate,
                         HdRenderParam *renderParam,
                         HdStDrawItem *drawItem,
                         HdDirtyBits *dirtyBits);
    
    void _UpdateMaterialTagsForAllReprs(HdSceneDelegate *sceneDelegate,
                                        HdRenderParam *renderParam);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_POINTS_H
