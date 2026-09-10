//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_IMPLICIT_SURFACE_H
#define PXR_IMAGING_HD_ST_IMPLICIT_SURFACE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/drawingCoord.h"
#include "pxr/imaging/hd/implicitSurface.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/array.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdStDrawItem;

/// \class HdStImplicitSurface
///
/// Base class for natively supported implicit surfaces (e.g., spheres)
///
class HdStImplicitSurface : public HdImplicitSurface
{
public:
    HDST_API
    ~HdStImplicitSurface() override;

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
    HdStImplicitSurface(SdfPath const &id,
                        TfToken const &primType,
                        HdBufferSpecVector customSpecs);

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
                                        HdStDrawItem *drawItem,
                                        HdImplicitSurfaceReprDesc const &desc);

    void _UpdateShadersForAllReprs(HdSceneDelegate *sceneDelegate,
                                   HdRenderParam *renderParam,
                                   bool updateMaterialNetworkShader,
                                   bool updateGeometricShader);

    void _UpdateMaterialTagsForAllReprs(HdSceneDelegate *sceneDelegate,
                                        HdRenderParam *renderParam);

    void _PopulateIndexBuffer(HdSceneDelegate *sceneDelegate,
                              HdRenderParam *renderParam,
                              HdStDrawItem *drawItem);

    // Needs to be implemented by each prim-type-specific child class
    virtual void _PopulateCustomConstantPrimvars(
                                         HdSceneDelegate *sceneDelegate,
                                         HdRenderParam *renderParam,
                                         HdStDrawItem *drawItem) = 0;

    // The type of implicit surface (e.g., sphere, cone)
    const TfToken _primType;

    // Buffer specs for prim-type-specific constant primvars (e.g., radius, height).
    // Needs to be populated by each prim-type-specific child class.
    const HdBufferSpecVector _customSpecs;

private:
    HdCullStyle _cullStyle{HdCullStyleDontCare};

    // In the future, we might want to create a map for the vertex counts
    // required for different reprs (such as points)
    const uint32_t _vertexCount{4};

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

#endif // PXR_IMAGING_HD_ST_IMPLICIT_SURFACE_H
