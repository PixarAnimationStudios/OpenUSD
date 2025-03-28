//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

/// \file usdImagingGL/renderParams.h

#ifndef PXR_USD_IMAGING_USD_IMAGING_GL_RENDER_PARAMS_H
#define PXR_USD_IMAGING_USD_IMAGING_GL_RENDER_PARAMS_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImagingGL/api.h"

#include "pxr/usd/usd/timeCode.h"

#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

enum class UsdImagingGLDrawMode 
{
    DRAW_POINTS,
    DRAW_WIREFRAME,
    DRAW_WIREFRAME_ON_SURFACE,
    DRAW_SHADED_FLAT,
    DRAW_SHADED_SMOOTH,
    DRAW_GEOM_ONLY,
    DRAW_GEOM_FLAT,
    DRAW_GEOM_SMOOTH
};

// Note: some assumptions are made about the order of these enums, so please
// be careful when updating them.
enum class UsdImagingGLCullStyle 
{
    CULL_STYLE_NO_OPINION,
    CULL_STYLE_NOTHING,
    CULL_STYLE_BACK,
    CULL_STYLE_FRONT,
    CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED,

    CULL_STYLE_COUNT
};


/// \class UsdImagingGLRenderParams
///
/// Used as an arguments class for various methods in UsdImagingGLEngine.
///
class UsdImagingGLRenderParams 
{
public:

    using ClipPlanesVector = std::vector<GfVec4d>;
    using BBoxVector = std::vector<GfBBox3d>;

    UsdTimeCode frame;
    float complexity;
    UsdImagingGLDrawMode drawMode;
    bool showGuides;
    bool showProxy;
    bool showRender;
    bool forceRefresh;
    bool flipFrontFacing;
    UsdImagingGLCullStyle cullStyle;
    bool enableLighting;
    bool enableSampleAlphaToCoverage;
    bool applyRenderState;
    bool gammaCorrectColors;
    bool highlight;
    GfVec4f overrideColor;
    GfVec4f wireframeColor;
    float alphaThreshold; // threshold < 0 implies automatic
    ClipPlanesVector clipPlanes;
    bool enableSceneMaterials;
    bool enableSceneLights;
    // Respect USD's model:drawMode attribute...
    bool enableUsdDrawModes;
    GfVec4f clearColor;
    TfToken colorCorrectionMode;
    // Optional OCIO color setings, only valid when colorCorrectionMode==HdxColorCorrectionTokens->openColorIO
    int lut3dSizeOCIO;
    TfToken ocioDisplay;
    TfToken ocioView;
    TfToken ocioColorSpace;
    TfToken ocioLook;
    // BBox settings
    BBoxVector bboxes;
    GfVec4f bboxLineColor;
    float bboxLineDashSize;

    inline UsdImagingGLRenderParams();

    inline bool operator==(const UsdImagingGLRenderParams &other) const;

    inline bool operator!=(const UsdImagingGLRenderParams &other) const {
        return !(*this == other);
    }
};


UsdImagingGLRenderParams::UsdImagingGLRenderParams() :
    frame(UsdTimeCode::EarliestTime()),
    complexity(1.0),
    drawMode(UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH),
    showGuides(false),
    showProxy(true),
    showRender(false),
    forceRefresh(false),
    flipFrontFacing(false),
    cullStyle(UsdImagingGLCullStyle::CULL_STYLE_NOTHING),
    enableLighting(true),
    enableSampleAlphaToCoverage(false),
    applyRenderState(true),
    gammaCorrectColors(true),
    highlight(false),
    overrideColor(.0f, .0f, .0f, .0f),
    wireframeColor(.0f, .0f, .0f, .0f),
    alphaThreshold(-1),
    clipPlanes(),
    enableSceneMaterials(true),
    enableSceneLights(true),
    enableUsdDrawModes(true),
    clearColor(0,0,0,1),
    lut3dSizeOCIO(65),
    bboxLineColor(1),
    bboxLineDashSize(3)
{
}

bool 
UsdImagingGLRenderParams::operator==(const UsdImagingGLRenderParams &other) 
    const 
{
    return frame                       == other.frame
        && complexity                  == other.complexity
        && drawMode                    == other.drawMode
        && showGuides                  == other.showGuides
        && showProxy                   == other.showProxy
        && showRender                  == other.showRender
        && forceRefresh                == other.forceRefresh
        && flipFrontFacing             == other.flipFrontFacing
        && cullStyle                   == other.cullStyle
        && enableLighting              == other.enableLighting
        && enableSampleAlphaToCoverage == other.enableSampleAlphaToCoverage
        && applyRenderState            == other.applyRenderState
        && gammaCorrectColors          == other.gammaCorrectColors
        && highlight                   == other.highlight
        && overrideColor               == other.overrideColor
        && wireframeColor              == other.wireframeColor
        && alphaThreshold              == other.alphaThreshold
        && clipPlanes                  == other.clipPlanes
        && enableSceneMaterials        == other.enableSceneMaterials
        && enableSceneLights           == other.enableSceneLights
        && enableUsdDrawModes          == other.enableUsdDrawModes
        && clearColor                  == other.clearColor
        && colorCorrectionMode         == other.colorCorrectionMode
        && ocioDisplay                 == other.ocioDisplay
        && ocioView                    == other.ocioView
        && ocioColorSpace              == other.ocioColorSpace
        && ocioLook                    == other.ocioLook
        && lut3dSizeOCIO               == other.lut3dSizeOCIO
        && bboxes                      == other.bboxes
        && bboxLineColor               == other.bboxLineColor
        && bboxLineDashSize            == other.bboxLineDashSize;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_GL_RENDER_PARAMS_H
