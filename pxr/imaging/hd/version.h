//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
// 54 -> 55: Introduce API in HdRenderDelegate to aid in transitioning
//           render delegates to a Hydra 2.0 world.
// 55 -> 56: Adds hydra-namespaced internal instancer primvars
// 56 -> 57: Changing SetOverrideWindowPolicy to std::optional on
//           HdRenderPassState, HdxPickFromRenderBufferTaskParams,
//           HdxTaskController and UsdImagingGLEngine.
// 57 -> 58: Introducing hdsi/version.h
// 58 -> 59: HdGeomSubsetsSchema::GetIds() renamed to
//           HdGeomSubsetsSchema::GetGeomSubsetNames().
// 59 -> 60: Introduced HdRenderDelegate::GetCapabilities().
// 60 -> 61: Adding HdPrimvarSchema::GetFlattenedPrimvarValue().
//           Note that in an upcoming change,
//           HdPrimvarSchema::GetPrimvarValue() might change and
//           simply return the data source source at primvarValue.
// 61 -> 62: Remove 'bindingStrength' from HdMaterialBindingSchema.
// 62 -> 63: HdMaterialSchema::GetMaterialNetwork,
//           HdMaterialNetwork::GetNodes, GetTerminals,
//           HdMaterialNode::GetParameters, GetInputConnections
//           return Hydra schemas instead of just container data sources.
//           schemaTypeDefs.h replaces vectorSchemaTypeDefs.h.
// 63 -> 64: Adding disableDepthOfField to HdRenderSettings::RenderProduct
// 64 -> 65: Introduce HdCollectionPredicateLibrary and 
//           HdCollectionExpressionEvaluator for path expression evaluation on
//           scene index prims.
// 65 -> 66: Make HdSchema::_GetTypedDataSource and getters in generated
//           hydra schemas const.
// 66 -> 67: Removes legacy internal instancer primvar names and the
//           TfEnvSetting for using them (see 56).
// 67 -> 68: Adds HdSceneDelegate::SampleFOO with startTime and endTime.
// 68 -> 69: Removes HdGeomSubsetsSchema. Geom subsets are now represented
//           in Hydra as child prims of their parent geometry.
// 69 -> 70: Add dirty bit translation for light filter prims in backend
//           emulation.
// 70 -> 71: Add virtual HdRenderDelegate::IsParallelSyncEnabled.
// 71 -> 72: Add render index API to batch notices sent by the merging scene
//           index.
// 72 -> 73: Adds HdExtComputationUtils::SampleComputedPrimvarValues with
//           startTime and endTime
// 73 -> 74: Extended HdMeshReprDesc to support optional generation of
//           surface edge ids.
// 74 -> 75: Added overload of HdSceneIndexPlugin::_AppendSceneIndex that
//           passes the renderInstanceId to the plugin callback.
// 75 -> 76: Added Scene State ID tunneling through the Hydra pipeline and
//           arbitrary values Setter/Getter to HdRenderParam.
// 76 -> 77: Added GetInputPrimType() to HdFlattenedDataSourceProvider::Context.
//           The Context now stores an HdSceneIndexPrim containing the data
//           source and prim type.
// 77 -> 78: Removed the widget renderTag and added a displayInOverlay boolean
//           attribute to serve the same purpose that widget signified.

#define HD_API_VERSION 78

// 1  ->  2: SimpleLighting -> FallbackLighting
#define HD_SHADER_API 2

#endif // PXR_IMAGING_HD_VERSION_H
