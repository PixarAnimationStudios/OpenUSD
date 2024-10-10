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
_TestIntersection(
    UsdImagingGLEngine & self, 
    const GfMatrix4d &viewMatrix,
    const GfMatrix4d &projectionMatrix,
    const UsdPrim& root, 
    UsdImagingGLRenderParams params)
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
        using Parameters = UsdImagingGLEngine::Parameters;

        scope engineScope = class_<UsdImagingGLEngine, noncopyable>(
                "Engine", "UsdImaging Renderer class")
            .def( init<>() )
            .def( init<const SdfPath &, const SdfPathVector&,
                    const SdfPathVector& >() )
            .def( init<const Parameters &>() )
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
            .def("TestIntersection", &_TestIntersection)
            .def("IsConverged", &UsdImagingGLEngine::IsConverged)
            .def("GetRendererPlugins", &UsdImagingGLEngine::GetRendererPlugins,
                 return_value_policy< TfPySequenceToList >())
                .staticmethod("GetRendererPlugins")
            .def("GetRendererDisplayName", 
                    &UsdImagingGLEngine::GetRendererDisplayName)
                .staticmethod("GetRendererDisplayName")
            .def("GetCurrentRendererId", 
                    &UsdImagingGLEngine::GetCurrentRendererId)
            .def("SetRendererPlugin", 
                    &UsdImagingGLEngine::SetRendererPlugin)
            .def("GetRendererAovs", 
                    &UsdImagingGLEngine::GetRendererAovs,
                 return_value_policy< TfPySequenceToList >())
            .def("SetRendererAov", 
                    &UsdImagingGLEngine::SetRendererAov)
            .def("GetRenderStats", 
                    &UsdImagingGLEngine::GetRenderStats)
            .def("GetRendererSettingsList", 
                    &UsdImagingGLEngine::GetRendererSettingsList,
                 return_value_policy< TfPySequenceToList >())
            .def("GetRendererSetting", &UsdImagingGLEngine::GetRendererSetting)
            .def("SetRendererSetting", &UsdImagingGLEngine::SetRendererSetting)
            .def("SetActiveRenderPassPrimPath",
                 &UsdImagingGLEngine::SetActiveRenderPassPrimPath)
            .def("SetActiveRenderSettingsPrimPath",
                 &UsdImagingGLEngine::SetActiveRenderSettingsPrimPath)
            .def("GetAvailableRenderSettingsPrimPaths",
                 &UsdImagingGLEngine::GetAvailableRenderSettingsPrimPaths,
                 return_value_policy< TfPySequenceToList >())
                 .staticmethod("GetAvailableRenderSettingsPrimPaths")
            .def("SetColorCorrectionSettings", 
                    &UsdImagingGLEngine::SetColorCorrectionSettings)
            .def("IsColorCorrectionCapable", 
                &UsdImagingGLEngine::IsColorCorrectionCapable)
                .staticmethod("IsColorCorrectionCapable")
            .def("GetRendererCommandDescriptors",
                &UsdImagingGLEngine::GetRendererCommandDescriptors,
                return_value_policy< TfPySequenceToList >() )
            .def("InvokeRendererCommand",
                &UsdImagingGLEngine::InvokeRendererCommand,
                (pxr_boost::python::arg("command"),
                 pxr_boost::python::arg("args") = HdCommandArgs()))
            .def("IsPauseRendererSupported", 
                &UsdImagingGLEngine::IsPauseRendererSupported)
            .def("PauseRenderer", &UsdImagingGLEngine::PauseRenderer)
            .def("ResumeRenderer", &UsdImagingGLEngine::ResumeRenderer)
            .def("IsStopRendererSupported", 
                &UsdImagingGLEngine::IsStopRendererSupported)
            .def("StopRenderer", &UsdImagingGLEngine::StopRenderer)
            .def("RestartRenderer", &UsdImagingGLEngine::RestartRenderer)
            .def("SetRenderBufferSize", &UsdImagingGLEngine::SetRenderBufferSize)
            .def("SetFraming", &UsdImagingGLEngine::SetFraming)
            .def("SetOverrideWindowPolicy", _SetOverrideWindowPolicy)
            .def("PollForAsynchronousUpdates",
                &UsdImagingGLEngine::PollForAsynchronousUpdates)
            
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
            
        ;
    }

    // Wrap the constants.
    scope().attr("ALL_INSTANCES") = UsdImagingDelegate::ALL_INSTANCES;

    TfPyContainerConversions::from_python_sequence<
        std::vector<GlfSimpleLight>, 
        TfPyContainerConversions::variable_capacity_policy>();
}
