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

#include "pxr/usdImaging/usdImagingGL/engine.h"

#include "pxr/usdImaging/usdImaging/delegate.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyResultConversions.h"

using namespace std;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

static pxr_boost::python::tuple
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

    return pxr_boost::python::make_tuple(hitPoint, hitNormal, hitPrimPath,
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
pxr_boost::python::list
_ToPyList(const HdInstancerContext &ctx)
{
    pxr_boost::python::list result;
    for (const auto& [path, id] : ctx) {
        result.append(pxr_boost::python::make_tuple(path, id));
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
                              const object &pyObj)
{
    extract<CameraUtilConformWindowPolicy> extractor(pyObj);
    if (extractor.check()) {
        self.SetOverrideWindowPolicy(extractor());
    } else {
        self.SetOverrideWindowPolicy(std::nullopt);
    }
}

} // anonymous namespace

void wrapEngine()
{
    {
        using This = UsdImagingGLEngine;
        
        using Parameters = This::Parameters;
        using PickParams = This::PickParams;
        using IntersectionResult = This::IntersectionResult;

        scope engineScope = class_<This, noncopyable>(
                "Engine", "UsdImaging Renderer class")
            .def( init<>() )
            .def( init<const SdfPath &, const SdfPathVector&,
                    const SdfPathVector& >() )
            .def( init<const Parameters &>() )
            .def("Render", &This::Render)
            .def("SetWindowPolicy", &This::SetWindowPolicy)
            .def("SetRenderViewport", &This::SetRenderViewport)
            .def("SetCameraPath", &This::SetCameraPath)
            .def("SetCameraState", &This::SetCameraState)
            .def("SetLightingState", &_SetLightingState)
            .def("SetSelected", &This::SetSelected)
            .def("ClearSelected", &This::ClearSelected)
            .def("AddSelected", &This::AddSelected)
            .def("SetSelectionColor", &This::SetSelectionColor)
            .def("TestIntersection", &_TestIntersection1)
            .def("TestIntersection",
                 &_TestIntersection2,
                 return_value_policy< TfPySequenceToList >())
            .def("IsConverged", &This::IsConverged)
            .def("GetRendererPlugins", &This::GetRendererPlugins,
                 return_value_policy< TfPySequenceToList >())
            .staticmethod("GetRendererPlugins")
            .def("GetRendererDisplayName", &This::GetRendererDisplayName)
            .staticmethod("GetRendererDisplayName")
            .def("GetRendererHgiDisplayName", &This::GetRendererHgiDisplayName)
            .def("GetCurrentRendererId", &This::GetCurrentRendererId)
            .def("SetRendererPlugin", &This::SetRendererPlugin)
            .def("GetRendererAovs", &This::GetRendererAovs,
                 return_value_policy< TfPySequenceToList >())
            .def("SetRendererAov", &This::SetRendererAov)
            .def("GetRenderStats", &This::GetRenderStats)
            .def("GetRendererSettingsList", &This::GetRendererSettingsList,
                 return_value_policy< TfPySequenceToList >())
            .def("GetRendererSetting", &This::GetRendererSetting)
            .def("SetRendererSetting", &This::SetRendererSetting)
            .def("GetActiveRenderPassPrimPath",
                 &This::GetActiveRenderPassPrimPath)
            .def("GetActiveRenderSettingsPrimPath",
                 &This::GetActiveRenderSettingsPrimPath)
            .def("SetActiveRenderPassPrimPath",
                 &This::SetActiveRenderPassPrimPath)
            .def("SetActiveRenderSettingsPrimPath",
                 &This::SetActiveRenderSettingsPrimPath)
            .def("GetAvailableRenderSettingsPrimPaths",
                 &This::GetAvailableRenderSettingsPrimPaths,
                 return_value_policy< TfPySequenceToList >())
                 .staticmethod("GetAvailableRenderSettingsPrimPaths")
            .def("SetColorCorrectionSettings",
                 &This::SetColorCorrectionSettings)
            .def("IsColorCorrectionCapable",
                 &This::IsColorCorrectionCapable)
            .staticmethod("IsColorCorrectionCapable")
            .def("GetRendererCommandDescriptors",
                &This::GetRendererCommandDescriptors,
                return_value_policy< TfPySequenceToList >() )
            .def("InvokeRendererCommand",
                &This::InvokeRendererCommand,
                (pxr_boost::python::arg("command"),
                 pxr_boost::python::arg("args") = HdCommandArgs()))
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

        class_<Parameters>(
                "Parameters", "Parameters to construct renderer engine")
            .def_readwrite("rootPath", &Parameters::rootPath)
            .def_readwrite("excludedPaths", &Parameters::excludedPaths)
            .def_readwrite("invisedPaths", &Parameters::invisedPaths)
            .def_readwrite("sceneDelegateID", &Parameters::sceneDelegateID)
            .def_readwrite("driver", &Parameters::driver)
            .def_readwrite("rendererPluginId", &Parameters::rendererPluginId)
            .def_readwrite("gpuEnabled", &Parameters::gpuEnabled)
            .def_readwrite("displayUnloadedPrimsWithBounds",
                &Parameters::displayUnloadedPrimsWithBounds)
            .def_readwrite("allowAsynchronousSceneProcessing",
                &Parameters::allowAsynchronousSceneProcessing)
            .def_readwrite("enableUsdDrawModes",
                &Parameters::enableUsdDrawModes)

        ;

        class_<PickParams>(
                "PickParams", "Parameters for TestIntersection")
            .def_readwrite("resolveMode", &PickParams::resolveMode)

        ;

        class_<IntersectionResult>(
                "IntersectionResult", "Results of TestIntersection")
            .def_readwrite("hitPoint", &IntersectionResult::hitPoint)
            .def_readwrite("hitNormal", &IntersectionResult::hitNormal)
            .def_readwrite("hitPrimPath", &IntersectionResult::hitPrimPath)
            .def_readwrite("hitInstancerPath",
                &IntersectionResult::hitInstancerPath)
            .def_readwrite("hitInstanceIndex",
                &IntersectionResult::hitInstanceIndex)
            .add_property("instancerContext",
                +[](const IntersectionResult &result) {
                    return _ToPyList(result.instancerContext); })
        ;
    }

    // Wrap the constants.
    scope().attr("ALL_INSTANCES") = UsdImagingDelegate::ALL_INSTANCES;

    TfPyContainerConversions::from_python_sequence<
        std::vector<GlfSimpleLight>,
        TfPyContainerConversions::variable_capacity_policy>();

    
}
