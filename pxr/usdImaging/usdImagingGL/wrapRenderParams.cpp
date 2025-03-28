//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/tuple.hpp"
#include "pxr/external/boost/python.hpp"
#include "pxr/external/boost/python/converter/from_python.hpp"

#include "pxr/usdImaging/usdImagingGL/renderParams.h"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void
wrapRenderParams()
{
    // Wrap the DrawMode enum. Accessible as UsdImagingGL.DrawMode
    enum_<UsdImagingGLDrawMode>("DrawMode")
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
    enum_<UsdImagingGLCullStyle>("CullStyle")
        .value("CULL_STYLE_NOTHING", UsdImagingGLCullStyle::CULL_STYLE_NOTHING)
        .value("CULL_STYLE_BACK", UsdImagingGLCullStyle::CULL_STYLE_BACK)
        .value("CULL_STYLE_FRONT", UsdImagingGLCullStyle::CULL_STYLE_FRONT)
        .value("CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED", 
            UsdImagingGLCullStyle::CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED)
        ;

    // Wrap the UsdImagingGLRenderParams struct. Accessible as 
    // UsdImagingGL.RenderParams
    using Params = UsdImagingGLRenderParams;
    class_<UsdImagingGLRenderParams>("RenderParams", "Render parameters")
        .def_readwrite("frame", &Params::frame)
        .def_readwrite("complexity", &Params::complexity)
        .def_readwrite("drawMode", &Params::drawMode)
        .def_readwrite("showGuides", &Params::showGuides)
        .def_readwrite("showRender", &Params::showRender)
        .def_readwrite("showProxy", &Params::showProxy)
        .def_readwrite("forceRefresh", &Params::forceRefresh)
        .def_readwrite("cullStyle", &Params::cullStyle)
        .def_readwrite("enableLighting", &Params::enableLighting)
        .def_readwrite("enableSampleAlphaToCoverage", 
            &Params::enableSampleAlphaToCoverage)
        .def_readwrite("applyRenderState", &Params::applyRenderState)
        .def_readwrite("gammaCorrectColors", &Params::gammaCorrectColors)
        .def_readwrite("overrideColor", &Params::overrideColor)
        .def_readwrite("wireframeColor", &Params::wireframeColor)
        .def_readwrite("clipPlanes", &Params::clipPlanes)
        .def_readwrite("highlight", &Params::highlight)
        .def_readwrite("enableSceneMaterials", &Params::enableSceneMaterials)
        .def_readwrite("enableSceneLights", &Params::enableSceneLights)
        .def_readwrite("enableUsdDrawModes", &Params::enableUsdDrawModes)
        .def_readwrite("colorCorrectionMode", &Params::colorCorrectionMode)
        .def_readwrite("clearColor", &Params::clearColor)
        .def_readwrite("ocioDisplay", &Params::ocioDisplay)
        .def_readwrite("ocioView", &Params::ocioView)
        .def_readwrite("ocioColorSpace", &Params::ocioColorSpace)
        .def_readwrite("ocioLook", &Params::ocioLook)
        .def_readwrite("bboxes", &Params::bboxes)
        .def_readwrite("bboxLineColor", &Params::bboxLineColor)
        .def_readwrite("bboxLineDashSize", &Params::bboxLineDashSize)
        ;
}
