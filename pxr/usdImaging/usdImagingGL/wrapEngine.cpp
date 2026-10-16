//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/vector.h>

#include "pxr/usdImaging/usdImagingGL/engine.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyResultConversions.h"

using namespace std;

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace {

static nb::tuple
_TestIntersection1(
    UsdImagingGLEngine & self,
    const GfMatrix4d &viewMatrix,
    const GfMatrix4d &projectionMatrix,
    const UsdPrim& root,
    const UsdImagingGLRenderParams& params)
{
    GfVec3d hitPoint(0);
    GfVec3d hitNormal(0);
    SdfPath hitPrimPath;
    SdfPath hitInstancerPath;
    int hitInstanceIndex = -1;
    HdInstancerContext hitInstancerContext;

    self.TestIntersection(
        viewMatrix,
        projectionMatrix,
        root,
        params,
        &hitPoint,
        &hitNormal,
        &hitPrimPath,
        &hitInstancerPath,
        &hitInstanceIndex,
        &hitInstancerContext);

    SdfPath topLevelPath = SdfPath::EmptyPath();
    int topLevelInstanceIndex = -1;
    if (hitInstancerContext.size() > 0) {
        topLevelPath = hitInstancerContext[0].first;
        topLevelInstanceIndex = hitInstancerContext[0].second;
    }

    return nb::make_tuple(hitPoint, hitNormal, hitPrimPath,
            hitInstanceIndex, topLevelPath, topLevelInstanceIndex);
}

static
UsdImagingGLEngine::IntersectionResultVector
_TestIntersection2(
    UsdImagingGLEngine & self,
    const UsdImagingGLEngine::PickParams& pickParams,
    const GfMatrix4d& viewMatrix,
    const GfMatrix4d& projectionMatrix,
    const UsdPrim& root,
    const UsdImagingGLRenderParams& params)
{
    UsdImagingGLEngine::IntersectionResultVector result;
    self.TestIntersection(
        pickParams, viewMatrix, projectionMatrix, root, params, &result);
    return result;
}
    
static
nb::list
_ToPyList(const HdInstancerContext &ctx)
{
    nb::list result;
    for (const auto& [path, id] : ctx) {
        result.append(nb::make_tuple(path, id));
    }
    return result;
}
    
static void
_SetLightingState(UsdImagingGLEngine &self, GlfSimpleLightVector const &lights,
                  GlfSimpleMaterial const &material, GfVec4f const &sceneAmbient)
{
    self.SetLightingState(lights, material, sceneAmbient);
}

void _SetOverrideWindowPolicy(UsdImagingGLEngine & self,
                              const nb::object &pyObj)
{
    CameraUtilConformWindowPolicy policy;
    if (nb::try_cast(pyObj, policy)) {
        self.SetOverrideWindowPolicy(policy);
    } else {
        self.SetOverrideWindowPolicy(std::nullopt);
    }
}

} // anonymous namespace

