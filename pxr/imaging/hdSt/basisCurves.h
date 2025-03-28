//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_BASIS_CURVES_H
#define PXR_IMAGING_HD_ST_BASIS_CURVES_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/basisCurves.h"
#include "pxr/imaging/hd/drawingCoord.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/array.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdStDrawItem;
using HdSt_BasisCurvesTopologySharedPtr =
    std::shared_ptr<class HdSt_BasisCurvesTopology>;

/// \class HdStBasisCurves
///
/// A collection of curves using a particular basis.
///
/// Render mode is dependent on both the HdBasisCurvesGeomStyle, refinement 
/// level, and the authored primvars.
///
/// If style is set to HdBasisCurvesGeomStyleWire, the curves will always draw
/// as infinitely thin wires.  Cubic curves will be refined if complexity is
/// above 0, otherwise they draw the unrefined control points. (This may
/// provide a misleading representation for Catmull-Rom and Bspline curves.)
///
/// If style is set to HdBasisCurvesGeomStylePatch, the curves will draw as
/// patches ONLY if refinement level is above 0.  Otherwise, they draw
/// as the unrefined control points (see notes on HdBasisCurvesGeomStyleWire).
///
/// Curves rendered as patches may be rendered as ribbons or halftubes.
/// Curves with primvar authored normals will always render as ribbons.
/// Curves without primvar authored normals are assumed to be round and may be
/// rendered in one of three styles:
///   * if complexity is 1, a camera facing normal is used
///   * if complexity is 2, a fake "bumped" round normal is used
///   * if complexity is 3 or above, the patch is displaced into a half tube
/// We plan for future checkins will remove the need for the camera facing normal
/// mode, using the fake "bumped" round normal instead.
class HdStBasisCurves final: public HdBasisCurves
{
public:
    HF_MALLOC_TAG_NEW("new HdStBasisCurves");

    HDST_API
    HdStBasisCurves(SdfPath const& id);

    HDST_API
    ~HdStBasisCurves() override;

    HDST_API
    void UpdateRenderTag(HdSceneDelegate *delegate,
                         HdRenderParam *renderParam) override;

    HDST_API
    void Sync(HdSceneDelegate *delegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits,
              TfToken const   &reprToken) override;

    HDST_API
    void Finalize(HdRenderParam   *renderParam) override;

    HDST_API
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    HDST_API
    TfTokenVector const & GetBuiltinPrimvarNames() const override;

protected:
    HDST_API
    void _InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits) override;

    HDST_API
    HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

    void _UpdateRepr(HdSceneDelegate *sceneDelegate,
                     HdRenderParam *renderParam,
                     TfToken const &reprToken,
                     HdDirtyBits *dirtyBitsState);

    void _PopulateTopology(HdSceneDelegate *sceneDelegate,
                           HdRenderParam *renderParam,
                           HdStDrawItem *drawItem,
                           HdDirtyBits *dirtyBits,
                           const HdBasisCurvesReprDesc &desc);

    void _PopulateVertexPrimvars(HdSceneDelegate *sceneDelegate,
                                 HdRenderParam *renderParam,
                                 HdStDrawItem *drawItem,
                                 HdDirtyBits *dirtyBits);
    
    void _PopulateVaryingPrimvars(HdSceneDelegate *sceneDelegate,
                                  HdRenderParam *renderParam,
                                  HdStDrawItem *drawItem,
                                  HdDirtyBits *dirtyBits);

    void _PopulateElementPrimvars(HdSceneDelegate *sceneDelegate,
                                  HdRenderParam *renderParam,
                                  HdStDrawItem *drawItem,
                                  HdDirtyBits *dirtyBits);

private:
    enum DrawingCoord {
        HullTopology = HdDrawingCoord::CustomSlotsBegin,
        PointsTopology,
        InstancePrimvar  // has to be at the very end
    };

    enum DirtyBits : HdDirtyBits {
        DirtyIndices        = HdChangeTracker::CustomBitsBegin,
        DirtyHullIndices    = (DirtyIndices       << 1),
        DirtyPointsIndices  = (DirtyHullIndices   << 1)
    };

    // When processing primvars, these will get set to if we determine that
    // we should do cubic basis interpolation on the normals and widths.
    // NOTE: I worry that it may be possible for these to get out of sync.
    // The right long term fix is likely to maintain proper separation between 
    // varying and vertex primvars throughout the HdSt rendering pipeline.
    bool _basisWidthInterpolation = false;
    bool _basisNormalInterpolation = false;

    bool _SupportsRefinement(int refineLevel);
    bool _SupportsUserWidths(HdStDrawItem* drawItem);
    bool _SupportsUserNormals(HdStDrawItem* drawItem);
    
    void _UpdateDrawItem(HdSceneDelegate *sceneDelegate,
                         HdRenderParam *renderParam,
                         HdStDrawItem *drawItem,
                         HdDirtyBits *dirtyBits,
                         const HdBasisCurvesReprDesc &desc);

    void _UpdateDrawItemGeometricShader(HdSceneDelegate *sceneDelegate,
                                        HdRenderParam *renderParam,
                                        HdStDrawItem *drawItem,
                                        const HdBasisCurvesReprDesc &desc);
    
    void _UpdateShadersForAllReprs(HdSceneDelegate *sceneDelegate,
                                   HdRenderParam *renderParam,
                                   bool updateMaterialNetworkShader,
                                   bool updateGeometricShader);

    void _UpdateMaterialTagsForAllReprs(HdSceneDelegate *sceneDelegate,
                                        HdRenderParam *renderParam);

    HdSt_BasisCurvesTopologySharedPtr _topology;
    HdTopology::ID _topologyId;
    HdDirtyBits _customDirtyBitsInUse;
    int _refineLevel;  // XXX: could be moved into HdBasisCurveTopology.
    bool _displayOpacity : 1;
    bool _displayInOverlay : 1;
    bool _occludedSelectionShowsThrough : 1;
    bool _pointsShadingEnabled : 1;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_BASIS_CURVES_H
