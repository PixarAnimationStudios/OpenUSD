//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_VOLUME_H
#define PXR_IMAGING_HD_ST_VOLUME_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/volume.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdStDrawItem;

/// Represents a Volume Prim.
///
class HdStVolume final : public HdVolume {
public:
    HDST_API
    HdStVolume(SdfPath const& id);
    HDST_API
    ~HdStVolume() override;

    HDST_API
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    HDST_API
    void UpdateRenderTag(HdSceneDelegate *delegate,
                         HdRenderParam *renderParam) override;

    HDST_API
    void Sync(HdSceneDelegate* delegate,
              HdRenderParam*   renderParam,
              HdDirtyBits*     dirtyBits,
              TfToken const  &reprToken) override;

    HDST_API
    void Finalize(HdRenderParam *renderParam) override;

    /// Default step size used for raymarching
    HDST_API
    static const float defaultStepSize;

    /// Default step size used for raymarching for lighting computation
    HDST_API
    static const float defaultStepSizeLighting;

    /// Default memory limit for a field texture (in Mb) if not
    /// overridden by field prim with textureMemory.
    HDST_API
    static const float defaultMaxTextureMemoryPerField;

protected:
    void _InitRepr(TfToken const &reprToken,
                   HdDirtyBits* dirtyBits) override;

    HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

    void _UpdateRepr(HdSceneDelegate *sceneDelegate,
                     HdRenderParam *renderParam,
                     TfToken const &reprToken,
                     HdDirtyBits *dirtyBitsState);

private:
    void _UpdateDrawItem(HdSceneDelegate *sceneDelegate,
                         HdRenderParam *renderParam,
                         HdStDrawItem *drawItem,
                         HdDirtyBits *dirtyBits);

    enum DrawingCoord {
        InstancePrimvar = HdDrawingCoord::CustomSlotsBegin
    };

    HdReprSharedPtr _volumeRepr;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_ST_VOLUME_H
