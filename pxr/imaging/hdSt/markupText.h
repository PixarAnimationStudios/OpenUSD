//
// Copyright 2021 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXR_IMAGING_HD_ST_MARKUP_TEXT_H
#define PXR_IMAGING_HD_ST_MARKUP_TEXT_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/basisCurves.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/markupText.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/array.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdStDrawItem;
using HdSt_MarkupTextTopologySharedPtr =
    std::shared_ptr<class HdSt_MarkupTextTopology>;

/// \class HdStMarkupText
///
/// A representation of text (both single-line and multi-line are supported, and the text style.
/// can be varied).
///
/// There will be one draw item for the whole text. And there will be separate line draw items for
/// overline, underline and strikethrough.
/// The geometry of each character should be triangles formed from the control points saved in the 
/// font file. The geometry is provided by the scene delegate. The text color and opacity is also
/// provided for each vertex. The RPrim will use the shader to form the curves outline of each 
/// character.
///
class HdStMarkupText final: public HdMarkupText
{
public:
    HF_MALLOC_TAG_NEW("new HdStMarkupText");

    HDST_API
    HdStMarkupText(SdfPath const& id);

    HDST_API
    ~HdStMarkupText() override;

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


    // The replacement function of HdStPopulateConstantPrimvars in 
    // MarkupText for handling lineColor and lineOpacity. 
    void _PopulateConstantPrimvars(
        HdSceneDelegate *delegate,
        HdRenderParam *renderParam,
        HdStDrawItem *drawItem,
        HdDirtyBits *dirtyBits,
        HdPrimvarDescriptorVector const& constantPrimvars,
        HdRprimSharedData& sharedData,
        size_t drawItemIndex);

    void _PopulateTopology(HdSceneDelegate *sceneDelegate,
        HdRenderParam *renderParam,
        HdStDrawItem *drawItem,
        HdDirtyBits *dirtyBits);

    void _PopulateLineTopology(HdSceneDelegate* sceneDelegate,
        HdRenderParam* renderParam,
        HdStDrawItem* drawItem,
        HdDirtyBits* dirtyBits,
        HdRprimSharedData& sharedData,
        int drawItemIndex);


    void _PopulateVertexPrimvars(HdSceneDelegate *sceneDelegate,
        HdRenderParam *renderParam,
        HdStDrawItem *drawItem,
        HdDirtyBits *dirtyBits);

    void _PopulateLineVertexPrimvars(HdSceneDelegate* sceneDelegate,
        HdRenderParam* renderParam,
        HdStDrawItem* drawItem,
        HdDirtyBits* dirtyBits,
        HdRprimSharedData& sharedData,
        int drawItemIndex);

    void _UpdateShadersForAllReprs(HdSceneDelegate *sceneDelegate,
        HdRenderParam *renderParam,
        bool updateMaterialShader,
        bool updateGeometricShader);

    void _UpdateLineDrawItemGeometricShader(
        HdSceneDelegate* sceneDelegate,
        HdRenderParam* renderParam,
        HdStDrawItem* drawItem);

private:
    enum DrawingCoord {
        Topology = HdDrawingCoord::CustomSlotsBegin,
        InstancePrimvar  // has to be at the very end
    };

    enum LineDrawingCoord {
        lineTopology = HdDrawingCoord::CustomSlotsBegin,
        linePointsTopology,
        lineInstancePrimvar  // has to be at the very end
    };

    enum DirtyBits : HdDirtyBits {
        DirtyIndices = HdChangeTracker::CustomBitsBegin
    };

    void _UpdateDrawItem(HdSceneDelegate *sceneDelegate,
        HdRenderParam *renderParam,
        HdStDrawItem *drawItem,
        HdDirtyBits *dirtyBits,
        HdRprimSharedData& sharedData,
        size_t drawItemIndex);

    HdSt_MarkupTextTopologySharedPtr _topology;
    HdTopology::ID _topologyId;
    HdDirtyBits _customDirtyBitsInUse;
    int _refineLevel;
    bool _displayOpacity : 1;

    // Decoration line(underline, overline)
    HdSt_BasisCurvesTopologySharedPtr _lineTopology;
    HdTopology::ID _lineTopologyId;
    bool _occludedSelectionShowsThrough : 1;
    bool _pointsShadingEnabled : 1;

    // shared data for line draw item.
    std::vector<HdRprimSharedData> _sharedDataLines;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_MARKUP_TEXT_H
