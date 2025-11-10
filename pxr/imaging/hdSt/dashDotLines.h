//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_DASH_DOT_LINES_H
#define PXR_IMAGING_HD_ST_DASH_DOT_LINES_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/dashDotLines.h"
#include "pxr/imaging/hd/drawingCoord.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/bufferSource.h"

#include "pxr/imaging/hdSt/material.h"
#include "pxr/imaging/hdSt/materialNetworkShader.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/array.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdStDrawItem;
using HdSt_DashDotLinesTopologySharedPtr =
    std::shared_ptr<class HdSt_DashDotLinesTopology>;

/// \struct DashDotPattern
///
/// A structure to hold dash and dot pattern information.
/// 
struct DashDotPattern
{
    // The _pattern is a vector of pairs, where each pair contains
    // the length of previous gap and the length of the current dash.
    // If the length of the current dash is zero, it is a dot.
    VtVec2fArray _pattern;
    float _period = 0.0f;

    bool operator==(DashDotPattern const& other) const
    {
        return (_pattern == other._pattern && _period == other._period);
    }
    bool operator!=(DashDotPattern const& other) const
    {
        return !(*this == other);
    }
};

/// \brief Convert a DashDotPattern to a TfToken as a file path.
TfToken& PatternToPathToken(DashDotPattern const& pattern);
/// \brief Convert a TfToken to a DashDotPattern.
const DashDotPattern& PathTokenToPattern(TfToken const& token);

/// \class HdStDashDotLines
///
/// A collection of lines which can have dashdot styles.
/// If the line doesn't have dashdot style, it will be a normal
/// line with uniform width.
/// If it has dashdot style, the style will be defined in a pattern.
/// The pattern defines the position and length of each dash and the
/// position of each dot, in one period.
/// The unit of line width and the pattern could be in screen space 
/// or world space.
/// 
/// Currently we have two methods to get the pattern in the shader.
/// One is to pass the pattern to the shader via an array of floats,
/// and the other is to use a texture to store the pattern. The first
/// method is more efficient for simple patterns, while the second
/// method is more efficient for complex patterns. And the second
/// method will round the pattern and period to the nearest integer.
/// 
class HdStDashDotLines final: public HdDashDotLines
{
public:
    HF_MALLOC_TAG_NEW("new HdStDashDotLines");

    HDST_API
    HdStDashDotLines(SdfPath const& id);

    HDST_API
    ~HdStDashDotLines() override;

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

    HdSt_MaterialNetworkShaderSharedPtr
    _GetMaterialNetworkShader(HdSceneDelegate* delegate);

    void _PopulateTopology(HdSceneDelegate *sceneDelegate,
                           HdRenderParam *renderParam,
                           HdStDrawItem *drawItem,
                           HdDirtyBits *dirtyBits);

    void _PopulateConstantPrimvars(HdSceneDelegate *sceneDelegate,
                                  HdRenderParam *renderParam,
                                  HdStDrawItem *drawItem,
                                  HdDirtyBits *dirtyBits,
                                  HdPrimvarDescriptorVector const& constantPrimvars);

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
        InstancePrimvar = HdDrawingCoord::CustomSlotsBegin,
    };

    bool _SupportsRefinement(int refineLevel);
    bool _SupportsUserWidths(HdStDrawItem* drawItem);
    bool _SupportsUserNormals(HdStDrawItem* drawItem);
    
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

    bool _HandleAdjVertexInfo(const VtValue& points,
                              const VtIntArray& curveVertexCounts,
                              HdBufferSourceSharedPtrVector *sources);

    bool _HandleAccumulatedWidth(HdSceneDelegate* sceneDelegate,
                                 const VtValue& points,
                                 const VtIntArray& curveVertexCounts,
                                 HdBufferSourceSharedPtrVector *sources);

    // Get the fallback material network for the dashdot styled lines.
    HdSt_MaterialNetworkShaderSharedPtr
    _GetDashDotMaterialNetworkShader(HdSceneDelegate* delegate);

    // Create the fallback material network for the dashdot styled lines.
    static void _CreateDashDotFallbackMaterialPrim();

    HdSt_DashDotLinesTopologySharedPtr _topology;
    HdTopology::ID _topologyId;
    HdDirtyBits _customDirtyBitsInUse;
    int _refineLevel; 
    bool _displayOpacity : 1;
    bool _displayInOverlay : 1;
    bool _occludedSelectionShowsThrough : 1;

    HdStTextureHandleSharedPtr _patternTexture;

    static HdStMaterial* _fallbackMaterial;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_DASH_DOT_LINES_H
