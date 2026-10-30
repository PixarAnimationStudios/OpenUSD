//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>

#include "pxr/usdImaging/usdImagingGL/renderParams.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void
wrapRenderParams(nb::module_ &m)
{
    // Wrap the DrawMode enum. Accessible as UsdImagingGL.DrawMode
    nb::enum_<UsdImagingGLDrawMode>(m, "DrawMode")
        .value("DRAW_POINTS", UsdImagingGLDrawMode::DRAW_POINTS)
        .value("DRAW_WIREFRAME", UsdImagingGLDrawMode::DRAW_WIREFRAME)
        .value("DRAW_WIREFRAME_ON_SURFACE", 
            UsdImagingGLDrawMode::DRAW_WIREFRAME_ON_SURFACE)
        .value("DRAW_SHADED_FLAT", UsdImagingGLDrawMode::DRAW_SHADED_FLAT)
        .value("DRAW_SHADED_SMOOTH", UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH)
        .value("DRAW_GEOM_ONLY", UsdImagingGLDrawMode::DRAW_GEOM_ONLY)
        .value("DRAW_GEOM_FLAT", UsdImagingGLDrawMode::DRAW_GEOM_FLAT)
        .value("DRAW_GEOM_SMOOTH", UsdImagingGLDrawMode::DRAW_GEOM_SMOOTH)
        ;

    // Wrap the CullStyle enum. Accessible as UsdImagingGL.CullStyle
    nb::enum_<UsdImagingGLCullStyle>(m, "CullStyle")
        .value("CULL_STYLE_NOTHING", UsdImagingGLCullStyle::CULL_STYLE_NOTHING)
        .value("CULL_STYLE_BACK", UsdImagingGLCullStyle::CULL_STYLE_BACK)
        .value("CULL_STYLE_FRONT", UsdImagingGLCullStyle::CULL_STYLE_FRONT)
        .value("CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED", 
            UsdImagingGLCullStyle::CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED)
        ;

    // Wrap the UsdImagingGLRenderParams struct. Accessible as 
    // UsdImagingGL.RenderParams
    using Params = UsdImagingGLRenderParams;
    nb::class_<UsdImagingGLRenderParams>(m, "RenderParams", "Render parameters")
        .def(nb::init<>())
        .def_rw("frame", &Params::frame)
        .def_rw("complexity", &Params::complexity)
        .def_rw("drawMode", &Params::drawMode)
        .def_rw("showGuides", &Params::showGuides)
        .def_rw("showRender", &Params::showRender)
        .def_rw("showProxy", &Params::showProxy)
        .def_rw("forceRefresh", &Params::forceRefresh)
        .def_rw("cullStyle", &Params::cullStyle)
        .def_rw("enableLighting", &Params::enableLighting)
        .def_rw("enableSampleAlphaToCoverage", 
            &Params::enableSampleAlphaToCoverage)
        .def_rw("applyRenderState", &Params::applyRenderState)
        .def_rw("gammaCorrectColors", &Params::gammaCorrectColors)
        .def_rw("overrideColor", &Params::overrideColor)
        .def_rw("wireframeColor", &Params::wireframeColor)
        .def_rw("clipPlanes", &Params::clipPlanes)
        .def_rw("highlight", &Params::highlight)
        .def_rw("enableSceneMaterials", &Params::enableSceneMaterials)
        .def_rw("enableSceneLights", &Params::enableSceneLights)
        .def_rw("enableUsdDrawModes", &Params::enableUsdDrawModes)
        .def_rw("colorCorrectionMode", &Params::colorCorrectionMode)
        .def_rw("clearColor", &Params::clearColor)
        .def_rw("ocioDisplay", &Params::ocioDisplay)
        .def_rw("ocioView", &Params::ocioView)
        .def_rw("ocioColorSpace", &Params::ocioColorSpace)
        .def_rw("ocioLook", &Params::ocioLook)
        .def_rw("bboxes", &Params::bboxes)
        .def_rw("bboxLineColor", &Params::bboxLineColor)
        .def_rw("bboxLineDashSize", &Params::bboxLineDashSize)
        ;
}