void wrapEngine(nb::module_ &m)
{
    {
        using This = UsdImagingGLEngine;
        
        using Parameters = This::Parameters;
        using PickParams = This::PickParams;
        using IntersectionResult = This::IntersectionResult;

        nb::class_<UsdImagingGLEngine> engineScope(
                m, "Engine", "UsdImaging Renderer class");

        nb::class_<Parameters>(
                engineScope, "Parameters", "Parameters to construct renderer engine")
            .def(nb::init<>())
            .def_rw("rootPath", &Parameters::rootPath)
            .def_rw("excludedPaths", &Parameters::excludedPaths)
            .def_rw("invisedPaths", &Parameters::invisedPaths)
            .def_rw("sceneDelegateID", &Parameters::sceneDelegateID)
            .def_rw("driver", &Parameters::driver)
            .def_rw("rendererPluginId", &Parameters::rendererPluginId)
            .def_rw("gpuEnabled", &Parameters::gpuEnabled)
            .def_rw("displayUnloadedPrimsWithBounds",
                &Parameters::displayUnloadedPrimsWithBounds)
            .def_rw("allowAsynchronousSceneProcessing",
                &Parameters::allowAsynchronousSceneProcessing)
            .def_rw("enableUsdDrawModes",
                &Parameters::enableUsdDrawModes)

        ;

        nb::class_<PickParams>(
                engineScope, "PickParams", "Parameters for TestIntersection")
            .def(nb::init<>())
            .def_rw("resolveMode", &PickParams::resolveMode)
        ;

        nb::class_<IntersectionResult>(
                engineScope, "IntersectionResult", "Results of TestIntersection")
            .def(nb::init<>())
            .def_rw("hitPoint", &IntersectionResult::hitPoint)
            .def_rw("hitNormal", &IntersectionResult::hitNormal)
            .def_rw("hitPrimPath", &IntersectionResult::hitPrimPath)
            .def_rw("hitInstancerPath",
                &IntersectionResult::hitInstancerPath)
            .def_rw("hitInstanceIndex",
                &IntersectionResult::hitInstanceIndex)
            .def_prop_ro("instancerContext",
                +[](const IntersectionResult &result) {
                    return _ToPyList(result.instancerContext); })
        ;

        engineScope
            .def( nb::init<>() )
            .def( nb::init<const SdfPath &, const SdfPathVector&,
                    const SdfPathVector& >() )
            .def( nb::init<const Parameters &>() )
            .def("Render", &UsdImagingGLEngine::Render)
            .def("SetWindowPolicy", &UsdImagingGLEngine::SetWindowPolicy)
            .def("SetRenderViewport", &UsdImagingGLEngine::SetRenderViewport)
            .def("SetCameraPath", &UsdImagingGLEngine::SetCameraPath)
            .def("SetCameraState", &UsdImagingGLEngine::SetCameraState)
            .def("SetLightingState", &_SetLightingState)
            .def("SetSelected", &UsdImagingGLEngine::SetSelected)
            .def("ClearSelected", &UsdImagingGLEngine::ClearSelected)
            .def("AddSelected", &UsdImagingGLEngine::AddSelected)
            .def("SetSelectionColor", &UsdImagingGLEngine::SetSelectionColor)
            .def("TestIntersection", &_TestIntersection1)
            .def("TestIntersection", &_TestIntersection2)
            .def("IsConverged", &UsdImagingGLEngine::IsConverged)
            .def_static("GetRendererPlugins", &UsdImagingGLEngine::GetRendererPlugins)
            .def_static("GetRendererDisplayName",
                    &UsdImagingGLEngine::GetRendererDisplayName)
            .def("GetRendererHgiDisplayName",
                    &UsdImagingGLEngine::GetRendererHgiDisplayName)
            .def("GetCurrentRendererId",
                    &UsdImagingGLEngine::GetCurrentRendererId)
            .def("SetRendererPlugin",
                    &UsdImagingGLEngine::SetRendererPlugin)
            .def("GetRendererAovs",
                    &UsdImagingGLEngine::GetRendererAovs)
            .def("SetRendererAov",
                    &UsdImagingGLEngine::SetRendererAov)
            .def("GetRenderStats",
                    &UsdImagingGLEngine::GetRenderStats)
            .def("GetRendererSettingsList",
                    &UsdImagingGLEngine::GetRendererSettingsList)
            .def("GetRendererSetting", &UsdImagingGLEngine::GetRendererSetting)
            .def("SetRendererSetting", &UsdImagingGLEngine::SetRendererSetting)
            .def("GetActiveRenderPassPrimPath",
                 &This::GetActiveRenderPassPrimPath)
            .def("GetActiveRenderSettingsPrimPath",
                 &This::GetActiveRenderSettingsPrimPath)
            .def("SetActiveRenderPassPrimPath",
                 &This::SetActiveRenderPassPrimPath)
            .def("SetActiveRenderSettingsPrimPath",
                 &UsdImagingGLEngine::SetActiveRenderSettingsPrimPath)
            .def_static("GetAvailableRenderSettingsPrimPaths",
                 &UsdImagingGLEngine::GetAvailableRenderSettingsPrimPaths)
            .def("SetColorCorrectionSettings",
                    &UsdImagingGLEngine::SetColorCorrectionSettings)
            .def_static("IsColorCorrectionCapable",
                &UsdImagingGLEngine::IsColorCorrectionCapable)
            .def("GetRendererCommandDescriptors",
                &UsdImagingGLEngine::GetRendererCommandDescriptors)
            .def("InvokeRendererCommand",
                &UsdImagingGLEngine::InvokeRendererCommand,
                nb::arg("command"),
                nb::arg("args") = HdCommandArgs())
            .def("IsPauseRendererSupported",
                &This::IsPauseRendererSupported)
            .def("PauseRenderer", &This::PauseRenderer)
            .def("ResumeRenderer", &This::ResumeRenderer)
            .def("IsStopRendererSupported", &This::IsStopRendererSupported)
            .def("StopRenderer", &This::StopRenderer)
            .def("RestartRenderer", &This::RestartRenderer)
            .def("SetRenderBufferSize", &This::SetRenderBufferSize)
            .def("SetFraming", &This::SetFraming)
            .def("SetOverrideWindowPolicy", _SetOverrideWindowPolicy)
            .def("PollForAsynchronousUpdates",
                 &This::PollForAsynchronousUpdates)

        ;
    }


    TfPyContainerConversions::from_python_sequence<
        std::vector<GlfSimpleLight>,
        TfPyContainerConversions::variable_capacity_policy>();
}
