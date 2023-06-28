//
// Copyright 2016 Pixar
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
#ifndef PXR_IMAGING_HD_VERSION_H
#define PXR_IMAGING_HD_VERSION_H

// 18 -> 19: Add support for SceneDelegate surface shaders.
// 19 -> 20: RenderPass constructor takes RenderIndex. RasterState class.
// 20 -> 21: Add HdSceneDelegate::IsEnabled().
// 21 -> 22: split HdRasterState out of HdRenderPass and renamed to HdRenderPassState.
//           HdEngine::Draw API change.
// 22 -> 23: remove ID render API
// 23 -> 24: GetPathForInstanceIndex returns absolute instance index.
// 24 -> 25: move simpleLightingShader to Hdx.
// 25 -> 26: move camera and light to Hdx.
// 26 -> 27: move drawTarget to Hdx.
// 27 -> 28: switch render index Sprim to take a typeId.
// 28 -> 29: cameras only support matrices.
// 29 -> 30: added IDRenderColor decode and direct Rprim path fetching.
// 30 -> 31: added pre-chained buffer sources
// 31 -> 32: renamed HdShader{Param} to HdMaterial{Param}
// 32 -> 33: Deleted GetPathForInstanceIndex; added GetScenePrimPath.
// 32 -> 34: Added HdInstancerContext to GetScenePrimPath.
// 34 -> 35: HdRepr is using std::unique_ptr<HdDrawItem>
// 35 -> 36: InsertRprim/InsertInstancer no longer take instancerId,
//           HdSceneDelegate now has GetInstancerId
// 36 -> 37: Renamed HdRprim::_SetMaterialId to SetMaterialId. It no longer
//           takes changeTracker.
// 37 -> 38: Removed Bprim garbage collection API from HdChangeTracker and
//           HdResourceRegistry.
// 38 -> 39: Removed garbage collection API from HdChangeTracker and
//           HdResourceRegistry.
//           Added HdSceneDelegate::GetInstancerPrototypes.
// 39 -> 40: Removed Bind and Unbind API from HdRenderPassState.
// 40 -> 41: Renamed HdDelegate::GetMaterialNeworkselector() to 
//           GetMaterialRenderContexts(). It now returns a TfTokenVector.
// 41 -> 42: Removed GetMaterialTag() from HdRenderIndex.
// 42 -> 43: Removed HdCamera pulling on view and projection matrix.
// 43 -> 44: Replaced HdCamera::GetProjectionMatrix with
//           HdComputeProjectionMatrix.
// 44 -> 45: Added HdSceneDelegate::GetScenePrimPaths.
// 45 -> 46: New signatures for HdRendererPlugin::IsSupported and
//           HdRendererPluginRegistry::GetDefaultPluginId
// 46 -> 47: Adding HdRenderDelegate::GetRenderSettingsNamespaces()
// 47 -> 48: New signature for HdRenderIndex::InsertSceneIndex: added optional 
//           argument needsPrefixing
// 48 -> 49: Moved HdExtCompCpuComputation, Hd_ExtCompInputSource,
//           Hd_CompExtCompInputSource, and Hd_SceneExtCompInputSource to hdSt.
// 49 -> 50: Added HdModelDrawMode struct and getter API to HdSceneDelegate.
// 50 -> 51: HdMaterialBindingSchema became HdMaterialBindingsSchema which uses
//           the new HdMaterialBindingSchema.
// 51 -> 52: Added lens distortion, focus, and split diopter parameters to
//           HdCamera.
// 52 -> 53: Changing dirty bits of HdCoordSys.
// 53 -> 54: Introducing HdFlattenedDataSourceProvider to make
//           HdFlatteningSceneIndex modular.
#define HD_API_VERSION 53

// 1  ->  2: SimpleLighting -> FallbackLighting
#define HD_SHADER_API 2

#endif // PXR_IMAGING_HD_VERSION_H
