# Change Log

## [21.02] - 2021-01-18

### Build
- Added build and packaging scripts for PyPI packages. Thanks to Nvidia for
  their work on this project!
- Updated documentation build to fix several issues and no longer require 
  Python. (Issue: [#718](https://www.github.com/PixarAnimationStudios/USD/issues/718))
- GLEW is no longer required for building imaging components.

- Various fixes and changes to build_usd.py:
  - Fixed command-line args not being respected for OpenVDB. 
    (PR: [#1406](https://www.github.com/PixarAnimationStudios/USD/pull/1406))
  - Updated boost to 1.70 on macOS for both Python 2 and 3. 
    (Issue: [#1369](https://www.github.com/PixarAnimationStudios/USD/issues/1369))
  - Updated MaterialX to 1.37.3 with shared libraries on Linux.

- Fixed various linking issues with OpenEXR. 
  (PR: [#1398](https://www.github.com/PixarAnimationStudios/USD/pull/1398))

### USD
- Allow setting malloc hook functions if they were previously set to malloc/etc.
- Fixed handling of symbolic links and mount points on Windows. 
  (PR: [#1378](https://www.github.com/PixarAnimationStudios/USD/pull/1378))
- Fixed incorrect handling of non-existent variables in TfEnvSetting in Python.
- Updated GfRect2i API to use function and argument names that are agnostic
  to the direction of the Y axis.
- Updated ilmbase half embedded in Gf to OpenEXR v2.5.3. 
  (Issue: [#1354](https://www.github.com/PixarAnimationStudios/USD/issues/1354))
- Added VtArray::AsConst, cfront, and cback methods to help avoid inadvertent
  copy-on-writes and thread-safety issues.

- Fixes for variety of issues with VtArray conversions in Python, including
  conversion from Python sequences if all elements are convertible and a
  bug with inadvertent copy-on-writes. 
  (Issue: [#1138](https://www.github.com/PixarAnimationStudios/USD/issues/1138))

- Initial implementation of Ar 2.0. This includes new features like support
  for URI resolvers and many changes to the ArResolver interface. For more
  details see https://graphics.pixar.com/usd/docs/668045551.html.

  Work on Ar 2.0 is not yet complete and will continue through the next few
  releases. Ar 2.0 is disabled by default but can be enabled for preview and
  initial testing by specifying `PXR_USE_AR_2=ON` when running CMake.

- Moved SdfFindOrOpenRelativeToLayer to SdfLayer::FindOrOpenRelativeToLayer.
- Fixed SdfLayer::FindRelativeToLayer to use the same anchoring logic as
  SdfLayer::FindOrOpenRelativeToLayer.
- Fixed string encoding issue in .usda file writer. 
  (Issue: [#1331](https://www.github.com/PixarAnimationStudios/USD/issues/1331))
- Improved behavior when hitting hard-coded composition graph limits in Pcp.
- Fixed incorrect native instancing behavior with sub-root references.

- Added support for auto-apply API schemas. This allows single-apply API
  schemas to be automatically applied to prims using one of a list of
  associated concrete schema types instead of requiring the user to manually
  apply the API schema.

- Renamed UsdSchemaType to UsdSchemaKind to disambiguate between the schema
  type (e.g. UsdGeomSphere) and kind (e.g. non-applied, single-apply, etc).
- Deprecated functions using the "schema type" terminology in favor of
  "schema kind".
- Added UsdVariantSet::BlockVariantSelection. 
  (Issue: [#1319](https://www.github.com/PixarAnimationStudios/USD/issues/1319), 
  PR: [#1340](https://www.github.com/PixarAnimationStudios/USD/pull/1340))
- Removed deprecated UsdAttribute::BlockConnections.
- Removed deprecated UsdRelationship::BlockTargets.
- Removed deprecated UsdCollectionAPI::ApplyCollection.
- Added "container" concept to UsdShadeConnectableAPI.
- Added support for connecting multiple sources to UsdShadeConnectableAPI.
- Deprecated API for connecting to single sources in favor of the more
  general multiple-source API on UsdShadeConnectableAPI.

- Deprecated UsdConnectableAPI::IsShader and IsNodeGraph in favor of
  IsContainer API. Warnings are emitted on first use unless the environment
  setting `USD_SHADE_EMIT_CONNECTABLE_API_DEPRECATION_WARNING` is set to 0.

- Updated various clients to apply the UsdShadeMaterialBindingAPI schema
  before binding materials.
- Fixed Python binding for UsdShade.CoordSysAPI.HasLocalBindings. 
  (PR: [#1360](https://www.github.com/PixarAnimationStudios/USD/pull/1360))
- Fixed Python binding for UsdSkel.SkinningQuery.ComputeExtentsPadding.
  (Issue: [#1375](https://www.github.com/PixarAnimationStudios/USD/issues/1375))
- Added UsdLuxPluginLight and UsdLuxPluginLightFilter schemas that allow for
  defining a light or light filter via an SdrShaderNode.
- UsdLuxLight and UsdLuxLightFilter schemas now publish an associated
  SdrShaderNode based on their built-in properties.
- Input attributes for UsdLuxLight and UsdLuxLightFilter schemas are now
  connectable and have been renamed to include the "inputs:" prefix.
- Deprecated UsdLuxLightPortal schema in favor of new UsdLuxPortalLight schema.

### Imaging
- Added new GL Loading Library (GLApi) to replace GLEW. GLEW is no longer 
  required.
- Added HioImage, removed GlfImage.
- Added new camera framing API. Introduces the display and data window and 
  storage size to separate these concepts. Updated Storm, HdPrman, and HdEmbree.
- Added support for normal buffers to HdxPickFromRenderBufferTask.
- Added standard prim API to HdInstancer and HdSceneDelegate::GetInstancerId.
- Added support for animated extents when using draw modes. 
  (PR: [#1365](https://www.github.com/PixarAnimationStudios/USD/pull/1365))
- Improved Hydra camera to better support physically based attributes.
- Extended HdDisplayStyle to house more advanced selection behaviors.
- Changed renderParams timeCode default values from Default to EarliestValue.
- Changed UsdImagingDelegate to map HdLightTokens to the new input attribute 
  names in queries through GetLightParamValue.
- Merged tokens textureResourceMemory into textureMemory for better performance 
  tracking.
- Renamed UsdImagingValueCache to UsdImagingPrimvarDescCache, it only stores 
  primvar descriptors.
- Removed implementations of deprecated HdSceneDelegate::GetTextureResource.
- Removed HwFieldReader volume material node in favor of typed nodes.
- Fixed bug in pick targets when resolving unique hits. 
  (Issue: [#1343](https://www.github.com/PixarAnimationStudios/USD/issues/1343))
- Fixed UsdSkel instance drawing at origin bug. 
  (Issue: [#1347](https://www.github.com/PixarAnimationStudios/USD/issues/1347))
- Fixed UsdImaging to discard coord sys bindings to non-existent xforms. 
  (Issue: [#1346](https://www.github.com/PixarAnimationStudios/USD/issues/1346))
- Fixed UsdSkelImagingSkeletonAdapter to forward GetMaterialId and 
  GetDoubleSided calls to skinned mesh primadapter. 
  (Issue: [#1384](https://www.github.com/PixarAnimationStudios/USD/issues/1384))
- Fixed an issue where UsdSkelImagingSkeletonAdapter::_RemovePrim() failed to 
  remove skeletons that did not have any bindings to skinned prims. 
  (Issue: [#1228](https://www.github.com/PixarAnimationStudios/USD/issues/1228), 
   Issue: [#1248](https://www.github.com/PixarAnimationStudios/USD/issues/1248))
- Fixed some display crashes after resyncing skeletons. 
  (PR: [#1397](https://www.github.com/PixarAnimationStudios/USD/pull/1397))
- Fixed dome light preview surface. 
  (PR: [#1392](https://www.github.com/PixarAnimationStudios/USD/pull/1392))
- Fixed links to OpenEXR for Alembic and OpenImageIO plugins. 
  (PR: [#1398](https://www.github.com/PixarAnimationStudios/USD/pull/1398))

### Storm
- Added support for varying interpolation of any basis curves primvar. 
  (Issue: [#1308](https://www.github.com/PixarAnimationStudios/USD/issues/1308))
- Added support for HdStResourceRegistry::ReloadResource to allow clients to 
  explicitly force textures to be reloaded by file path. 
  (Issue: [#1352](https://www.github.com/PixarAnimationStudios/USD/issues/1352))
- Added a new HdMaterialNetwork2 to combine (and then replace) 
  HdStMaterialNetwork and MatFiltNetwork.
- Added step to convert 3 channel textures to 4 channel to support Hgi backends 
  with 4 channel requirements.
- Added Resize() to barContainer.
- Added fieldTextureMemory render setting to specify the target memory of 
  volume textures.
- Added experimental MaterialXFilter to process a material network and convert 
  it into a MaterialX network. Also, added a first pass on a MaterialX 
  shadergen.
- Enabled bindless textures by default.
- Several improvements to volume rendering including a step size relative to the 
  sample distance so that the quality of the rendered volume does not depend on 
  the scale.
- Optimized HdStRenderBuffer performance by only allocating when the descriptor 
  changes.
- Optimized computations by avoiding copying the compute kernel for GPU 
  computations.
- Refactored face culling to remove fragment shader discards when possible.
- Refactored geometric shaders such that only prims with the "masked" material 
  tag can use alpha threshold based discards.
- Switched Storm over to using the "translucent" material tag rather than 
  "additive" as the default translucency state.
- Fixed handling of valid to invalid BAR transitions due to Scene graph 
  operations on primvars or primvar filtering. 
  (Issue: [#1182](https://www.github.com/PixarAnimationStudios/USD/issues/1182))
- Fixed GPU memory leak of certain buffers never getting garbage collected.
- Fixed fullscreen pass to preserve alpha.
- Removed instance primvar filtering.
- Various improvements to Hgi :
  - Added initial version of the Hgi codegen to be able to produce 
    GLSL/MetalSL/others from glslfx.
  - Added the first push of our experimental, incomplete, HgiVulkan backend, 
    which does not yet build by default.
  - Added memory barriers to Hgi.
  - Added CopyTextureToBuffer and CopyBufferToTexture to HgiBlitCmds.
  - Added 2D_ARRAY support to Hgi.
  - Moved Ptex and Udim loading from Glf to Hgi and Storm.
  - Added UINT16 format as prep work for Ptex support.
  - Fixed obj-c autorelease issue, and simplification of secondary command 
    buffer in HgiMetal compute encoder.
  - Fixed HgiTexture::GetByteSizeOfResource to take the mip levels into account.
  - Fixed for missing include. 
    (PR: [#1359](https://www.github.com/PixarAnimationStudios/USD/pull/1359))

### usdview
- Fixed StageView.pickObject when doubles are small or out of image-bounds. 
  (PR: [#1296](https://www.github.com/PixarAnimationStudios/USD/pull/1296))

### MaterialX Plugin
- Removed deprecated support for MaterialX 1.36.

### Embree Hydra Plugin
- Added support for new camera framing API and HdCamera API.

### RenderMan Hydra Plugin
- Bumped version requirement to RenderMan 23.5 or greater.
- Improved calculation of vertex, varying, and face-varying primvar counts in 
  BasisCurves to better match RenderMan.
- Added support for new camera framing API and HdCamera API.
- Added support for material node int array inputs. 
  (Issue: [#1294](https://www.github.com/PixarAnimationStudios/USD/issues/1294))
- Fixed issue that caused parameters from a shader to be incorrectly carried 
  over to the next shader that was parsed. 
  (Issue: [#1396](https://www.github.com/PixarAnimationStudios/USD/issues/1396))

## [20.11] - 2020-10-14

### Build
- Updated CMakeLists.txt to conform to recommended practice. 
  (Issue: [#1241](https://www.github.com/PixarAnimationStudios/USD/issues/1241))
- The build system now sets `Boost_NO_BOOST_CMAKE=ON` by default to
  work around issues with boost's cmake files. 
  (Issue: [#1255](https://www.github.com/PixarAnimationStudios/USD/issues/1255))

- Various fixes and changes to build_usd.py:
  - Added `--toolset` option for specifying CMake toolset. 
    (PR: [#1325](https://www.github.com/PixarAnimationStudios/USD/pull/1325))
  - Added `--prefer-safety-over-speed` and `--prefer-speed-over-safety`
    options. See below for more details.
  - Fixed issue with building libTIFF with Xcode 12. 
    (PR: [#1315](https://www.github.com/PixarAnimationStudios/USD/pull/1315))

### USD
- Fixed incorrect underlying container type for TfHashMultiMap. 
  (PR: [#1281](https://www.github.com/PixarAnimationStudios/USD/pull/1281))
- Fixed invalid conversions of Gf containers like GfVec3f in Python. Note this
  issue is still present in Python 3. 
  (Issue: [#1290](https://www.github.com/PixarAnimationStudios/USD/issues/1290))
- Fixed various tests to accommodate `PXR_OVERRIDE_PLUGINPATH_NAME`. 
(PR: [#1275](https://www.github.com/PixarAnimationStudios/USD/pull/1275))

- Added GetExternalAssetDependencies to SdfLayer and SdfFileFormat to allow
  file format plugins to declare additional dependencies that determine when
  a layer produced from that file format should be reloaded.

- Reduced memory usage from SdfPath::GetString and added SdfPath::GetAsString
  and SdfPath::GetAsToken to avoid populating internal caches. 
  (Issue: [#1287](https://www.github.com/PixarAnimationStudios/USD/issues/1287))

- Various fixes to composition behavior:
  - Fixed issues with sub-root references, payloads, and inherits, and
    ancestral variant selections.
  - Fixed issue with payloads on ancestors of sub-root references/payloads.
  - Fixed issue with combinations of root, sub-root, and ancestral inherits.

- Fixed bug in .usda file writer that could cause data corruption. 
  (Issue: [#1331](https://www.github.com/PixarAnimationStudios/USD/issues/1331))
- Fixed performance issue when writing many relationship targets or attribute
  connections to .usdc files. 
  (Issue: [#1345](https://www.github.com/PixarAnimationStudios/USD/issues/1345))

- Improved performance for certain queries with .usdc files. In one example,
  this decreased draw times in usdview after transform changes by ~90%.
  (Issue: [#1300](https://www.github.com/PixarAnimationStudios/USD/issues/1300))

- New .usdc files now default to version 0.8.0. These files cannot be read in
  USD releases prior to v19.03. Users can revert to writing older versions by
  setting the environment variable `USD_WRITE_NEW_USDC_FILES_AS_VERSION` to an
  older version.

- Added safety checks to guard against reading malformed .usdc files. These
  checks may negatively impact performance and may be disabled by specifying
  `PXR_PREFER_SAFETY_OVER_SPEED=OFF` when running CMake or
  `--prefer-speed-over-safety` with build_usd.py.

- Native instancing "master" prims are now referred to as "prototype" prims.
  The generated paths of prototype prims has been changed from /__Master_<X>
  to /__Prototype_<X>. Clients can use new UsdPrim::IsPrototypePath and
  UsdPrim::IsPathInPrototype API to determine if a given path is related
  to a prototype.

  UsdStage and UsdPrim API using "master" terminology like UsdPrim::GetMaster
  and UsdStage::GetMasters have been deprecated and will be removed in a future
  release.

- Added Get/Set/ClearChildrenReorder to UsdPrim for reordering prim children.
- UsdProperty::FlattenTo now allows flattening properties across stages.
- Deprecated UsdRelationship::BlockTargets and UsdAttribute::BlockConnections.
  Clients should use SetTargets({}) and SetConnections({}) instead.
- Changed UsdRelationship::GetTargets and UsdAttribute::GetConnections to return
  false when HasAuthored would return false.
- Fixed bug where UsdFlattenLayerStack was not applying the asset path
  resolution callback to asset paths in `clips` metadata. 
  (PR: [#1266](https://www.github.com/PixarAnimationStudios/USD/pull/1266))
- Added UsdNotice::LayerMutingChanged notice. 
  (Issue: [#676](https://www.github.com/PixarAnimationStudios/USD/issues/676))
- Fixed resource leak with Usd.PrimCompositionQuery in Python. 
  (PR: [#1297](https://www.github.com/PixarAnimationStudios/USD/pull/1297))
- Fixed crashes when printing expired Sdf.Layer and Usd.Stage objects in
  Python. (PR: [#1012](https://www.github.com/PixarAnimationStudios/USD/pull/1012))
- Removed `isPrivateApply` functionality from schema generation.
- Fixed error in UsdUtilsComputeAllDependencies when an invalid
  templateAssetPath value is specified. 
  (PR: [#1289](https://www.github.com/PixarAnimationStudios/USD/pull/1289))
- Improved UsdUtilsGetPrimaryUVSetName and UsdUtilsGetPrefName. 
  (PR: [#1283](https://www.github.com/PixarAnimationStudios/USD/pull/1283))
- Fixed bug in UsdUtilsStitchClips that led to inconsistent stitched results.

- Added CanContainPropertyName to UsdCollectionAPI, UsdGeomPrimvars,
  UsdShadeMaterialBindingAPI, and UsdShadeCoordSysAPI for checking if a
  property name is valid for these schemas.

- Deprecated UsdCollectionAPI::ApplyCollection in favor of Apply.
- Added fallback values for texture card attributes on UsdGeomModelAPI schema.
- Added exposure control to UsdGeomCamera.
- Added ease-of-use API to UsdGeom primitives for querying the size of primary
  geometric properties (e.g. UsdGeomMesh::GetFaceCount).
- Fixed plugin extent computations for primitives in UsdGeom to respect
  given timecode parameter. 
  (PR: [#1284](https://www.github.com/PixarAnimationStudios/USD/pull/1284))
- Fixed crash in UsdGeomSubset::GetUnassignedIndices when a given subset had
  negative indices. 
  (Issue: [#1227](https://www.github.com/PixarAnimationStudios/USD/issues/1227))
- Added UsdShadeNodeDefAPI schema to represent connectable nodes in a graph.
  UsdShadeShader has been modified to build off of this schema.
- Added UsdShadeConnectableAPIBehavior to allow plugins to customize
  connectability by prim type.
- Added API to retrieve material outputs for standard terminals in all
  render contexts on UsdShadeMaterial.
- Added more instancing support to UsdSkel. 
  (PR: [#1258](https://www.github.com/PixarAnimationStudios/USD/pull/1258))
- Updated UsdVol FieldAsset schemas.
- Various documentation fixes. 
  (Issue: [#1162](https://www.github.com/PixarAnimationStudios/USD/issues/1162), 
  PR: [#1335](https://www.github.com/PixarAnimationStudios/USD/pull/1335))

### Imaging
- Added new "sourceColorSpace" input to UsdUVTexture with values "raw", "sRGB",
  and "auto" for better control of with which color space a texture is read.
- Added ability to use mip maps authored in a file in the new texture system in
  Storm.
- Added UsdImagingPrimAdapter::ShouldIgnoreNativeInstanceSubtrees() to allow
  adapters to disable instancing of itself and its descendants.
- Added pre-multiply alpha functionality for UDIM textures. Also added sRGB
  internal formats for UDIMs.
- Added checks in HioGlslfxConfig to make sure the default value for an
  attribute and its type match.
- Added HdEngine::ClearTaskContextData() to avoid the task context to hold on to
  resources that are about to become invalid as the render delegate is destroyed.
- Added HDST_DUMP_FAILING_SHADER_SOURCEFILE debug flag to facilitate shader
  debugging.
- Added texture filepath resolution for symlinks.
- Added an "outHitNormal" parameter to UsdImagingGLEngine::TestIntersection().
- Added ReloadResource in Storm which reloads shader files when they change.

- Improvements to Hgi:
  - Added Draw, DrawIndirect and DrawIndexedIndirect to Hgi and backends.
  - Added tracking of HgiCmds submission to ensure they only get submitted once.
  - Added TextureCpuToGpu copy on BlitCmd, and CopyBufferGpuToCpu to read back
    the GPU frustum cull results.
  - Added guarantees that HgiGLInterop will restore GL state after interop.
  - Added guarantees that Hgi GPU-GPU sync between SubmitCmds calls.
  - Added TextureView to Hgi and convert domelight to use Hgi compute and
    texture views.
  - Added staging buffer in Hgi to reduce the amount of times small amounts of
    data are copied into the same GPU buffer.
  - Added HgiPrimitiveType, and added HgiSampleCount on HgiPipelineDesc.
  - Added support for BC texture compression.
  - Added component mapping to HgiTextureDesc and implementations for GL and
    Metal.
  - Added ability to share HgiCmds objects in Storm.
  - Added optional wait behavior for command submission in Hgi.

- Changed GLenum usage in GlfBaseTextureData and others to use new HioFormat.
- Changed Storm's frustum culling to be done via a vertex shader without a
  fragment shader.
- Changed HdxShadowTask to specify that it should only render opaque and masked
  materials.
- Changed cards UVs from face-varying to vertex interpolated.
- Changed to GL_SAMPLE_ALPHA_TO_ONE when using alpha to coverage, the alpha
  computed after the framebuffer resolve is more meaningful.
- Changed texture cards to require prims with UsdGeomModelAPI schema applied.
- Changed type of UsdPrimvarReader.varname input from token to string.
- Renamed various HdSt*GL classes to just HdSt*, this is part of the transition
  of Storm to support multiple rendering APIs.
- Enabled bindless shadow maps by default in Storm.
- On first material sync in Storm, now we try to batch prims with identical
  textures together.
- HdStDrawTarget's preferred mechanism are AOVs not GlfDrawTarget's. This change
  allows us to remove all GlfDrawTarget support from HdStDrawTarget.
- Since HdStDrawTarget's no longer rely on the resolve task, removing code to
  communicate the draw target task render passes to the resolve task.
- Deprecated HdSceneDelegate::GetTextureResource and will eventually be removed.
- An application can now disable the PresentTask in hdx task controller.
- When imaging basis curves, treat empty normals array the same as missing
  normals (i.e. draw as tube and not ribbon at higher complexities).
- Store HgiResourceBindings and HgiPipeline objects on HdSt resource registry.
- Switched (most of) the GPU frustum culling code from raw GL to Hgi.
- With the addition of HgiMetal, all platforms we currently support (for Storm)
  support color correction and 16F targets.
- Use BufferResource for GPU frustum cull result buffer so we can use the
  regular buffer binding APIs.
- Load mipmap data in HgiMetal textures.
- Changed signature of HgiDataSizeOfFormat so that it can return blockWidth and
  blockHeight. Returning block size in bytes for compressed formats.
- Describe shader constants (push constants) in pipeline.
- Using HgiCompute in smooth normals computation, switched flat normals and
  quadrangulate to HgiCompute, and converted HdStExtCompGpuComputation to Hgi.

- Removed UsdImaging value cache API for all data except primvar descriptors and
  instance indices.
- Removed unused hitMode field from HdxPickTaskContextParams.
- Removed UsdImagingGLEngine::InvalidateBuffers().
- Removed pre-multiply alpha behavior from volume.glslfx and changed
  renderPass.glslfx to allow fully transparent but still emissive volumes.
- Removed pre-multiply alpha behavior from stb_image and make it optional via
  SubtextureIdentifier within the HdSt texture system.
- Removed support of old-style draw targets using GlfDrawTarget instead of AOVs
  and render buffers.
- Removed UsdUvTexture's rgba output, and updated the cards draw mode to now use
  "rgb" and "a" outputs rather than rgba.
- Removed ReloadAllShaders from HdEngine.
- Removed HD_DRAWITEM_DRAWN as it uses raw gl calls that map/unmap a buffer.
- Removed HgiPipelineType. We split HgiPipeline into HgiComputePipeline and
  HgiGraphicsPipeline a little bit ago.
- Removed resource bindings handle from pipeline.
- Removed smooth normals CPU fallback path. Since previous releases required
  OpenGL 4.5 for Hgi to work, OpenGL compute must always be available.
- Removed HdBinding::TBO.
- Removed the implicit barriers from Hgi and let the external client manage
  synchronization via SubmitCmds.

- Fixed potential dead-lock caused by parallel loading plugins.
- Fixed lights to be turned off when invisible in UsdImaging.
- Fixed issue with UsdImagingGLEngine::TestIntersection such that it now
  correctly populates. Callers of this function are reminded to provide the
  "frame" field in UsdImagingGLRenderParams for correct and performant results.
- Fixed handling of computed primvar sources for meshes.
- Fixed normals after complexity change.
- Fixed HgiGL garbage collector leak when Hgi is recreated during process.
- Fixed HgiGL to better protect against long labels when using glObjectLabel.
- Fixed render settings behaviors so we test whether or not we're really
  changing anything before incrementing the settings version.
- Fixed for cycle detection in material network processing.
- Fixed Glf lighting resource binding.
- Fixed USD edit dependency tracking for cards prims.

- Updated UsdPreviewSurface clearcoat calculations to better match expected
  inputs for a specular lobe in Storm, and added clearcoat component to indirect
  lighting calculation in Storm. 
  (Issue: [#1307](https://www.github.com/PixarAnimationStudios/USD/issues/1307))
- Added support for UsdPreviewSurface's opacityThreshold parameter in
  Storm. (Issue: [#990](https://www.github.com/PixarAnimationStudios/USD/issues/990))
- Added support for normal mapping for UsdPreviewSurfaces in Storm. 
  (Issue: [#701](https://www.github.com/PixarAnimationStudios/USD/issues/701))
- Metallic materials should have an F0 equal to their base color in
  UsdPreviewSurface in Storm. 
  (Issue: [#1174](https://www.github.com/PixarAnimationStudios/USD/issues/1174))

- Removed pre-multiplication behavior from OIT resolve shader. The preview
  surface (and other shaders) pre-multiply within the fragment shader (unless
  the "diffuseColor" and "opacity" params are connected to the same
  texture). 
  (Issue: [#1269](https://www.github.com/PixarAnimationStudios/USD/issues/1269))

- Added support for UDIM texture scale and bias in Storm. 
  (Issue: [#1129](https://www.github.com/PixarAnimationStudios/USD/issues/1129))
- Converted render index to usdImaging cachePath before calling the adapter
  InvokeComputation method. 
  (Issue: [#1333](https://www.github.com/PixarAnimationStudios/USD/issues/1333))
- Fix draw mode adapter handling of transforms for instanced card prims. 
  (Issue: [#1251](https://www.github.com/PixarAnimationStudios/USD/issues/1251))
- _IsVarying no longer clears dirty bit and now ProcessPropertyChange is filled
  out to provide more efficiency. 
  (Issue: [#1250](https://www.github.com/PixarAnimationStudios/USD/issues/1250))

### usdview
- Added 'Enable Scene Lights' option.
- Changed UI to use bundled Roboto and Roboto Mono fonts.
- Fixed build issues due to stricter checks in PySide2 5.15.1. 
  (PR: [#1320](https://www.github.com/PixarAnimationStudios/USD/pull/1320))

### Alembic Plugin
- Alembic uv's are now converted to texCoord2f[] primvars:st. This can
  be disabled by setting the `USD_ABC_WRITE_UV_AS_ST_TEXCOORD2FARRAY`
  environment variable to 0.

### RenderMan Plugin
- Fixed infinite loop in material processing.
- Initial implementation of exposure.
- Volumes can react to changes to a field prim. 
- Fixed UsdUVTexture wrap mode support. 
- Add support for UsdPreviewSurface's opacityThreshold param in HdPrman. 
  (Issue: [#990](https://www.github.com/PixarAnimationStudios/USD/issues/990))
- Updated UsdPreviewSurface clearcoat calculations to better match expected
  inputs for a specular lobe in HdPrman. 
  (Issue: [#1307](https://www.github.com/PixarAnimationStudios/USD/issues/1307))
- Metallic materials should have an F0 equal to their base color in
  UsdPreviewSurface in HdPrman. 
  (Issue: [#1174](https://www.github.com/PixarAnimationStudios/USD/issues/1174))

## [20.08] - 2020-07-21

### Build
- The "master" branch on GitHub has been renamed "release".
- Improved error handling when building the RenderMan plugin. 
  (Issue: [#1054](https://www.github.com/PixarAnimationStudios/USD/issues/1054))

- Various fixes and changes to build_usd.py:
  - Added `--tools` and `--no-tools` options.
  - Updated OpenImageIO (2.1.16.0) and MaterialX (1.37.1) dependencies.
  - Specifying `--embree` will now build the Embree library. The
    `--embree-location` parameter has been removed.
  - CMake 3.14 is now required on Windows to support boost 1.70+.
  - Python 3.8 on Windows now causes an error, as USD does not support
    this version on Windows.
  - Improved handling of boost build failures.
  - Fixed locale decoding errors. 
    (Issue: [#1165](https://www.github.com/PixarAnimationStudios/USD/issues/1165))
  - Fixed incorrect detection of Draco library. 
    (PR: [#1239](https://www.github.com/PixarAnimationStudios/USD/pull/1239))

### USD
- Added support for "future division" in Python 2 to aid transition to Python 3.

- Added ArResolver::CreatePathForLayer to allow users to customize behavior
  when writing a layer. SdfLayer now uses this method instead of creating a
  directory, which was not appropriate for non-filesystem uses. 
  (Issue: [#1148](https://www.github.com/PixarAnimationStudios/USD/issues/1148))

- Added SdfReference::IsInternal. 
  (PR: [#1204](https://www.github.com/PixarAnimationStudios/USD/pull/1204))
- Added fallback prim types feature, allowing clients to specify alternative
  schemas to use if a schema can't be found. See documentation for more details.
- Added UsdPrim::ApplyAPI, RemoveAPI, AddAppliedSchemas,
  and RemoveAppliedSchemas. 
  (Issue: [#1218](https://www.github.com/PixarAnimationStudios/USD/issues/1218))
- Added Python __repr__ for generated schemas.
- Added methods to UsdGeomPrimvar for processing property names.
- Added methods to UsdGeomPrimvarsAPI for creating, removing, and blocking
  primvars. 
  (Issue: [#1100](https://www.github.com/PixarAnimationStudios/USD/issues/1100))
- Improved documentation for UsdVol schema domain. 
  (PR: [#1203](https://www.github.com/PixarAnimationStudios/USD/pull/1203))
- Improved documentation for UsdGeomMesh. 
  (PR: [#1254](https://www.github.com/PixarAnimationStudios/USD/pull/1254))
- Improved diagnostic messages for plugin registration and loading.

- Plugin search paths in PXR_PLUGINPATH_NAME can now be a symlink. Paths are
  now processed in order to ensure plugins in earlier entries take priority
  over those in later entries.

- Replaced SdrShaderNode::GetSourceURI and GetResolvedSourceURI with
  GetResolvedDefinitionURI and GetResolvedImplementationURI to properly
  represent RenderMan C++ shaders.

- Time sample times from layers whose timeCodesPerSecond value differ from
  the UsdStage are now automatically scaled to match. This can be disabled
  by setting the PCP_DISABLE_TIME_SCALING_BY_LAYER_TCPS environment variable,
  but we expect to remove this in a future release.

- Significant changes and additions to value clips functionality. See
  documentation for more details:
  - Added support for jump discontinuities in clip times metadata to encode
    looping behaviors.
  - The clip manifest is now used to determine which attributes have values
    in clips. The manifest will be generated in-memory at runtime if one
    hasn't been specified.
  - Value resolution will no longer fall through to weaker layers if a clip
    does not have values for an attribute declared in the manifest. In these
    cases, the value may come from the default value authored in the manifest
    or (optionally) be interpolated from surrounding clips.
  - Removed support for legacy value clips metadata.
  - Fixed numerous bugs, including incorrect time samples when reversing
    clips. (Issue: [#1116](https://www.github.com/PixarAnimationStudios/USD/issues/1116))

- Changed the strength ordering of entries in the apiSchemas metadata to be
  strongest-to-weakest, matching the ordering of references and other fields.
- Constructing a UsdGeom::XformOp with an invalid attribute is no longer a
  coding error for consistency with other schemas and objects.
- Deprecated many schemas in UsdRi in preparation for modernization efforts
  in a future release.
- Removed support for deprecated "hermite" and "power" basis from UsdGeomCurves.
- Fixed crash when removing many entries from an SdfPathTable. 
  (PR: [#1172](https://www.github.com/PixarAnimationStudios/USD/pull/1172))
- Fixed compile error due to missing virtual destructor for
  PcpDynamicFileFormatInterface. 
  (PR: [#1156](https://www.github.com/PixarAnimationStudios/USD/pull/1156))
- Fixed type conversions when setting metadata values using dictionaries
  in Python. (Issue: [#813](https://www.github.com/PixarAnimationStudios/USD/issues/813))
- Fixed handling of stage metadata and session layer muting in UsdStage.
- Fixed UsdFlattenLayerStack to handle mismatched attribute types and
  time-sampled asset paths. 
  (PR: [#1169](https://www.github.com/PixarAnimationStudios/USD/pull/1169))
- Fixed undefined behavior issue with iterator comparisons. 
  (Issue: [#1146](https://www.github.com/PixarAnimationStudios/USD/issues/1146))
- Fixed regression in handling of inherited bindings in UsdSkelCache.
- Fixed crash in UsdSkelBakeSkinning. 
  (PR: [#1213](https://www.github.com/PixarAnimationStudios/USD/pull/1213))

### Imaging
- Updated minimum required version of OpenGL to v4.5.

- Added new texture system to Storm. This enables multi-threaded texture
  loading, uses Hgi, and frees the scene delegate from having to load
  textures. In a performance test using Nvidia's Attic scene, time to first
  image dropped by ~90%, from 115s to 10s.

- Added first iteration of HgiMetal, a Metal-based Hgi implementation for
  Apple platforms. Currently, Hgi is used in several Hydra tasks to perform
  tasks like blitting, the goal is to slowly integrate Hgi in Storm.

- Numerous updates to Hgi:
  - Added HgiInterop to exchange rendered results between GL-GL or Metal-GL.
  - Added HgiSampler to represent texture samplers.
  - Added HgiComputeCmds to issue compute commands.
  - Added HgiBlitCmds for mipmap generation and GPU-GPU copy.
  - Added SRGB format and removed unsupported 24-bit formats.
  - Added GetRawResource function to various resource objects to expose
    low-level resource handles.

- Added garbage collection to HgiGL for proper handling of resource destruction.
- Added HdRendererPluginHandle, an RAII object for managing plugin lifetimes.
- Added HdxAovInputTask which takes HdRenderBuffer and (if needed) uploads
  it to the GPU as HgiTexture.
- Added support for down-sampling volumes in Storm.
- Improved performance of volumes in Storm by not re-creating the entire
  volume shader when fields are animated.
- Added support for UsdTransform2d node in Storm. 
  (Issue: [#1207](https://www.github.com/PixarAnimationStudios/USD/issues/1207))
- Added support for UsdUVTexture scale and bias in Storm. 
  (Issue: [#1129](https://www.github.com/PixarAnimationStudios/USD/issues/1129))
- Added USDIMAGINGGL_ENGINE_DEBUG_SCENE_DELEGATE_ID environment variable to
  specify scene delegate ID for debugging. 
  (Issue: [#1093](https://www.github.com/PixarAnimationStudios/USD/issues/1093))
- Changed default version of HdSceneDelegate::GetScenePrimPath to strip
  delegate ID.
- Replaced obsolete GL_GENERATE_MIPMAP with glGenerateMipmap. 
  (Issue: [#1171](https://www.github.com/PixarAnimationStudios/USD/issues/1171))
- Added support for uint16 type for GL textures and OpenImageIO. 
  (PR: [#1212](https://www.github.com/PixarAnimationStudios/USD/pull/1212))
- Restructured OpenImageIO support as a plugin to Glf. 
  (PR: [#1214](https://www.github.com/PixarAnimationStudios/USD/pull/1214))
- Converted common Hydra tasks (e.g. HdxColorCorrectionTask, HdxPresentTask,
  HdxColorizeSelectionTask) to use Hgi.
- Renamed HdxProgressiveTask to HdxTask.
- Storm now uses Hgi for buffers, shaders, and programs instead of OpenGL.
- HdStResourceRegistry is instantiated once per Hgi instance in Storm.
- Unshared computation BAR are reused when possible. 
  (Issue: [#1083](https://www.github.com/PixarAnimationStudios/USD/issues/1083))
- Improved handling of invalid cases in several areas. 
  (PR: [#1232](https://www.github.com/PixarAnimationStudios/USD/pull/1232), 
  PR: [#1201](https://www.github.com/PixarAnimationStudios/USD/pull/1201))
- Improved performance of UsdImagingDelegate::SetTime. 
  (Issue: [#1166](https://www.github.com/PixarAnimationStudios/USD/issues/1166))
- Improved performance for displaying meshes with large numbers of geometry
  subsets. (PR: [#1170](https://www.github.com/PixarAnimationStudios/USD/pull/1170))
- Normals are ignored on skinned meshes so they are computed post-skinning.
- Inherited UsdSkel bindings are now respected.
- Allow HdStRenderBuffer::Resolve() to change the size of the image. 
  (PR: [#1236](https://www.github.com/PixarAnimationStudios/USD/pull/1236))
- Removed "catmark" token in pxOsd in favor of "catmullClark".
- Removed UsdImagingGLMaterialTextureAdapter, UsdImagingGLDomeLightAdapter,
  GlfVdbTexture[Container], and GlfTextureContainer, which are unneeded with
  the new texture system.
- Removed unused HdxColorizeTask and HdxFullscreenShaderGL.
- Removed HdWrapLegacyClamp, which corresponded to the deprecated GL_CLAMP.
- Removed uses of HdStGLSLProgram as part of a new strategy to create these
  via HdStResourceRegistry (since it has access to Hgi).
- Removed transform feedback from non-instanced GPU culling in Storm.
- Fixed incorrect handling of buffer array resizing to 0 elements. 
  (Issue: [#1230](https://www.github.com/PixarAnimationStudios/USD/issues/1230))
- Fixed memory regression when calling UsdImagingDelegate::SampleTransform
  and SamplePrimvar.
- Fixed error in UsdImaging when changing from a time where a mesh provided
  points or normals to a time that does not.
- Fixed potential deadlocks if plugins were loaded at various points in
  UsdImagingGLEngine.
- Fixed error when processing skinned prim with no joint influences.
- Fixed change processing issue with native instanced prims. 
  (Issue: [#1163](https://www.github.com/PixarAnimationStudios/USD/issues/1163))
- Fixed handling of widths primvar with no value.
- Fixed "IsFlipped" computation for native instanced prims. 
  (Issue: [#1190](https://www.github.com/PixarAnimationStudios/USD/issues/1190))
- Fixed crash when updating a removed primvar. 
  (PR: [#1223](https://www.github.com/PixarAnimationStudios/USD/pull/1223))
- Fixed crash when reparenting an instance root. 
  (Issue: [#1245](https://www.github.com/PixarAnimationStudios/USD/issues/1245))
- Fixed various issues with cards prims, including a crash issue. 
  (Issue: [#1210](https://www.github.com/PixarAnimationStudios/USD/issues/1210))
- Fixed selection encoding/decoding of invisible native instances.
- Fixed invalidation when changing the "purpose" attribute. 
  (Issue: [#1243](https://www.github.com/PixarAnimationStudios/USD/issues/1243))

### usdview
- Added "Prototypes" pick mode that selects the prototype for a picked gprim
  in a PointInstancer and highlights the picked instance.
- Reverted "Prims" and "Instances" pick modes to original behavior of selecting
  the root boundable of the picked object. 
  (Issue: [#1196](https://www.github.com/PixarAnimationStudios/USD/issues/1196))
- Fixed "Reopen Stage" when usdview was launched with `--norender`. 
  (PR: [#1192](https://www.github.com/PixarAnimationStudios/USD/pull/1192))
- Fixed camera guide and reticles rendering on high DPI displays.

### Alembic Plugin
- Fixed conversion of mesh subdivision interpolation options between
  UsdGeomMesh and AbcGeom's ISubD and OSubD schemas. 
  (PR: [#1246](https://www.github.com/PixarAnimationStudios/USD/pull/1246))

### Embree Plugin
- Removed support for Embree 2.0. The HdEmbree plugin now requires Embree 3.

### MaterialX Plugin
- Added support for MaterialX 1.37 and deprecated support for 1.36. We
  anticipate requiring 1.37 in a future release.
- MaterialX "vectorN" datatypes are now translated to USD "floatN" datatypes.
- Changed default output name from "result" to "out" to match conventions
  in MaterialX specification.
- Fixed test issue found during Python 3 work. 
  (PR: [#1161](https://www.github.com/PixarAnimationStudios/USD/pull/1161))

### RenderMan Plugin
- Added support for arbitrary numbers of AOVs in HdPrman.
- Removed deprecated RenderMan 22 Hydra plugin.

- Material processing now uses SdrShaderNode::GetResolvedImplementationURI to
  locate shaders, which allows it consume shaders identified via 
  UsdShadeShader's info:sourceAsset property.

- Modified UsdVol support to allow easier extensions with custom volume plugin
  for RenderMan.
- Fixed calculation of specularFaceColor in UsdPreviewSurfaceParameters.osl
  to better match the spec.

## [20.05] - 2020-04-13

This release includes initial support for Python 3. Many thanks to our
collaborators at Nvidia and SideFX for all of their work to make this
happen!

The RenderMan 22 Hydra plugin is deprecated, and the deprecated Houdini
and Katana plugins have been removed from the repository. See details below.

### Build
- Added support for Python 3. This may be enabled by specifying
  `PXR_USE_PYTHON_3=TRUE` when running cmake or by running build_usd.py
  with Python 3.
- Updated C++ standard version to C++14 per VFX Reference Platform CY2018.
- Updated CMake minimum version to 3.12. 2.x support was deprecated in 19.11.
- Moved to Azure Pipelines for CI.
- PySide2 is now preferred over PySide if both are installed. Users can specify
  `PYSIDE_USE_PYSIDE=TRUE` when running cmake to force the use of PySide.
- Building examples and tutorials is now optional. 
  (PR: [#594](https://www.github.com/PixarAnimationStudios/USD/pull/594))

- Various fixes and changes to build_usd.py
  - Updated TBB (2017.6), Alembic (1.7.10), OpenImageIO (1.8.9), and
    OpenColorIO (1.0.9) dependencies for VFX Reference Platform CY2018 and/or
    to reflect versions used for testing internally.
  - Updated OpenSubdiv to 3.4.3. This removes the dependency on GLFW,
    which itself required X libraries on Linux. 
    (Issue: [#261](https://www.github.com/PixarAnimationStudios/USD/issues/261))
  - Miscellaneous fixes. 
    (Issue: [#1110](https://www.github.com/PixarAnimationStudios/USD/issues/1110), 
    Issue: [#1126](https://www.github.com/PixarAnimationStudios/USD/issues/1126))

- Made OpenEXR an optional dependency. It is only required for enabling
  OpenImageIO, OpenVDB, or OSL support. 
  (PR: [#1082](https://www.github.com/PixarAnimationStudios/USD/pull/1082))
- Fixed strict builds on Linux when building with boost version 1.61 or
  earlier. (PR: [#1081](https://www.github.com/PixarAnimationStudios/USD/pull/1081))
- Fixed issue where warnings from 3rd-party headers would cause strict builds
  to fail.
- Fixed bug where the boost_python library would not be found when building
  against boost 1.67+.
- Fixed bug where specifying `PXR_BUILD_DOCUMENTATION=TRUE` but not
  having required programs would result in an incomplete build. 
  (Issue: [#1127](https://www.github.com/PixarAnimationStudios/USD/issues/1127))
- Fixed incorrect instructions in README.md for running build_usd.py under
  Visual Studio 2017. 
  (PR: [#1120](https://www.github.com/PixarAnimationStudios/USD/pull/1120))
- Fixed bug where build_usd.py would pick up external builds of boost 1.70
  or later. 
  (Issue: [#1071](https://www.github.com/PixarAnimationStudios/USD/issues/1071))
- Fixed bug in build_usd.py when building boost with Visual Studio 2019.
  (Issue: [#1041](https://www.github.com/PixarAnimationStudios/USD/issues/1041))
- Fixed issue where non-debug builds with Visual Studio 2019 would not work
  properly or crash immediately. 
  (Issue: [#1095](https://www.github.com/PixarAnimationStudios/USD/issues/1095))
- Fixed several issues that caused errors when building a program against
  USD with clang on Windows. 
  (Issue: [#1030](https://www.github.com/PixarAnimationStudios/USD/issues/1030), 
  PR: [#1079](https://www.github.com/PixarAnimationStudios/USD/pull/1079))
- Fixed issue where debug mode builds on Windows with Python bindings enabled
  would fail without a special debug build of Python. 
  (PR: [#785](https://www.github.com/PixarAnimationStudios/USD/pull/785), 
  Issue: [#1006](https://www.github.com/PixarAnimationStudios/USD/issues/1006))

### USD
- Added typed value proxies to VtValue.
- Added SdfPathAncestorsRange for iterating over an SdfPath's ancestors.
- Added convenience scenegraph object accessor APIs to UsdPrim and UsdStage.
- Added UsdPrim::GetInstances to get all instances of a master. 
  (Issue: [#962](https://www.github.com/PixarAnimationStudios/USD/issues/962))
- Added UsdPrimDefinition to represent the built-in properties of a given prim.
  Users can access a prim's definition via UsdPrim::GetPrimDefinition.
- Added UsdUtilsConditionalAbortDiagnosticDelegate, a diagnostic delegate that
  aborts when an error or warning is encountered based on text-matching rules.
- Added UsdUtilsSparseValueWriter::GetSparseAttrValueWriters to help with
  debugging. (PR: [#1038](https://www.github.com/PixarAnimationStudios/USD/pull/1038))
- Added UsdGeomHermiteCurves schema.
- Deprecated 'hermite' and 'power' basis for UsdGeomBasisCurves.
- Removed remnants of relationship-based shader encoding from UsdShade schemas.
- Removed "shadow:include" and "shadow:exclude" from UsdLuxShadowAPI schema.
- Removed raw spec access API from UsdSchemaRegistry in favor of the new
  UsdPrimDefinition objects.
- For static USD builds, the Plug library now resolves plugin paths relative
  to the path of the executable that links to USD.
- SdfLayer and UsdStage API for timeCodesPerSecond now fall back to using
  framesPerSecond if no timeCodePerSecond is specified.
- Numerous optimizations for UsdStage load time and authoring speed.
- UsdStage now sends notification when population mask, load rules, and
  interpolation type are changed. 
  (Issue: [#1139](https://www.github.com/PixarAnimationStudios/USD/issues/1139))
- Applied API schemas now impart their properties and fallback values as
  builtins on the applied prim.
- Concrete typed schemas can now provide fallback metadata values.
- C++ typenames for schemas are no longer supported as prim typenames
  in scene description.

- UsdGeomImageable now computes 'purpose' using non-pruning semantics.
  See documentation for UsdGeomImageable::ComputePurpose for a description
  of the new behavior.

- Various fixes and changes for usdGenSchema
  - Added --headerTerminatorString option. 
    (Issue: [#1092](https://www.github.com/PixarAnimationStudios/USD/issues/1092), 
    PR: [#1097](https://www.github.com/PixarAnimationStudios/USD/pull/1097))
  - Output directory automatically created if it doesn't already exist.
  - Changed output in generatedSchema.usda for multiple-apply API schemas.
  - Changed output ordering for tokens in tokens.h.

- Fixed race condition in UsdGeomBoundable::ComputeExtentFromPlugins that
  could cause incorrect results for bounds computations.
- Fixed bug with value block handling in UsdFlattenLayerStack.
- Fixed incorrect documentation on crate file structure. 
  (Issue: [#1072](https://www.github.com/PixarAnimationStudios/USD/issues/1072))
- Fixed change processing bug for nested instancing that led to crashes.
- Fixed bug reading .usdc files in .usdz files if USDC_USE_PREAD=1 was set.
- Fixed usdedit failure on Windows due to file access conflicts. 
  (PR: [#1094](https://www.github.com/PixarAnimationStudios/USD/pull/1094))
- Fixed line ending issue causing some tests to fail on Windows. 
  (PR: [#848](https://www.github.com/PixarAnimationStudios/USD/pull/848))

### Imaging
- Added swizzle metadata to the output of UsdUVTexture. 
  (Issue: [#657](https://www.github.com/PixarAnimationStudios/USD/issues/657))
- Added ability to display unloaded prims as bounding boxes. 
  (PR: [#1145](https://www.github.com/PixarAnimationStudios/USD/pull/1145))
- Added selection highlight outline capabilities (not exposed in usdview).
- Added many more improvements to volume visualizations in Storm.
- Added basic support for transporting pinned curves to UsdImaging.
- Added support for computed vertex primvars for points prim in Storm.
- Added support for element invising and point colors in basis curves.
- Added hdTiny, a very simple example render delegate.
- Added support for cascaded shadow maps (not exposed in usdview).
- Added IsStopSupported/Stop/Restart APIs to render delegates.
- Added transport of light filters through usdImaging.
- Added HdResourceRegistry::ReloadResource API to allow for texture reloading.
- Added USDVIEW_OCIO_LUT3D_EDGE_SIZE environment variable for setting 3D LUT
  size for OpenColorIO.
- Removed IsFlipped() from certain Storm shaders for clean up.
- Many changes to remove or replace uses of boost with C++ features.
- Changed many APIs in Hgi as that subsystem continues to be built up.
- Changed HdResourceRegistry APIs for several cleanups and fixes.
- Replaced HdSceneDelegate::GetPrimPathForInstanceIndex with GetScenePrimPath.
- Moved HdMaterialParam to HdSt and made it private.
- Moved HdxSimpleLightingShader to HdSt.
- Fixed issue with Storm shutdown on certain platforms.
- Fixed several issues with selection highlighting (including issues relating
  to instancing) in usdImaging.
- Fixed issue with Storm not drawing geometry when a PxrSurface is attached.
  (Issue: [#1128](https://www.github.com/PixarAnimationStudios/USD/issues/1128))
- Fixed issue where UsdUVTexture alpha output was driven by the wrong channel.
  (Issue: [#657](https://www.github.com/PixarAnimationStudios/USD/issues/657))
- Fixed drawing of sphere implicit to be centered. 
  (Issue: [#1086](https://www.github.com/PixarAnimationStudios/USD/issues/1086))
- Fixed handling of primvar addition and removal in Hydra. 
  (Issue: [#1078](https://www.github.com/PixarAnimationStudios/USD/issues/1078))
- Fixed .glslfx processing to use Ar for resolving paths for includes.
- Fixed parsing of texture default values for .glslfx files in Sdr.
- Fixed issue with texture binding with certain configurations. 
  (Issue: [#1063](https://www.github.com/PixarAnimationStudios/USD/issues/1063))
- Fixed several cleanup issues. 
  (Issue: [#1103](https://www.github.com/PixarAnimationStudios/USD/issues/1103), 
  PR: [#1104](https://www.github.com/PixarAnimationStudios/USD/pull/1104))
- Fixed order of draw targets drawing in HdxDrawTargetTask.
- Fixed default values for displayColor, displayOpacity and widths. 
  (PR: [#1098](https://www.github.com/PixarAnimationStudios/USD/pull/1098))
- Fixed point instancer resync of prototypes. For example, we now resync
  prototypes properly if their material bindings change.
- Fixed point instancer refresh in response to structural primvar and prototype
  updates. (PR: [#1077](https://www.github.com/PixarAnimationStudios/USD/pull/1077))
- Fixed HdRenderIndex::Clear to always call Finalize() for cleared rprims.
  (Issue: [#1040](https://www.github.com/PixarAnimationStudios/USD/issues/1040))
- Fixed color banding issue when using OpenColorIO.
- Fixed reading stride with OpenImageIO. 
  (Issue: [#1080](https://www.github.com/PixarAnimationStudios/USD/issues/1080))
- Fixed drawMode updates with native instancing. 
  (Issue: [#1069](https://www.github.com/PixarAnimationStudios/USD/issues/1069))
- Fixed case sensitivity for image loading. 
  (PR: [#804](https://www.github.com/PixarAnimationStudios/USD/pull/804))
- Fixed issue with Storm shader generation when input and primvar names match.
- Fixed debug flag not properly displaying source when shader fails to link.
  (PR: [#1020](https://www.github.com/PixarAnimationStudios/USD/pull/1020))

### usdview
- Added ability to change font size.
- Added 'Apply' button to renderer settings dialog.
- Improved start up time by waiting until we've populated more of the UI.
- Made free-cam's FOV part of saved user settings.
- Fixed multi-line paste in the interpreter. 
  (Issue: [#1117](https://www.github.com/PixarAnimationStudios/USD/issues/1117))
- Fixed errors when using PySide2 5.14.0 or later. 
  (Issue: [#1111](https://www.github.com/PixarAnimationStudios/USD/issues/1111))
- Fixed errors when closing usdview with the native windowing system commands.

### Alembic Plugin
- Added file format arguments 'abcReRoot' for reparenting hierarchy beneath
  a new parent prim and 'abcLayers' for specifying a list of secondary files
  to load as layered Alembic. 
  (PR: [#1099](https://www.github.com/PixarAnimationStudios/USD/pull/1099))
- Only deliver interesting timeSamples for constant properties. 
  (PR: [#1114](https://www.github.com/PixarAnimationStudios/USD/pull/1114))
- Relaxed type-checking on position property and let any float[3] type through.
  (PR: [#1115](https://www.github.com/PixarAnimationStudios/USD/pull/1115))
- Removed need to link directly against Alembic backend libraries to open files.

### Houdini Plugin
- The Houdini plugin has been removed from the USD repository in favor of the
  native USD support in Houdini's Solaris suite.

### Katana Plugin
- The Katana plugin has been removed from the USD repository in favor of the
  Foundry-supported Katana plugins available at
  https://github.com/TheFoundryVisionmongers/KatanaUsdPlugins.

### MaterialX Plugin
- Fixed bug where implementation file relative paths were assumed to be
  relative to the .mtlx file with the implementation node instead of to
  the MaterialX library itself.

### RenderMan Plugin
- Deprecated support for RenderMan 22. We anticipate removing the renderman-22
  plugin in a future release.
- Added support for HdAov clearValue.
- Added support for Stop/Restart functionality.
- Added early initial support for light filters and trace groups.
- Fixed bug that caused empty renders when no scene camera was present.
  (Issue: [#1113](https://www.github.com/PixarAnimationStudios/USD/issues/1113))
- Fixed routing of ri:attributes properties when instancing is involved.
- Fixed restarting of RenderMan when application is told that resources
  (e.g., textures) need to be reloaded.

## [20.02] - 2020-01-24

We anticipate aligning with the version requirements from the VFX Reference
Platform CY2018 (https://vfxplatform.com) in the next release. In particular,
this includes requiring support for C++14.

This release adds a Hydra plugin for RenderMan 23 and removes the deprecated
Maya plugin from the repository. This release also deprecates support for the
Houdini and Katana plugins. See details below.

### Build
- The source tree has been restructured so that code no longer lives under a
  separate "lib" directory. This means that (with a few exceptions) include
  paths now match source tree paths.

- Added support for Visual Studio 2019 to build_usd.py. 
  (PR: [#1042](https://www.github.com/PixarAnimationStudios/USD/pull/1042))
- Added PXR_BUILD_USD_TOOLS option to disable building of command-line tools
  (e.g. usdcat, usdedit) 
  (PR: [#1048](https://www.github.com/PixarAnimationStudios/USD/pull/1048))
- The PXR_LIB_PREFIX option now defaults to "" and is respected for .lib files
  on Windows. 
  (Issue: [#1027](https://www.github.com/PixarAnimationStudios/USD/issues/1027))

### USD
- Added "subIdentifier" and "sourceType" parameters to GetNodeFromAsset API in
  Ndr and Sdr and "subIdentifier" attribute to UsdShadeShader schema. This
  allows clients to identify and retrieve definitions for a particular shader
  in source assets that have multiple shaders defined in them. 
  (Issue: [#919](https://www.github.com/PixarAnimationStudios/USD/issues/919))

- Removed unused legacy SdfVariabilityConfig variability value.
- Fixed bug where prims in masters would be populated incorrectly when a
  UsdStage's population mask contained paths descendant to masters.
- Fixed bug where layer stack flattening would lose some variant selections.
  (Issue: [#1060](https://www.github.com/PixarAnimationStudios/USD/issues/1060))
- UsdStage::CreateClassPrim is no longer restricted to root prims. 
  (Issue: [#994](https://www.github.com/PixarAnimationStudios/USD/issues/994))
- UsdVariantSets::GetNames() now returns the union of all variant set names
  across layer stacks.
- Improved performance of retrieving metadata with templated API in UsdObject.
- Numerous changes to improve performance when making edits to many properties.
- Removed support for inverse layer offsets which were deprecated in v0.8.2.
- Removed support for legacy UsdShade encodings, yielding cleaner code and
  improved performance.
- Improved diagnostics in usdGenSchema for non-identifier tokens in schema.usda.
- Improved documentation generated by usdGenSchema for schema attributes.
- usdGenSchema now treats all C++ and Python keywords as reserved and will
  prepend a "_" if they are encountered in any identifiers.
- Modified UsdGeomXformCommonAPI to be generated and behave like other API
  schemas (as the name implies) for consistency.
- Fixed bug in UsdGeomImageable::MakeVisible where the given prim would not be
  made visible if other "invis" opinions existed on ancestor prims. 
  (PR: [#1043](https://www.github.com/PixarAnimationStudios/USD/pull/1043))
- Fixed bug in UsdGeomSubset::GetUnassignedIndices where passing in an
  invalid element count could lead to a crash. 
  (Issue: [#989](https://www.github.com/PixarAnimationStudios/USD/issues/989))
- Removed deprecated shader binding API on UsdShadeMaterial in favor of
  UsdShadeMaterialBindingAPI schema.
- Improved fidelity of light-related UsdRi schemas.
- Added UsdLuxDomeLight::OrientToStageUpAxis utility for creating dome lights
  with an appropriate default orientation. 
  (Issue: [#938](https://www.github.com/PixarAnimationStudios/USD/issues/938))
- Added UsdMedia schema library and UsdMediaSpatialAudio schema.
- Fixed bugs in UsdSkelCache::ComputeSkelBinding where inherited skel:skeleton
  bindings were incorrectly evaluated and bindings would be returned for
  inactive prims.
- Python binding for UsdSkelSkinningQuery is now UsdSkel.SkinningQuery.

### Imaging
- Added RenderMan 23 Hydra plugin.
- Added support for specifying thread limits and convergence criteria in
  HdPrman via settings.
- Added initial support for light filters.
- Added an Sdr parser plugin for .glslfx files.
- Added support for material networks in Storm.
- Added initial support for volume rendering in Storm.
- Added basic support for bindless shadow maps in Storm.
- Added primvar/points editing support for points, basis curves, and meshes
  in UsdImaging. Previously everything was resync.
- Added Hydra changetracker dependencies between parent and child instancers.
- Added new task HdxColorChannelTask to output a specific display channel.
- Added command line option (--traceToFile) to usdview to start tracing as soon
  as the application is started.
- Changed HdPrman linking to be independent of RenderMan install.
- Changed "depth" AOV range from [-1, 1] to [0, 1].
- Changed name "linearDepth" AOV to "cameraDepth".
- Changed HdxPickHit "ndcDepth" field with a range of [-1,1] to
  "normalizedDepth" with a range of [0,1]
- Changed edge hashing to use triangular numbers in Hd. This is about 3x faster
  than the previous code.
- Changed UsdImaging's DrawModeAdapter to use HdMaterialNetwork.
- Changed default camera shutter in UsdImaging to match UsdGeomCamera [0,0].
- Improved Storm's instance registry and moved resource registries from Hd core
  to Storm.
- Improved aggregation of material shaders in Storm.
- Improved material primvar filtering in Storm.
- Improved property refresh support in UsdImaging for instance attributes.
- Removed HydraMaterialAdapter in UsdImaging since it is no longer needed.
- Removed old Hydra scene delegate API that was only used by the old material
  system in Storm. (GetSurfaceShaderSource, GetDisplacementShaderSource,
  GetMaterialParams and others..).
- Fixed instance adapter selection highlighting performance issues.
- Fixed bug where pick task was continuously running in usdview.
- Fixed Storm to skip string primvars.
- Fixed bug where rtx_glfImage was not honoring the channel offset and number
  of channels specified in the FillRequest parameter.
- Fixed nullptr exception in HdPrman when rendering without a camera.
- Fixed data ownership of instance indices. DirtyInstanceIndex is set on
  instancers and tells them they need to re-fetch their index buffer, relative
  to one of their prototypes.
- Fixed UsdImaging native instancing so instead of compensating for prototype
  transforms by overwriting prototype transforms, it compensates in the
  per-instance transform primvar.
- Fixed usdSkelImaging dirtyness propagation to the skinning aggregator
  computation.
- Fixed double color correction on MacOS.
- Implemented SamplePrimvar for native instancer adapter. 
  (Issue: [#996](https://www.github.com/PixarAnimationStudios/USD/issues/996))
- Fixed UsdPreviewSurface material displacement in Storm. 
  (Issue: [#922](https://www.github.com/PixarAnimationStudios/USD/issues/922), 
   Issue: [#1026](https://www.github.com/PixarAnimationStudios/USD/issues/1026))
- Fixed change processing when an inherited primvar is removed. 
  (Issue: [#1004](https://www.github.com/PixarAnimationStudios/USD/issues/1004))
- Fixed incorrect points buffer computation for implicit geometry. 
  (Issue: [#1024](https://www.github.com/PixarAnimationStudios/USD/issues/1024))
- Added GetHgiTextureHandle API on renderbuffer to avoid HdSt dynamic_casts
  and CPU -> GPU copies during post-process pipeline 
  (Issue: [#1008](https://www.github.com/PixarAnimationStudios/USD/issues/1008))
- Fixed TBB library linkage in hdEmbree. 
  (PR: [#991](https://www.github.com/PixarAnimationStudios/USD/pull/991))
- Added support for 'jpeg' extension for Glf_StbImage. 
  (PR: [#998](https://www.github.com/PixarAnimationStudios/USD/pull/998))
- Fixed bug where time-invariant geometry subsets would mask time-varying
  topology from Hydra. 
  (PR: [#1045](https://www.github.com/PixarAnimationStudios/USD/pull/1045))
- Fixed bug where UsdGeomSubset indices were always retrieved at the default
  time. (PR: [#1059](https://www.github.com/PixarAnimationStudios/USD/pull/1059))
- Fixed incomplete handling of UsdGeomMesh face-varying options. 
  (PR: [#1061](https://www.github.com/PixarAnimationStudios/USD/pull/1061))

### Houdini Plugin
- Deprecated Houdini plugins in favor of Houdini-native USD support.
  We anticipate removing the plugin from the USD repository in a future
  release.
- Additional changes for Houdini 17.5 compatibility.

### Katana Plugin
- Deprecated Katana plugin in favor of the Foundry-supported Katana plugins 
  available at https://github.com/TheFoundryVisionmongers/KatanaUsdPlugins.
  We anticipate removing the plugin from the USD repository in a future release.
- Changed how UsdGeomBasisCurve Widths and Normal attributes are read into
  Katana. Author either a geometry.point.* entry (old behavior) or a
  geometry.arbitrary.* entry (new behavior) based on the attribute's
  interpolation.
- Changes in preparation for reading UsdShadeMaterials which contain
  UsdShadeNodegraph subgraphs.
- Removed custom Viewer Modifier Plugins. These were used by Katana's legacy
  Viewer tab, which has been replaced by the Hydra Viewer tab.
- Removed support for /LooksDerivedStructure in USD shader libraries.
- Fixed incorrect geometry.instanceSource attribute for nested instances.
  (PR: [#1015](https://www.github.com/PixarAnimationStudios/USD/pull/1015))

### MaterialX Plugin
- Added support for versions 1.36.3 and later.

### Maya Plugin
- The Maya plugin has been removed from the USD repository. Development for
  the plugin has moved to the Autodesk maya-usd repository located at
  https://github.com/Autodesk/maya-usd.

## [19.11] - 2019-10-18

This release deprecates support for cmake 2.x and the Maya plugin.
See details below.

### Build
- Deprecated support for cmake 2.x. We anticipate requiring cmake 3.x in the
  next release.
- build_usd.py now fails if using 32-bit Python, which is a common gotcha on
  Windows. (Issue: [#921](https://www.github.com/PixarAnimationStudios/USD/issues/921))
- Fixed issue where build_usd.py would fail to build TBB on MacOS if CUDA was
  installed due to an issue in the TBB build system. 
  (Issue: [#767](https://www.github.com/PixarAnimationStudios/USD/issues/767))
- Made PXR_VERSION a valid and comparable integer in C++. 
  (PR: [#886](https://www.github.com/PixarAnimationStudios/USD/pull/886))
- Fixed issue where pxrTargets.cmake would include plugins with no exported
  symbols. This caused errors when building against USD on Windows. 
  (Issue: [#530](https://www.github.com/PixarAnimationStudios/USD/issues/530))
- Fixed issue where downstream cmake projects using pxrTargets.cmake could not
  find the USD headers.

### USD
- Added warning when reading a .usda layer with size in MB greater than value
  of SDF_TEXTFILE_SIZE_WARNING_MB environment variable. 
  (PR: [#980](https://www.github.com/PixarAnimationStudios/USD/pull/980))
- Added UsdStageLoadRules to support more robust and declarative intent about
  how payloads should be included in a UsdStage.
- Added UsdPrimCompositionQuery to facilitate discovery of composition arcs
  affecting a UsdPrim.

- Added "timecode" scene description value type. This type is represented by
  the SdfTimeCode class and can be used for attribute and metadata values.
  Layer offsets are applied to all timecode values during UsdStage
  value resolution, enabling "timing curve"-like attributes.

- Added support for Google's Draco compression library. This includes an
  SdfFileFormat plugin for reading .drc files and a standalone "usdcompress"
  tool that extracts and compress Mesh primitives from a USD file, creates
  .drc files for each one, and references them back into a result USD file.
  (PR: [#912](https://www.github.com/PixarAnimationStudios/USD/pull/912))

- Added --quiet flag to usdGenSchema for suppressing output. 
  (PR: [#480](https://www.github.com/PixarAnimationStudios/USD/pull/480))
- Added --brief flag to usddiff so differing lines are not printed.
- Added support for indexed uint primvars to UsdGeomPrimvar. 
  (PR: [#861](https://www.github.com/PixarAnimationStudios/USD/pull/861))

- Added "accelerations" attribute to UsdGeomPointInstancer schema and updated
  UsdGeomPointInstancer::ComputeInstanceTransformAtTime(s) to account for
  authored accelerations, if present.

- Added "accelerations" attribute to UsdGeomPointBased schema and added
  UsdGeomPointBased::ComputePointsAtTime(s) to compute point positions using
  authored velocities and accelerations, if present.

- Added initial version of UsdRender domain and schemas. See documentation and
  white paper on openusd.org for more details. 
  (Issue: [#910](https://www.github.com/PixarAnimationStudios/USD/issues/910))
- Added UsdShadeInput::GetValueProducingAttribute to facilitate more robust and
  correct processing of UsdShade shading networks.
- Added initial support for skinning mesh normals in UsdSkel.

- Removed SdfAbstractDataSpecId and updated the SdfAbstractData interface to
  use SdfPath in its place. Existing SdfAbstractData subclasses must be
  updated to match. See this usd-interest post for more details:
      https://groups.google.com/forum/#!topic/usd-interest/IVmd1t1GKBA

- Numerous fixes for C++14 / gcc6 warnings. 
  (PR: [#869](https://www.github.com/PixarAnimationStudios/USD/pull/869))
- Various optimizations and cleanups in trace library.

- Inherits and specializes arcs that target root prims ("global" arcs) are now
  combined with arcs that target sub-root prims ("local" arcs) for strength
  ordering in composition. Previously, local arcs would always be considered
  stronger than global arcs regardless of their authored order.

- Calling UsdReferences::SetReferences (and equivalent API on UsdPayloads,
  UsdInherits, and UsdSpecializes) with an empty vector now authors an
  explicit empty list op instead of being a no-op. 
  (Issue: [#749](https://www.github.com/PixarAnimationStudios/USD/issues/749))

- Numerous fixes and changes to Sdr and associated parser plugins. This
  includes the addition of a "terminal" property type and "role" and
  "renderType" metadata to better describe shader properties and their
  representation in scene description.

- Numerous optimizations for UsdStage load time and authoring speed.
- Improved performance of adding empty sublayers to an existing UsdStage.

- USD native instancing now includes both LoadRules and PopulationMasks in
  instancing keys, so different instances can have different load state.
  Instances that have equivalent load rules and population masks still share,
  but those that do not will use different masters.

- Load/Unload operations on a UsdStage now must operate on the "expanded" stage
  namespace. That is, the namespace as it would be if instancing was disabled.
- Updated UsdLuxDomeLight schema to clarify a dome light's orientation and
  its textures' orientation. 
  (Issue: [#938](https://www.github.com/PixarAnimationStudios/USD/issues/938))
- Changed 'pointIndices' attribute on UsdSkelBlendShape schema from uint[]
  to int[] for consistency with other core schemas. 
  (Issue: [#858](https://www.github.com/PixarAnimationStudios/USD/issues/858))
- Rewrote UsdSkelBakeSkinning to provide a more efficient baking solution.
- Fixed bug where converting .usdc files from version 0.7 to 0.8 when saving
  an existing file would produce incorrect layer offsets in payloads.
- Fixed crashes when calling methods on invalid Usd.SchemaBase objects in
  Python. 
  (Issue: [#872](https://www.github.com/PixarAnimationStudios/USD/issues/872), 
  PR: [#876](https://www.github.com/PixarAnimationStudios/USD/pull/876))
- Fixed crash when muting and unmuting layers on a UsdStage. 
  (Issue: [#883](https://www.github.com/PixarAnimationStudios/USD/issues/883))

### Imaging
- Renamed the real-time engine/rasterizer to "Storm"
- Added support for dome lights in Storm.
- Added support for screen-space adaptive tessellation and drawing for Loop
  scheme subdivision surface meshes when using OpenSubdiv 3.4 in Storm.
- Added basic support for AOVs in Storm. In usdview, you can now switch between
  color and depth when using Hydra Storm.
- Added Hgi (Hydra Graphics Interface) library. Hgi abstracts away graphics
  API calls in Storm.
- Added HgiGL library, an OpenGL backend implementation for Hgi.
- Added support for vstructs in Hydra RenderMan plugin.
- Added support for light attributes in UsdImaging and Hydra RenderMan plugin.
- Added an adapter in UsdImaging to read camera information, including custom
  camera parameters.
- Added support for physical camera parameters in Hydra and Hydra RenderMan
  plugin.
- Added support for camera motion blur in Hydra RenderMan plugin.
- Added support to read any number of time samples authored in Usd. Improved
  Hydra API to allow render delegates to extract any number of samples from
  scene delegates.
- Added API to HdRenderDelegate and the render thread to pause and resume
  renderers if supported. Updated Hydra RenderMan plugin to support it. 
  (PR: [#888](https://www.github.com/PixarAnimationStudios/USD/pull/888))
- Added API to HdRenderDelegate to return render stats to the client. Updated
  Hydra Embree to expose the number of samples completed this way. 
  (PR: [#888](https://www.github.com/PixarAnimationStudios/USD/pull/888))
- Added element/scope selection highlighting for raytraced backends.
- Added basic support for NURBS curves to UsdImaging. For now, it will
  send them to the render delegate as basis curves.
- Added support for UsdShadeNodeGraph to UsdImaging.
- Added support for acceleration primvars in UsdImaging and Hydra.
- Added support for inherited primvars to native instances.
- Added instance-inheritance support for string-typed primvars.
- Added convergence criteria to render settings in Hydra RenderMan.
- Added support for basis curves instancing in Hydra RenderMan plugin.
- Added support for OpenImageIO's IOProxy feature to allow supported image
  files (such as .exr) to be read from .usdz files.
- Added "Set As Active Camera' on USD camera prims context menu in usdview.
- Added a "Pause" menu item to usdview, and display the render status message.
- Replaced the key/fill/back lights in usdview by a dome light, still only
  works in Storm for now.

- Deprecated old material accessors on HdSceneDelegate in favor of using
  material networks in Storm.
- Removed TrackVariabilityPrep/UpdateForTimePrep in UsdImaging.

- Moved usdShaders from usdImaging/lib to usdImaging/plugin.
- Moved rendererPlugin and rendererPluginRegistry from hdx to hd.
- Moved UsdPreviewSurface conversion to the MatfiltFilterChain in
  Hydra RenderMan plugin.
- Changed return type of HdRenderBuffer::Map from uint8_t to void*.
- Improved handling of invalidations and edits of data such as instancers,
  primvars and others, in UsdImaging.
- Improved 32 bit color support when using Hydra and the default task controller.
- Improved draw item batching in Hydra Storm. 
  (PR: [#528](https://www.github.com/PixarAnimationStudios/USD/pull/528))
- Improved GLSL error reporting in Storm.
- Improved usdrecord to allow more consistent results with usdview (e.g.,
  default materials, purpose).
- Prim browser in usdview now maintains its expansion-state and view when the
  stage recomposes.

- Prim browser in usdview now behaves more like Maya's outliner with respect to
  viewport selections: the PrimView will no long expand/scroll to display the
  selected prim until you use the 'f' hotkey over it to frame the selection.
  Instead, all of the ancestors of selected prims will be highlighted in a
  secondary color.

- Searches in the usdview prim browser begin at the currently selected prim
  rather than at the root of the scene.
- Made usdview panes easier to resize by doubling size of splitter lanes.
- Made usdview timeline playhead much larger to make it easier to grab.
- Fixed inheriting primvars through instance boundaries when the primvar is an
  array.
- Fixed the way UsdGeomPointInstancer calculates purpose and visibility across
  instance/master boundaries.
- Fixed memory leak for draw items in Storm.
- Fixed handling infinite bounds explicitly during GPU frustum culling in Storm.
- Fixed UsdShadeShader material invalidations in UsdImaging.
- Fixed handling of topology changes for basis curves in Storm.
- Fixed ortho camera handling in Hydra RenderMan plugin.

### Houdini Plugin
- Added general methods for converting between USD values and GA_Attribute data.
- Fixed missing usdSkel dependency. 
  (PR: [#827](https://www.github.com/PixarAnimationStudios/USD/pull/827))

- Detect skinning influences stored as detail attribs when converting to
  capture attributes. This allows rigid shapes to be coalesced into deforming
  agent shapes.

### Katana Plugin
- Point instancers with velocities now get motion blur on the first frame,
  even if their positions are not sampled at any previous frames.
- Katana paths are now better aligned with USD paths when possible for point
  instancer prototypes.
- Includes "prmanLightParams.iesProfileNormalize" when reading UsdLux lights
  in as RenderMan representations

- Fixed bug where per-instance primvar values on point instancers could be
  inherited by the prototype geometry, causing warnings about mismatched sizes.
  (Issue: [#893](https://www.github.com/PixarAnimationStudios/USD/issues/893))

### MaterialX Plugin
- Fixed bug when reading a .mtlx file with more than 1 bindinput.
  (Issue: [#950](https://www.github.com/PixarAnimationStudios/USD/issues/950), 
  PR: [#956](https://www.github.com/PixarAnimationStudios/USD/pull/956))

### Maya Plugin
- Deprecated Maya plugin in favor of the Autodesk maya-usd plugin available at
  https://github.com/Autodesk/maya-usd. We anticipate removing the plugin from
  the USD repository in the next release and recommend users begin transitioning
  to the Autodesk plugin -- it contains the same features as the Pixar plugin
  (in fact, it is the same code).

- FindMaya.cmake module on MacOS (once again) supports paths that include the
  "Maya.app/Contents" subdirectory when specifying MAYA_LOCATION. 
  (PR: [#878](https://www.github.com/PixarAnimationStudios/USD/pull/878))
- Visibility for Maya point instancers is now exported to UsdGeomPointInstancer.
- Maya "stroke" nodes are now exported as UsdGeomBasisCurves. 
  (Issue: [#867](https://www.github.com/PixarAnimationStudios/USD/issues/867))
- Various bug fixes and cleanup.

## [19.07] - 2019-06-17

### Build
- Improved detection of GLEW and Ptex libraries during build. 
  (PR: [#808](https://www.github.com/PixarAnimationStudios/USD/pull/808))
- Fixed bug in build_usd.py that caused errors when specifying the
  "Xcode" CMake generator.

### USD:
- Various optimizations and cleanups in trace library.
- Added iterator-based construction for VtArray. 
  (PR: [#644](https://www.github.com/PixarAnimationStudios/USD/pull/644))
- Added Python bindings for ArResolver::RefreshContext. 
  (PR: [#820](https://www.github.com/PixarAnimationStudios/USD/pull/820))
- Fixed incorrect results from NdrRegistry::GetAllNodeSourceTypes.
- Removed unused spec types from Sdf.
- Reimplemented SdfPath. The new implementation dramatically improves the
  performance of creating property paths, which is a key part of value
  resolution.
- SdfFileFormat::IsStreamingLayer was moved to SdfAbstractData::StreamsData.
  SdfAbstractData implementations are now responsible for expressing
  whether they stream data from their back-end instead of SdfFileFormat.
- Added file format arguments to SdfLayer::CreateAnonymous to match CreateNew.
- Fixed bug where strings in scene description that look like format
  specifiers would be interpreted as such during .usda output.
- Added dynamic payloads feature. Dynamic payloads allow file formats to
  procedurally generate a layer using metadata authored on the prim where the
  layer is introduced via a payload arc. See examples and "Advanced Features"
  documentation for more details.
- Fixed erroneous composition cycle detection for subroot references to
  sibling prims across reference boundaries.
- Added support for "schemaTokens" customData entry in schema.usda. This
  allows developers to associate tokens with a particular schema for clarity
  and organization. These tokens will continue to be emitted into the
  standard tokens.h and tokens.cpp files in each schema library.
- Removed deprecated UsdListPositionTempDefault and USD_AUTHOR_OLD_STYLE_ADD
  environment variable.
- Improved error detection for truncated .usdc files.
- Improved error messages and debug output for USD schema generation.
  (PR: [#427](https://www.github.com/PixarAnimationStudios/USD/pull/427), 
  PR: [#478](https://www.github.com/PixarAnimationStudios/USD/pull/478))
- Fixed various issues that caused build errors on Windows. 
  (Issue: [#812](https://www.github.com/PixarAnimationStudios/USD/issues/812))
- Fixed bug that caused corrupt .usdc files to be created in certain rare
  cases. (Issue: [#811](https://www.github.com/PixarAnimationStudios/USD/issues/811))
- Fixed bug where asset-valued stage metadata would not be resolved.
- Calling Usd.Prim.IsDefined on a null prim in Python now raises a
  Python exception. 
  (Issue: [#753](https://www.github.com/PixarAnimationStudios/USD/issues/753))
- Fixed bug in UsdUtils dependency extraction and .usdz creation where
  asset dependencies in layer metadata were ignored.
- Improved integer encoding and fixed a source of non-deterministic output
  in .usdc file format. 
  (Issue: [#830](https://www.github.com/PixarAnimationStudios/USD/issues/830))
- Added ability to usdcat to only show layer metadata via --layerMetadata
  command-line flag.
- UsdGeomBasisCurves schema now accepts "pinned" as a valid value for
  wrapping to represent pinned curves.
- UsdGeomPointInstancer will now use the "append" list op when authoring
  inactiveIds metadata instead of the deprecated "added" list op. This
  behavior can be reverted by setting the environment variable
  `USDGEOM_POINTINSTANCER_NEW_APPLYOPS` to 0, but this will be removed
  in a future release.
- Added UsdShadeCoordSysAPI schema for representing coordinate systems.
- Added shaping:ies:normalize attribute to UsdLuxShapingAPI.
- Numerous fixes and improvements to UsdSkel schemas.

### Imaging:
- Added early version of RenderMan Hydra render delegate with support for
  geometry,material networks, lights, volumes, coordinate systems and more!
- Added support for coordinate systems in Hydra and UsdImaging.
- Added support for categories per instance in Hydra to support light linking
  to individual instances.
- Added order independent transparency (OIT) to Hydra GL.
- Added support for backend-independent picking and highlighting.
- Improved Hydra support for scenegraph cameras.
- Added support for instance-rate primvar queries in Hydra GL shading code.
- Added support for generating normals for picking hits in Hydra GL.
- Added "DontConform" camera window policy.
- Added support for picking points in Hydra GL.
- Added depth compositing to HdxColorizeTask.
- Render delegates can now access "velocities" attribute on point-based schemas.
  (PR: [#824](https://www.github.com/PixarAnimationStudios/USD/pull/824))
- Refactored and clarified use of "path" and "prim" terminology in UsdImaging.
- Removed HdChangeTracker::MarkAllCollectionsDirty
- Removed render tags API from HdRprimCollection. They have become a Hydra
  task concept.
- Fixed instancing-related crashes in UsdImaging. 
  (Issue: [#838](https://www.github.com/PixarAnimationStudios/USD/issues/838), 
  Issue: [#847](https://www.github.com/PixarAnimationStudios/USD/issues/847))
- Added usdAppUtils library containing common functionality and utilities
  for applications that view USD stages.
- Added initial version of usdrecord command line tool for generating images
  from a USD file.
- Added "debug flags" configuration dialog in usdview.
- Added ability to specify first/last/current frame when launching usdview
  via "--ff", "--lf", and "--cf" command line parameters. 
  (PR: [#832](https://www.github.com/PixarAnimationStudios/USD/pull/832))
- Numerous correctness, interaction, and performance improvements to usdview's
  transport control (frame slider). 
  (PR: [#770](https://www.github.com/PixarAnimationStudios/USD/pull/770))

### Alembic Plugin:
- Added support for Alembic 1.7.9. 
  (PR: [#825](https://www.github.com/PixarAnimationStudios/USD/pull/825))

### Houdini Plugin:
- Support for Houdini 16.0 is deprecated. We anticipate bumping the minimum
  version requirement to 16.5 and dropping 16.0 support in the next release.
- Fixed bug which could cause visibility to be incorrect.
- Added support for export of additional primvar types.
- Fixed bug which could cause incorrect attribute typeinfo when importing
  array attributes.
- Added support for additional OSD subdiv interpolation tags.
- Added support for targeting the default prim on import via a reserved
  "defaultPrim" prim path.
- Miscellaneous improvements for converting between UsdSkel and Agent
  primitives.

### Katana Plugin:
- Added support for reading and resolving collection-based material bindings to
  PxrUsdIn. This is done as a two stage process in which bindings (for the
  purposes specified in "additionalBindingPurposeNames") are added within a
  "usd.materialBindings" group attribute. The PxrUsdInResolveMaterialBindings
  node (and its corresponding Op) transfers a purpose's binding to the
  system-level "materialAssign" attribute and optionally filters inherited
  redundancy.
- The "info.usd" attributes which advertise which API schemas are applied can
  be created in either array or group attribute form. The latter is convenient
  to match against via CEL.

### Maya Plugin:
- Add support for exporting assembly edits on pxrUsdReferenceAssembly nodes.
- Expose controls for tweaking selection parameters in pxrHdImagingShape.
- Importing shaders and lights will use mel commands to property add shaders
  to hypershade and lights to the defaultLightSet.

## [19.05] - 2019-04-10

### Build
- Added support for Ninja build system in build_usd.py. 
  (PR: [#590](https://www.github.com/PixarAnimationStudios/USD/pull/590))
- Added --build-debug option to build_usd.py. 
  (PR: [#502](https://www.github.com/PixarAnimationStudios/USD/pull/502))
- Better support for static builds in exported pxrConfig.cmake. 
  (PR: [#787](https://www.github.com/PixarAnimationStudios/USD/pull/787))
- .pdb files are now installed with the libraries on Windows. 
  (PR: [#502](https://www.github.com/PixarAnimationStudios/USD/pull/502))
- MacOS users can now run build_usd.py with Maya's Python interpreter (mayapy)
  to ensure that USD and the Maya plugin will be built against Maya's Python.
  This requires the --no-usdview option, as Maya does not provide the OpenGL
  module in Python. 
  (Issue: [#10](https://www.github.com/PixarAnimationStudios/USD/issues/10))
- Numerous fixes for FindMaya.cmake module. In particular, users on MacOS
  who specify MAYA_LOCATION should now use the root of the installation,
  without the "Maya.app/Contents" suffix.

### USD:
- Added usdtree utility for viewing the scenegraph at the command line. 
  (PR: [#619](https://www.github.com/PixarAnimationStudios/USD/pull/619))
- Added ability to configure default camera prim name via plugin and query
  the default name via UsdUtilsGetPrimaryCameraName.
- Added UsdUtilsTimeCodeRange for representing an iterable range of time codes.
- Added API to UsdGeom for setting and querying Stage-level linear units via
  the "metersPerUnit" metadata field.
- Changed SdfFileFormat interface to operate in terms of SdfLayer instead of
  SdfLayerBase.
- Improved performance of UsdGeomPointBased::ComputeExtent overload with an
  additional transform. 
  (PR: [#640](https://www.github.com/PixarAnimationStudios/USD/pull/640))
- Various cleanup changes to fix documentation, compiler warnings, and remove
  unused or legacy code.
- Numerous changes to UsdSkel schemas for resolving and imaging blend shapes.
- Removed UsdGeomFaceSetAPI schema.
- Removed SdfLayerBase. Its functionality was folded into SdfLayer.
- Fixed bug where parts of the codegen template for multiple-apply API schemas
  were specific to UsdCollectionAPI. 
  (Issue: [#799](https://www.github.com/PixarAnimationStudios/USD/issues/799))
- Fixed bug preventing the use of symlinks for generatedSchema.usda. 
  (Issue: [#763](https://www.github.com/PixarAnimationStudios/USD/issues/763))
- Fixed several bugs in .usdz creation related to nesting of .usdz files and
  file path syntax on Windows.

### Imaging:
- Added "MaterialTags" mechanism which allows the GL backend to render opaque
  primitives separated from translucent primitives.
- Added early implementation of CPU/GPU UsdSkel skinning support, working
  in both GL and Embree backends.
- Added early implementation of blend shapes support to UsdSkelImaging adapter.
- Refactored renderer-independent input/output code into new "hio" library.
- Added new phase for Hydra tasks called "Prepare", which runs after Sync
  phase and before Commit.
- Added "instanceId" and "elementId" AOVs in HdEmbree.
- Added new purpose cache in UsdImaging to improve performance.
- Added support for animated textures in drawModeAdapter. 
  (PR: [#735](https://www.github.com/PixarAnimationStudios/USD/pull/735))
- TaskController::TestIntersections now allows selection by nearestToCamera.
  (PR: [#760](https://www.github.com/PixarAnimationStudios/USD/pull/760))
- Split Hydra's color primvar into a float3 displayColor and a float
  displayOpacity.
- Removed hidden GL dependencies in UsdImaging tests.
- Removed unnecessary parameters from GetInstancerTransform and
  SampleInstancerTransforms in HdSceneDelegate API.
- Fixed several issues with curve drawing in GL backend. 
  (Issue: [#690](https://www.github.com/PixarAnimationStudios/USD/issues/690))
- Fixed bug where UsdGeomSubset called Populate instead of Resync during
  resyncs.
- Fixed leak of empty GlfSimpleShadowArray instance. 
  (PR: [#786](https://www.github.com/PixarAnimationStudios/USD/pull/786))

### Alembic plugin:
- Fixed double time scaling when converting Alembic sample times from seconds
  to USD time codes. 
  (Issue: [#662](https://www.github.com/PixarAnimationStudios/USD/issues/662))
- USD's "timeCodesPerSecond" metadata is now used to scale when reading or
  writing an Alembic file.
- Added support for facesets, which are represented using the UsdGeomSubset
  schema. (PR: [#758](https://www.github.com/PixarAnimationStudios/USD/pull/758))

### Houdini plugin:
- Added support for namespaced primvars in Houdini 17.5+. 
  (PR: [#747](https://www.github.com/PixarAnimationStudios/USD/pull/747))
- Added support for caching in-memory USD stages in GusdStageCache. 
  (PR: [#775](https://www.github.com/PixarAnimationStudios/USD/pull/775))
- Added support for import and export of holeIndices attribute on UsdGeomMesh.
- Added support for all registered USD file extensions.
- Changes in support of upcoming Houdini versions.
- Fixed spurious warnings when evaluating transforms on pseudo-root prims.

### Katana plugin:
- The Katana plugin no longer supports Katana 2.x. Support for 2.x was
  deprecated in release 19.03.
- Expanded support for and use of zero-copy construction of FnAttribute from
  VtArray.
- Ensure that a NullAttribute is used to block an inherited shading connection
  when the USD equivalent is present.
- PxrUsdIn will observe a "katana:useDefaultMotionSamples" USD attribute to
  hard-wire the equivalent behavior of a PxrUsdInDefaultMotionSamples node
  targeting that scope.
- Fixed bug where calling UsdKatanaCache::FindSessionLayer with a cache key
  derived from an empty GroupAttribute would not return a layer whose key
  was generated for an invalid/nonexistent GroupAttribute.
- Fixed crash in PxrUsdIn when an invalid regex value was given for
  "ignoreLayerRegex".

### Maya plugin:
- Added support for exporting units via new "metersPerUnit" metadata.
- Imported shaders now appear in the Hypergraph UI.
- Imported lights now appear in defaultLightSet.

## [19.03] - 2019-02-13

### Added
Build:
- Ability to download and build OpenColorIO dependency via `--opencolorio`
  argument to support new color management functionality in Hydra.

USD:
- Enabled usddumpcrate utility for viewing .usdc file information in the build.
  (PR: [#470](https://www.github.com/PixarAnimationStudios/USD/pull/470))
- Internal payloads, list editing and layer offsets for payload arcs. Payloads
  now provide the same set of features as references.

  This may affect backwards compatibility of layers written with this version
  of USD:
  - Layers containing payloads using these new features will not be readable
    in previous USD versions. .usdc files containing these payloads will be
    marked as version 0.8.0.
  - Layers containing payloads that only use the previously-available features
    will continue to be readable in previous USD versions.
- API for querying authored and inherited primvars in UsdGeomPrimvarsAPI schema.
- Optional 'jointNames' attribute to UsdSkelSkeleton to help DCC import/export.

Imaging:
- HdxColorCorrectionTask to do linear to sRGB conversions or color management
  via OpenColorIO.
- HdxProgressiveTask type tag for tasks that support progressive rendering.
- Initial support for computations in the UsdImaging scene delegate.
- Support for points rendering for basis curves in Hydra GL.
- Publish the normals attribute or primvar from points primitives through 
  Hydra. (PR: [#742](https://www.github.com/PixarAnimationStudios/USD/pull/742))
- usdview now responds to all scene edits from the interpreter. Plugins can
  connect to the usdviewApi.dataModel.signalPrimsChanged signal to be notified
  for these changes.

Houdini plugin:
- Support for more primvar types.

Katana plugin:
- Support for reading materials with multiple incoming shader connections.

Maya plugin:
- Initial work for a UsdPreviewSurface shader node and USD export support.

### Changed
Build:
- build_usd.py now explicitly detects Python version to link against on macOS
  to avoid conflicts between multiple installations. 
  (Issue: [#19](https://www.github.com/PixarAnimationStudios/USD/issues/19), 
   Issue: [#699](https://www.github.com/PixarAnimationStudios/USD/issues/699))
- build_usd.py only builds boost libraries like boost::filesystem when needed.

USD:
- UsdStage::MuteAndUnmuteLayers and LoadAndUnload now send a
  UsdNotice::StageContentsChanged notification. 
  (PR: [#710](https://www.github.com/PixarAnimationStudios/USD/pull/710))

Imaging:
- UsdImaging scene delegate now stops population traversal at non-imageable
  prims.
- Materials are no longer resynced on visibility changes.
- Hydra will now only reset varying state for clean prims.
- HdxCompositor can now copy depth without remapping its range and can also
  copy color and depth to a user-provided viewport size.
- Interacting with the 'Vis' or 'Draw Mode' columns in usdview no longer
  changes selection.
- When multiple prims are selected in usdview, changes in the 'Vis' column
  to one of those prims will affect all of them.

Katana plugin:
- Optimizations for reading gprims and point instancers via a new library,
  vtKatana. This library requires Katana 3.0 or later.

Maya plugin:
- Configuration of the default material scope name has been moved to usdUtils
  to facilitate sharing across plugins. See UsdUtilsGetMaterialsScopeName.

### Deprecated
USD:
- UsdAttribute::HasAuthoredValueOpinion in favor of HasAuthoredValue.
- UsdPrim::ClearPayload, HasPayload, and SetPayload in favor of API on
  UsdPayloads object returned by GetPayloads.
- Primvar API on UsdGeomImageable in favor of UsdGeomPrimvarsAPI schema.

Katana:
- Support for Katana 2.x in favor of Katana 3.x. We anticipate removing support
  for Katana 2.x in the next release.

### Removed
- Dependency on boost::filesystem 
  (Issue: [#679](https://www.github.com/PixarAnimationStudios/USD/issues/679))
- Ability to read pre-xformOp transform attributes on UsdGeomXformable prims.
- UsdShadeMaterial::CreateMaterialFaceSet, GetMaterialFaceSet,
  HasMaterialFaceSet functions.
- UsdGeomCollectionAPI schema.

### Fixed
Build:
- Detection of Visual Studio on non-English platforms in build_usd.py.
  (Issue: [#613](https://www.github.com/PixarAnimationStudios/USD/issues/613), 
   Issue: [#697](https://www.github.com/PixarAnimationStudios/USD/issues/697))

USD:
- Error when composing prims with both direct and ancestral specializes arcs.
- Incorrect strength ordering when composing specializes arcs declared across
  an internal reference arc.
- UsdAttribute::GetConnections and UsdRelationship::GetTargets could return
  incorrect paths in cases involving nested instancing and instance proxies
  inside of masters.
- Corrupted values when parsing OSL string arrays in Sdr via sdrOsl plugin.
- Various fixes for UsdSkelSkinningQuery and UsdSkelAnimMapper.
- Incorrect results from UsdGeomBBoxCache::ComputeRelativeBound.
- UsdGeomPointInstancer enums in Python are now properly scoped under
  UsdGeom.PointInstancer instead of UsdGeom.

Imaging:
- GlfDrawTarget::_BindAttachment could leave a different framebuffer bound
  than what was bound before calling the function.
- Multiple GLSL shader fixes to allow Hydra GL to render correctly with Intel
  GPUs.
- Errors when trying to use usdview on macOS due to incorrect handling of
  unsupported Hydra renderers. When no supported renderers are found, usdview
  will fall back to the legacy renderer.
- Missing symbol exports in UsdVolImaging that could cause build errors on
  Windows. (PR: [#742](https://www.github.com/PixarAnimationStudios/USD/pull/742))
- Regression in usdview that caused the "Redraw On Frame Scrub" option to
  always be in effect. 
  (Issue: [#734](https://www.github.com/PixarAnimationStudios/USD/issues/734))

Alembic plugin:
- Alembic curve basis, type, and wrap were being converted to varying 
  attributes in USD instead of uniform.

Houdini plugin:
- Incorrect display in Tree View panel when unimporting the top-most prim.
- USD Reference ROP behavior when updating or removing an existing reference.
- Various compilation errors with different Houdini versions.
- Incorrect default value for horizontal aperture offset in USD camera node.
- Vertex attributes on meshes with rightHanded winding order. 
  (Issue: [#631](https://www.github.com/PixarAnimationStudios/USD/issues/631), 
  PR: [#632](https://www.github.com/PixarAnimationStudios/USD/pull/632))

Maya plugin:
- Incorrect name conflict error on export when stripping namespaces and merging
  transform and shape nodes. 
  (PR: [#683](https://www.github.com/PixarAnimationStudios/USD/pull/683))

## [19.01] - 2018-12-14

### Added
USD:
- usdresolve utility for checking the results of asset resolution at the 
  command line. (PR: [#609](https://www.github.com/PixarAnimationStudios/USD/pull/609))
- SdfFileFormat::FindAllFileFormats and FindAllFileFormatExtensions
  for querying available file formats. 
  (PR: [#532](https://www.github.com/PixarAnimationStudios/USD/pull/532))
- Option to UsdGeomBBoxCache to ignore visibility.

Imaging:
- Render settings API to HdRenderDelegate. ("Enable Tiny Prim Culling" is the
  first example for Hydra GL)
- Support for UDIM textures in Hydra GL. 
  (PR: [#597](https://www.github.com/PixarAnimationStudios/USD/pull/597))
- Blending controls to HdRenderPassState for initial transparency support in
  Hydra GL.
- Ability to specify backend when starting usdview via "--renderer" parameter.
- Optional "Draw Mode" column to prim browser in usdview that provides control
  of model draw modes. (texture cards, bbox, etc.)

Houdini plugin:
- Optional layer scale, layer offset, and target prim parameters to the
  USD Reference ROP.
- Option on USD Output ROP to use or ignore obj-level transforms.
- Support for exporting ramp parameters in OSL shader networks.

Katana plugin:
- "info.usd.outputSession" attribute authored by PxrUsdIn. This is a sibling
  to "info.usd.session" but is not affected by the translation of PxrUsdIn's
  old "variants" parameter into session data. It acts as a more stable session 
  cache key for external apps to use.
- "forceArrayForSingleValue" parameter that allows PxrUsdInAttributeSet to
  author 1-length arrays.

Maya plugin:
- Support for instancing USD reference assemblies in instancers.

### Changed
Build:
- Symbols for wrapping functions are no longer exported from modules
  to avoid issues with using RTLD_GLOBAL in Python. 
  (Issue: [#641](https://www.github.com/PixarAnimationStudios/USD/issues/641))
- Updated minimum required version of GLEW to 2.0.0.

USD:
- Numerous fixes and cleanup changes throughout to improve performance, 
  remove dead code, convert code to more modern patterns, and remove 
  unneeded uses of boost.
- Replaced safe-bool idiom with explicit bool operator in several places.
- Improved performance of setting and erasing time samples in .usda 
  layers from linear complexity to logarithmic.
- Improved error messages when attempting to read a .usdz files using
  compression or encryption.
- Various improvements to UsdSkel documentation and API.
- Changed scene description name for UsdSkelBindingAPI from "BindingAPI"
  to "SkelBindingAPI" to distinguish it from other binding API schemas.

Imaging:
- Several improvements to the handling of AOVs in Hydra.
- Simplified class hierarchy in usdImagingGL. The primary API entry point
  is now UsdImagingGLEngine.
- Better performance for animated textures and large numbers of invisible
  prims in Hydra.
- Optimized batch removal of prims in Hydra and UsdImaging.
- Better handling of edits made in the interpreter in usdview.
- Mesh normals are suppressed when the subdivision scheme is something
  other than "none".
- The default stb-based image reader now extracts gamma information from
  .png images.
- Functionality specific to the prim browser in usdview have been moved
  to a browser-specific menu bar with "Navigation" and "Show" menus.
- Property browser in usdview now shows resolved material bindings and
  inherited primvars.

 Maya plugin:
- The name of the scope for exported material prims can now be customized 
  in the UI or via the "materialsScopeName" parameter to usdExport. The 
  default scope name is "Looks", matching the previous behavior.
- Refactoring and cleanup in preparation for shading export support.

### Removed
Build:
- Build-time dependency on Python. 
  (Issue: [#605](https://www.github.com/PixarAnimationStudios/USD/issues/605), 
  PR: [#615](https://www.github.com/PixarAnimationStudios/USD/pull/615))

Imaging:
- HdSceneTask in favor of HdTask.
- UsdImagingGL and UsdImagingGLHdEngine.

### Fixed
USD:
- Incorrect composition error in cases involving internal sub-root references 
  and variants. 
  (Issue: [#677](https://www.github.com/PixarAnimationStudios/USD/issues/677))
- Regression that caused prims to sometimes be composed incorrectly.
- Missing symbol exports that could cause build errors on Windows. 
  (PR: [#623](https://www.github.com/PixarAnimationStudios/USD/pull/623), 
   Issue: [#703](https://www.github.com/PixarAnimationStudios/USD/issues/703), 
   Issue: [#704](https://www.github.com/PixarAnimationStudios/USD/issues/704), 
   Issue: [#705](https://www.github.com/PixarAnimationStudios/USD/issues/705))
- Incorrect type validation and conversion when authoring metadata via Python
  that could lead to invalid scene description. 
  (Issue: [#529](https://www.github.com/PixarAnimationStudios/USD/issues/529))
- Various issues in usdzip and related usdUtils API when processing references 
  for packaging into a .usdz file.

Imaging:
- Batch aggregation for prims with face-varying primvars.
- Drawing coord initialization of instance primvar slots.
- Deep batch validation only happens when needed, improving performance.
- Crash in cases where a prim is removed from a stage and a collection
  targeting that prim is updated in the same round of changes. 
  (PR: [#685](https://www.github.com/PixarAnimationStudios/USD/pull/685))
- Crash when reading half-float .exr images in the OpenImageIO plugin.
  (Issue: [#581](https://www.github.com/PixarAnimationStudios/USD/issues/581))
- Incorrect handling of edits to material bindings.

Houdini plugin:
- Issue where visibility and purpose attributes weren't properly inherited 
  on import. 
  (Issue: [#649](https://www.github.com/PixarAnimationStudios/USD/issues/649))
- Crash when running ginfo on a USD file. 
  (Issue: [#673](https://www.github.com/PixarAnimationStudios/USD/issues/673), 
  PR: [#674](https://www.github.com/PixarAnimationStudios/USD/pull/674))
- Issue where imported string attribute values could be incorrect. 
  (Issue: [#653](https://www.github.com/PixarAnimationStudios/USD/issues/653))

Katana plugin:
- Issues with reading UsdGeomPointInstancers
  - Regression in instance transform computation with masked instances.
  - Prototype transforms are cleared out on the Katana side since they are
    folded in when computing instance transforms.

Maya plugin:
- Reference assemblies weren't being drawn in "playback" representation.
  (Issue: [#675](https://www.github.com/PixarAnimationStudios/USD/issues/675))
- Issue where HdImagingShape prevented nodes from being reordered.

## [18.11] - 2018-10-10

### Added
USD:
- [usdVol] UsdVol schema for representing volumes. 
  (PR: [#567](https://www.github.com/PixarAnimationStudios/USD/pull/567))
- [usdShade] Ability to define shaders in the shader definition registry 
   using UsdShade. 
- [usdShaders] Shader definition registry plugin for core nodes defined in 
   the UsdPreviewSurface specification.

Imaging:
- [hd, usdImaging] Hydra support for UsdVol schema. 
  (PR: [#567](https://www.github.com/PixarAnimationStudios/USD/pull/567))
- [hd, hdSt] Topology visibility operations for meshes in Hydra.
- [hd, hdSt] Support for inverting the depth test of a draw target.
- [hd, hdSt] "Flat normals" computed buffer source for meshes.
- [hdx] ResolveNearestToCenter pick hit resolution method for ray picking.

- [usdview] Menu for switching AOVs for supported backends.
- [usdview] Menu for backend-specific settings.

Alembic plugin:
- [usdAbc] Behavior where an Xform with a single geometry or camera in an 
  Alembic file would be collapsed into a single prim in USD can now be disabled 
  by setting the environment variable `USD_ABC_XFORM_PRIM_COLLAPSE` to 0. 
  (PR: [#527](https://www.github.com/PixarAnimationStudios/USD/pull/527))

Maya plugin:
- [usdMaya] Display filter for USD proxy shapes.
- [usdMaya, pxrUsdMayaGL] pxrHdImagingShape node that reduces overhead when 
  imaging scenes with many Hydra-backed shapes.
- [pxrUsdMayaGL] Support for Maya lights casting shadows between Hydra shapes, 
  and Hydra shapes to Maya shapes, but not Maya shapes to Hydra shapes.

Houdini plugin:
- [gusd] Experimental build support for Houdini plugin on Windows.
- [OP_gusd] Multi-seg export to USD output ROP.

### Changed
- Numerous fixes and cleanup changes throughout to improve performance, 
  convert code to more modern patterns, and remove unneeded uses of boost.
  (PR: [#373](https://www.github.com/PixarAnimationStudios/USD/pull/373), 
   PR: [#453](https://www.github.com/PixarAnimationStudios/USD/pull/453), 
   PR: [#481](https://www.github.com/PixarAnimationStudios/USD/pull/481), 
   PR: [#484](https://www.github.com/PixarAnimationStudios/USD/pull/484), 
   PR: [#488](https://www.github.com/PixarAnimationStudios/USD/pull/488))

USD:
- [usd] UsdAPISchemaBase is now an abstract base class as originally intended.
- [usdGeom] Improved performance of UsdGeomPointBased::ComputeExtent. In one 
  example, this provided a ~30% improvement. 
  (PR: [#588](https://www.github.com/PixarAnimationStudios/USD/pull/588))
- [usdGeom] Improved performance of computing extents for UsdGeomPointInstancer.
  In one example with >5 million instances, extent computation time decreased
  from ~25 seconds to ~3 seconds.
- [usdSkel] UsdSkelBakeSkinning bakes skinning at additional time samples
  based on stage's configured sampling rate.
- [usdedit] Opening a .usdz file now forces read-only mode.
- [usdzip] "-l" or "-d" arguments will now provide information about existing 
  .usdz files.
- [usdzip] ARKit compliance checks now require valid sources for shader inputs
  with connections and valid prims and/or collections for material bindings.

Imaging:
- [hd] Improved Hydra AOV API.
- [hd] Split reprs into the topology indices they affect.
- [hd] Properly set up both the shadow projection and view matrices. 
  (PR: [#583](https://www.github.com/PixarAnimationStudios/USD/pull/583))
- [hd] Optimization to not run garbage collection if a texture hasn't change.
- [hd, hdSt, usdImaging] Update wrap mode fallback behavior to useMetadata.
- [hdx] Improved shadow support in HdxTaskController. 
  (PR: [#541](https://www.github.com/PixarAnimationStudios/USD/pull/541))
- [hdx] "Enable Hardware Shading" entry point is now "Enable Scene Materials".
- [hdSt] Support to generate GLSL-compatible names for resources/primvars.
- [usdImagingGL] Better support for animated textures. 
  (PR: [#488](https://www.github.com/PixarAnimationStudios/USD/pull/488))
- [usdview] Better error reporting if an unsupported backend is selected. 
  (PR: [#635](https://www.github.com/PixarAnimationStudios/USD/pull/635))
- [hdEmbree] Improved AO sampling (cosine-weighted sampling, stratified AO 
  samples LHS).

Maya plugin:
- [pxrUsdMayaGL] Many performance improvements in the Hydra batch renderer.
- [usdMaya, pxrUsdMayaGL] Using USD in live surface uses the normal at the 
  snap point.
- [usdMaya, pxrUsdTranslators] Various code cleanup in USD exporter.

Katana plugin:
- [usdKatana] Use bracketing sample closest to shutter open/close if no frames
  found in shutter interval.
- [usdKatana] Honor UsdStage's authored timeCodesPerSecond and framesPerSecond.
  If they differ, Katana will operate in frames, with current time, motion
  samples, and shutter interval scaled appropriately.

Houdini plugin:
- [gusd] Rebind materials when creating sub-root references.
- [gusd] Optimizations in handling of stage mask operations.
- [OP_gusd] USD output ROP now allows binding to shaders referenced from other 
  models.

### Removed

Imaging:
- [hd] Removed Repr requirements from GetRenderTag API.

### Fixed
Build:
- [cmake] User-specified CMAKE_MODULE_PATH was being ignored during build. 
  (PR: [#614](https://www.github.com/PixarAnimationStudios/USD/pull/614))
- [cmake] Installed headers could take precedence over source headers, leading 
  to build failures. 
  (Issue: [#83](https://www.github.com/PixarAnimationStudios/USD/issues/83)) 

USD:
- [tf] TfEnvSetting crash on MSVC 2017.
- [ar] ArDefaultResolver::AnchorRelativePath did not call IsRelativePath on
  derived resolvers. 
  (PR: [#426](https://www.github.com/PixarAnimationStudios/USD/pull/426))
- [sdf] SdfCopySpec did not handle copying prim specs to variants and vice-versa.
- [sdf] Incorrect notification was sent when removing subtrees of inert specs.
- [usd] Prim payload was not automatically loaded when a deactivated ancestor 
  prim with a payload was activated. 
  (Issue: [#604](https://www.github.com/PixarAnimationStudios/USD/issues/604))
- [usd] Incorrect lists of time samples would be returned in certain cases
  with value clips.
- [usd, usdzip] .usdz files were not fully compliant with the zip file format 
  specification. 
- [usdUtils, usdzip] Creating an ARKit .usdz file with a .usda asset would 
  create an invalid file.
- [usdMtlx] MaterialX plugin build failures on Windows and macOS. 
  (Issue: [#611](https://www.github.com/PixarAnimationStudios/USD/issues/611))

Imaging:
- [hdx] Picking with disabled scene materials generating shader errors.
- [hdSt] Fix immediate draw batch invalidation on buffer migration.
- [hdSt] Fix garbage collection on buffer migration.
- [hdSt] Bindless texture loading not being deferred.
- [usdImaging] Wrong transforms were being used when using usdLux light 
  transforms 
  (Issue: [#612](https://www.github.com/PixarAnimationStudios/USD/issues/612)).
- [usdImaging] Fix for usdImaging's TextureId not accounting for origin.

Houdini plugin:
- [gusd] Several fixes for building with Houdini 17. 
  (PR: [#549](https://www.github.com/PixarAnimationStudios/USD/pull/549))
- [gusd] Several fixes for stale caches and crashes.
- [gusd] Potential PackedUSD transform issues in Houdini 16.5.

## [18.09] - 2018-08-07

This release includes several major new features and changes, including:
  - .usdz file format for packaging assets into a single file
  - Introduction of UsdPreviewSurface
  - Switch to calendar-based versioning scheme for USD libraries

### Added
- Build:
  - build_usd.py can be given custom arguments for building libraries via the
    `--build-args` option.
  - Option to disable building usdview by specifying `PXR_BUILD_USDVIEW=OFF` 
    when running cmake or `--no-usdview` when running build_usd.py.
  - Allow specifying `Boost_USE_STATIC_LIBS` to cmake on Windows. Note that 
    static boost libraries may lead to issues with Python bindings. 
    (Issue: [#407](https://www.github.com/PixarAnimationStudios/USD/issues/407))

- USD:
  - ArAsset and ArResolver::OpenAsset interfaces allowing resolvers to control
    how data for a given asset is accessed.
  - "Package" asset and layer concepts to Ar and Sdf, including 
    "package-relative" asset path syntax for addressing assets within packages.
  - sdffilter utility for inspecting and summarizing the contents of a layer.
  - .usdz file format. This format allows multiple assets (including layers, 
    textures, etc.) to be packaged into a single file that can be consumed by
    USD without being unpacked to disk. These files can be created via the 
    "usdzip" command-line utility or via APIs like UsdZipFileWriter and 
    UsdUtilsCreateNewUsdzPackage.
  - UsdStage::GetObjectAtPath for retrieving a generic UsdObject. 
    (PR: [#390](https://www.github.com/PixarAnimationStudios/USD/pull/390))
  - UsdCollectionAPI can represent collections that include all paths or exclude
    some paths but include all others via the new "includeRoot" attribute.
  - Support for "sourceAsset" and "sourceCode" implementation sources in 
    UsdShadeShader.
  - Ndr and Sdr libraries that provide a registry for shader definitions that 
    can be extended via plugins.
  - sdrOsl plugin for populating shader definition registry from OSL 1.8.12. 
    OSL support must be enabled by specifying `PXR_ENABLE_OSL_SUPPORT=TRUE` 
    when running cmake.
  - usdMtlx plugin containing USD file format and shader definition registry 
    plugins based on MaterialX 1.36.0. MaterialX support must be enabled by
    specifying `PXR_BUILD_MATERIALX_PLUGIN=TRUE` when running cmake.
  - Initial UsdSkel schema for blend shapes, which are not yet factored into
    computed deformation.

- Imaging:
  - Per-prim adaptive refinement enabled via materials in hdSt.
  - Preliminary AOV support in Hydra with sample implementation in Embree 
    plugin.
  - HdRenderThread utility class to enable render delegates to render in 
    background threads. The Embree plugin uses this class as an example.
  - Support for scene-authored normals on meshes.

- UsdImaging:
  - Light linking support.
  - Initial support for USD material preview nodes. For more details, see:
    http://graphics.pixar.com/usd/docs/UsdPreviewSurface-Proposal.html 

    Currently unimplemented features include:
    - Support for automatically computed tangents when normal mapping. Primvars
      must be supplied.
    - Handling for ior (index or refraction).

- Alembic plugin:
  - Support for TextureCoordinate role and value types.

- Maya plugin:
  - Initial support for exporting .usdz files.
  - Support for importing and exporting volume and displacement in "pxrRis"
    shading mode.
  - pxrUsdPointBasedDeformerNode - an animation cache deformer that allows users
    to pose a Maya mesh based on a UsdGeomPointBased prim.
  - Option to apply Euler filtering to transforms during export. 
    (PR: [#299](https://www.github.com/PixarAnimationStudios/USD/pull/299))
  - Option to filter out certain Maya node types during export. 
    (PR: [#475](https://www.github.com/PixarAnimationStudios/USD/pull/475))

- Katana plugin:
  - Support for TextureCoordinate role and value types.
  - Support for importing inherit paths metadata as a info.usd group attribute
  - "additionalLocations" parameter to PxrUsdInVariantSelect
  - "asArchive" option to PxrUsdIn. When enabled, a "usd archive" location will 
    be created rather than loading the USD data directly. This is intended for 
    passing along to renderers which support reading USD natively.
  - PxrUsdIn supports a "sharedLooksCacheKey" attribute to permit sharing cached
    USD shading information across multiple references/instances which are known
    to be identical from a pipeline standpoint.

- Houdini plugin:
  - Support import of Scope prims.

### Changed
- Build:
  - USD resource files are now installed into <prefix>/lib/usd instead of
    <prefix>/share/usd/plugins to bring them closer to the installed libraries.
  - OpenGL dependency in imaging is now optional and may be disabled by 
    specifying `PXR_ENABLE_GL_SUPPORT=FALSE` when running cmake. This will 
    disable all GL-based functionality, including usdview.
  - OpenImageIO dependency imaging is now optional and is disabled by default. 
    Support must be enabled by specifying `PXR_BUILD_OPENIMAGEIO_PLUGIN=TRUE`
    when running CMake or `--openimageio` when running build_usd.py.
  - build_usd.py on Windows will now use powershell to download dependencies
    if it's available to avoid TLS 1.2 issues. 
    (Issue: [#449](https://www.github.com/PixarAnimationStudios/USD/issues/449))
  - build_usd.py now directs users to install PySide2 via pip on MacOS.

- USD:
  - Numerous fixes and cleanup changes throughout to improve performance, 
    convert code to more modern patterns, and remove unneeded uses of boost.
  - ArGetResolver no longer returns an instance of the ArResolver subclass used
    for asset resolution. Clients can use ArGetUnderlyingResolver in the special
    cases where access to the exact subclass is necessary.
  - ArResolver context and scoped cache functions are now public and have been
    renamed to match convention. Resolvers that override these functions will 
    need to be updated.
  - ArDefaultResolver default context for an asset now includes the directory
    of the asset in its search path.
  - Optimizations to prim change processing in UsdStage. In one example, 
    processing time for adding new prims decreased ~50%.
  - New .usdc files now default to version 0.7.0, which includes compression
    features introduced in earlier releases. These files cannot be read in USD
    releases prior to v0.8.4. Users can revert to writing older versions by
    setting the environment variable `USD_WRITE_NEW_USDC_FILES_AS_VERSION`
    to an older version.
  - Several changes to provide cleaner and more detailed runtime error messages.
  - Better support for generating code for properties of multiple-apply API
    schemas in usdGenSchema.
  - Schema types are now specified using the UsdSchemaType enum instead of 
    individual flags.
  - "expansionRule" attribute for UsdCollectionAPI is now optional; collections
    are created simply by calling UsdCollectionAPI::ApplyCollection.
  - UsdGeomBBoxCache's bounds computation now includes all defined prims,
    including those with no type specified.
  - UsdRiMaterialAPI now writes standardized "ri:surface" output by default 
    instead of deprecated "ri:bxdf".
  - UsdRiStatementsAPI now writes ri attributes as primvars by default.
  - Renamed UsdSkelPackedJointAnimation schema to UsdSkelAnimation
  - Renamed UsdSkelBakingSkinningLBS function to UsdSkelBakeSkinning
  - Numerous additional fixes and changes to UsdSkel schemas and APIs.

- Imaging:
  - Several API cleanup changes in HdSprim/HdBprim/HdRprim.
  - HdRenderDelegate::CanComputeMaterialNetworks has been replaced by
    GetMaterialBindingPurpose, which has a default value of "preview".
  - Improved handling of fallback texturing behavior when texture inputs are
    missing from a material.
  - .png, .jpg, .bmp, .tga, and .hdr images are now handled by a built-in
    image reader based on stb_image.
  - Texture loading is deferred until needed.
  - Truncate mesh vertex primvar data to expected length if larger than expected.
  - Improved ptex rect packing performance. In one large test case, processing
    time decreased from ~600s to ~40s.

- UsdImaging:
  - Avoid loading custom shaders and textures when the "Enable Hardware Shading"
    option is disabled in usdview.
  - Default values for wrapS/wrapT match the proposed value in the preview
    material spec. Also adds support for mirrored wrapS/wrapT.
  - Scene lights are now enabled by default. This can be disabled by setting
    the environment variable `USDIMAGING_ENABLE_SCENE_LIGHTS` to 0.
  - UsdShadeMaterialAPI is now used instead of UsdRi to look up material 
    networks.

- Alembic plugin:
  - Alembic writer now prefers writing to the "st" attribute instead of "uv". 
    (PR: [#463](https://www.github.com/PixarAnimationStudios/USD/pull/463))

- Maya plugin:
  - MAYA_SCRIPT_PATH and XBMLANGPATH environment variable settings have changed
    due to new resource file install location documented above. Refer to Maya
    plugin documentation for new values.
  - Large refactoring and cleanup of usdMaya. USD importers and exporters for
    built-in Maya nodes have been moved to pxrUsdTranslators plugin.
  - Proxy shape now redraws in response to changes on UsdStage.
  - Diagnostic messages are now batched together for better reporting behavior
    and to prevent log spewage.
  - Improved primvar export. 
    (PR: [#330](https://www.github.com/PixarAnimationStudios/USD/pull/330))
  - Camera shake now applied to USD cameras on export. 
    (PR: [#366](https://www.github.com/PixarAnimationStudios/USD/pull/366))

### Deprecated
- USD:
  - UsdSkelPackedJointAnimation schema in favor of UsdSkelAnimation
  - Specialized RIS and RSL shader schemas in UsdRi (e.g. UsdRisPattern and 
    UsdRisBxdf) in favor of generic UsdShadeShader prims.

### Removed
- Build:
  - boost::regex dependency.

- USD:
  - ArResolver::CreateDefaultContextForDirectory to better accommodate asset
    systems that are not file-system based.
  - SdfExtractExternalReferences. UsdUtilsExtractExternalReferences can be used
    as an alternative.
  - UsdLuxLinkingAPI, in favor of UsdCollectionAPI to represent light linking.

### Fixed
- Build:
  - Fixed issues with using Visual Studio 2017 in build_usd.py.
  - Issue preventing use of NMake generator on Windows. 
    (PR: [#519](https://www.github.com/PixarAnimationStudios/USD/pull/519))
  - Several fixes for static library builds.

- USD:
  - Bug where SdfLayer::GetDisplayName would return an empty string for 
    anonymous layers if the tag contained a ":". 
    (PR: [#440](https://www.github.com/PixarAnimationStudios/USD/pull/440))
  - Bug where SdfLayer::UpdateExternalReferences would not handle variants and 
    payloads correctly.
  - Incorrect load state handling in UsdStage for payloads introduced across 
    sub-root references, inherits, and specializes arcs.
  - Incorrect value clip time mapping during value resolution on UsdStage.
  - Change processing bug when adding a new inert prim spec.
  - Load/unload change processing bug with nested instances on UsdStage.
  - Crash when opening a UsdStage with a UsdStagePopulationMask containing 
    instance prims. 
    (Issue: [#497](https://www.github.com/PixarAnimationStudios/USD/issues/497))
  - Test failures when USD_EDITOR environment variable is set. 
    (Issue: [#505](https://www.github.com/PixarAnimationStudios/USD/issues/505))
  - Bug where non-constant primvars were inherited down namespace in
    UsdGeomPrimvarsAPI.

- Imaging:
  - Triangulation/quadrangulation of face varying for refined meshes.
  - Several selection and picking issues.

- UsdImaging:
  - Material binding to instanced prims.
  - Change processing for UsdShadeShader prims beneath UsdShadeMaterial prims.

- Alembic plugin:
  - Texture coordinate indices are now preserved. 
  (PR: [#520](https://www.github.com/PixarAnimationStudios/USD/pull/520))

- Katana plugin:
  - Several asset resolution fixes. 
    (Issue: [#535](https://www.github.com/PixarAnimationStudios/USD/issues/535))
  - Bug preventing correct inheritance of motion sample times overrides as
    authored by PxrUsdInDefaultMotionSamples and PxrUsdInMotionOverrides in
    Katana plugin.

- Houdini plugin:
  - Fixed several display and update issues with treeview panel.
  - Fixed bug in batched loading of masked prims.

## [0.8.5a] - 2018-05-21

### Fixed
- Fixed broken URL in build_usd.py for downloading libtiff. 
  (Issue: [#498](https://www.github.com/PixarAnimationStudios/USD/issues/498))

## [0.8.5] - 2018-05-07

### Added
- New 'trace' library providing performance profiling functionality.
- API to Ar for resolver implementations that wrap around other resolvers.
- TextureCoordinate role and TexCoord{2,3}{h,f,d}{Array} value types to Sdf 
  to indicate attributes that represent UV(W) texture coordinates.
- UsdUtilsSparseValueWriter and UsdUtilsSparseAttrValueWriter utility classes
  for authoring attribute values sparsely.
- UsdGeomPrimvar::GetIndicesAttr API for indexed primvars.
- UsdGeomPrimvarsAPI schema for accessing primvars, including primvar
  values inherited down namespace. This is intended as an eventual replacement 
  for the primvar API on UsdGeomImageable.
- API to UsdShadeMaterial to support render context-specific terminal outputs 
  and core support for three universal render context outputs: surface, 
  displacement, and volume.
- Time-sampling support for UsdGeomPointInstancer via new methods for
  computing extents and instance transforms at multiple times.
- Finalize method for sprims and bprims in Hydra.
- Time-sampling support in Hydra for geometry instancing.
- Maya plugin users can specify if colors coming from Maya are linear via the
  `PIXMAYA_LINEAR_COLORS` environment variable.
- Support for import/export with new TextureCoordinate role in Maya plugin.
- Support for cards drawMode in USD reference assembly in Maya plugin.
- Support for LiveSurface with proxy shapes in Maya plugin.
- Support for importing UsdSkel schemas and exporting locators and
  particles in Maya plugin.
- Option for placing exported data in parent scope in Maya plugin.
- Initial support for session layer metadata in PxrUsdIn in Katana plugin.
- Support for reading in UsdLuxCylinderLight schemas in Katana plugin.
- Gusd Python bindings for Houdini plugin.

### Changed
- USD now requires TBB version 4.4 Update 6 or later.
- Removed GLUT dependency for libtiff in build_usd.py. 
  (Issue: [#402](https://www.github.com/PixarAnimationStudios/USD/issues/402))
- build_usd.py will now use cURL to download dependencies if it's
  installed in the user's PATH. This can help avoid TLS v1.2 errors
  when downloading from certain sites. 
  (Issue: [#449](https://www.github.com/PixarAnimationStudios/USD/issues/449))
- Numerous documentation additions and fixes throughout the codebase.
- Improved performance of SdfCopySpec and UsdUtils stitching API; one test
  case showed a 35% speed improvement, from 160s to 118s.
- SdfLayer now uses the resolved path provided by Ar for a given identifier
  to determine the layer file format. 
  (Issue: [#144](https://www.github.com/PixarAnimationStudios/USD/issues/144))
- Simplified API for setting layer data in SdfFileFormat subclasses.
- Adding or removing invalid sublayers now results in composition errors.
- UsdStage::CreateNew and CreateInMemory now accept an InitialLoadSet argument.
  This controls whether new payloads are loaded automatically. 
  (Issue: [#267](https://www.github.com/PixarAnimationStudios/USD/issues/267))
- UsdStage::ExpandPopulationMask now considers attribute connections.
- UsdStage::Flatten now authors anchored asset paths in flattened results.
- UsdTraverseInstanceProxies() now includes instance proxy prims that pass the
  default predicate instead of all prims.
- .usdc files now support zero-copy array access, which can significantly
  improve performance. This feature is supported by all .usdc file
  versions, but may not be activated for certain files depending on their
  data alignment. In these cases, users can simply re-export the file 
  to activate zero-copy access.

  Users can set the environment variable `USDC_ENABLE_ZERO_COPY_ARRAYS` to 0 
  to disable this feature. Users can also track cases where array data is 
  copied due to modifications by setting the environment variable
  `VT_LOG_STACK_ON_ARRAY_DETACH_COPY` to 1.
- Other performance improvements in UsdStage composition.
- API schemas are now classified as either "applied" or "non-applied". See
  "Generating New Schema Classes" tutorial for more information. 
- Behavior of UsdUtils stitching API can now be customized via callbacks.
- UsdShadeMaterialBindingAPI now issues a warning when computing resolved
  material bindings if prims with old "look:binding" relationships are found.
  This can be disabled by setting the environment variable 
  `USD_SHADE_WARN_ON_LOOK_BINDING` to 0.
- UsdRiMaterialAPI now supports writing ri:surface outputs. ri:bxdf outputs are
  still written by default, but this can be disabled by setting the environment
  variable `USD_RI_WRITE_BXDF_OUTPUT` to 0.
- UsdRiStatementsAPI now supports encoding ri attibutes as primvars. This is
  disabled by default but can be enabled by setting the environment variable 
  `USDRI_STATEMENTS_WRITE_NEW_ATTR_ENCODING` to 1.
- Additions and improvements to UsdLux and UsdSkel schemas.
- Hydra backends that consume full network materials now receive all primvars
  for all rprims, not just those with bound materials.
- Several improvements to primvar handling and picking infrastructure in Hydra.
- Several performance improvements in Hydra, especially for material bindings
  and scenes with heavy nesting of instances.
- Ongoing work on Hydra compute framework.
- Conformed Hydra API to use "Primvar" instead of "PrimVar".
- HdMaterialNetwork is now emitted in topological order.
- Redundant animation data is no longer written during Maya export.
- Katana plugin now allows parent scope names for materials other than "Looks".
  This can be enabled by setting the `USD_KATANA_ALLOW_CUSTOM_MATERIAL_SCOPES`
  environment variable to 1.
- Refactor material loading in Katana plugin so that material locations aren't 
  computed all at once and site ops can run on each location individually.
- Improved construction of material terminal outputs in Katana plugin.
- Caching improvements in Houdini plugin to share USD stages between loaded
  primitives when possible.
- Improved error handling and reporting in Houdini plugin.
- USD stage masking in Houdini plugin can now be disabled via 
  `GUSD_STAGEMASK_ENABLE` environment variable.
- Improved performance when opening USD stages which don't use "component" kind
  in their model hierarchy.
- Object-level transforms are now written at a higher-level scope than leaf
  primitives if possible in Houdini plugin.

### Deprecated
- UsdGeomFaceSetAPI in favor of UsdGeomSubset.
- UsdGeomCollectionAPI in favor of UsdCollectionAPI.

### Removed
- tracelite library, which has been replaced with the trace library.
- Conversions for Python's datetime and dependency on boost::date_time.
- Several unused classes and functions.
- usdHydra schemas. This functionality is being replaced with by a registry
  of shaders in a future release.
- API for ri:bxdf output on UsdRiMaterialAPI schema. ri:bxdf sources will still
  be returned by UsdRiMaterialAPI::GetSurface for backwards compatibility.

### Fixed
- Build errors when using ninja.
- Error when extracting boost on Windows in build_usd.py. 
  (Issue: [#308](https://www.github.com/PixarAnimationStudios/USD/issues/308))
- The build now prefers Alembic library specified at cmake time over any
  Alembic library found in PATH. 
  (Issue: [#409](https://www.github.com/PixarAnimationStudios/USD/issues/409))
- Several compile-time warnings on clang and other compilers.
- Bug where list-op valued metadata was not emitted by UsdStage::Flatten.
- Missing notifications for master prims affected by metadata/property changes.
- Crash in UsdCollectionAPI::ApplyCollection when given an invalid collection 
  name. (Issue: [#425](https://www.github.com/PixarAnimationStudios/USD/issues/425))
- Change processing for changes to drawMode property in imaging.
- Numerous fixes to cards draw mode.
- Change processing bug when removing nested point instancers.
- Crashes in pxOsd due to incorrect authored crease data.
- Bug where usdview would not redraw after switching renderer.
- Crash in Alembic plugin when reading files with object names beginning 
  with numerals.
- Change processing bug when reloading an Alembic layer. 
  (Issue: [#429](https://www.github.com/PixarAnimationStudios/USD/issues/429))
- Several crash bugs in Maya plugin.
- Material export bug in Maya plugin where the surface terminal was not
  being exported under the "ri" render context. This fix requires re-exporting
  any material networks exported from Maya using version 0.8.4.
- Prevent overwriting instance sources that resolve to the same master name
  in Katana plugin.
- Path resolution issue in Houdini plugin.
- Miscellaneous bug fixes in Houdini plugin.

## [0.8.4] - 2018-03-05

### Added
- The PXR_PLUGINPATH_NAME environment variable name may be changed by
  specifying `PXR_OVERRIDE_PLUGINPATH_NAME=<name>` when running cmake.
- Example sendmail plugin for usdview, located at
  extras/usd/examples/usdviewPlugins/sendMail.py
- ArDefaultResolverContext, a context object for the ArDefaultResolver asset
  resolution implementation that allows additional search paths to be used
  during asset resolution.
- Users can now query the UsdNotice::ObjectsChanged notice for the changed
  scene description fields that affected the reported objects.
- UsdAPISchemaBase base class for all API schemas.
- All UsdGeomBoundable schemas in usdGeom now have functions for computing
  extents. These functions are also used when calling the general 
  UsdGeomBoundable::ComputeExtentFromPlugins method.
- UsdLuxCylinderLight schema.
- Significant additions to edge and point selection and highlighting 
  capabilities in Hydra.
- Initial support for UsdSkel bones in usdImaging.
- Initial support for exporting joints and skin clusters using the UsdSkel
  schema in the Maya plugin.
- Documentation for third party plugin code is now included in the
  doxygen build.

### Changed
- build_usd.py no longer checks for pyside-uic or boost::python if Python
  support is disabled, and no longer builds OpenImageIO's Python bindings.
- Updated moduleDeps.cpp files to only register direct library dependencies.
  This makes it easier for users to generate their own file for custom schemas.
- ArDefaultResolver now allows search paths like "Dir/File.usd" to be anchored 
  to other paths via AnchorRelativePath. During composition, these asset paths 
  will be resolved relative to the layer where they were authored before
  falling back to the previous search path behavior.
- Updates to VtArray and .usdc code in preparation for zero-copy functionality.
- Inherit and specializes arcs to non-existent prims are no longer considered
  composition errors.
- Apply method on API schemas have been moved to UsdAPISchemaBase and now
  require a UsdPrim. Also improved documentation.
- Property queries on UsdPrim now accept a predicate for filtering results.
- UsdPrim::HasAPI now accepts an instance name argument to query if a prim has
  a particular instance of a multiple-apply API schema has been applied.
- Adding or removing an inert prim spec no longer causes affected prims to
  be resynced. These prims are now reported as "changed info only" in the
  corresponding UsdNotice::ObjectsChanged notice.
- UsdNotice::ObjectsChanged::GetResyncedPaths and GetChangedInfoOnlyPaths now
  return a custom range object instead of a SdfPathVector.
- Performance optimizations for querying properties on UsdPrim.
- Replaced UsdCollectionAPI::AddPrim/RemovePrim with IncludePath/ExcludePath.
- UsdGeomBoundable::ComputeExtentFromPlugins now accepts an optional 
  transform matrix, which may be used to provide more accurate bounds.
- UsdGeomBBoxCache now computes extents for all UsdGeomBoundable schemas.
- Performance optimizations in UsdShadeMaterialBindingAPI.
- Numerous changes and fixes to UsdSkel schemas.
- Significantly improved curve rendering in Hydra.
- Many improvements towards the goal of getting modern UsdShade materials
  through Hydra to various kinds of backends.
- Performance improvements to hydra gather phase via multi-threading and other
  optimizations.
- Changed complexity options in usdview to prevent users from inadvertently
  bumping the complexity value too high and hanging the application.
- Several tweaks and improvements to usdview UI.
- Refactored Maya/Hydra batch renderer to improve performance for imaging USD
  proxy shape nodes.

### Removed
- UsdShadeLook schema. This has been replaced by UsdShadeMaterial.
  Material bindings authored using the "look:binding" relationship are no
  longer respected.

### Fixed
- Various typo and compiler warning fixes throughout the codebase.
- Fixed bug where build_usd.py would not use the CMake generator specified at
  the command line.
- Fixed crash in Apply method on API schemas. 
- Fixed several bugs in UsdShadeMaterialBindingAPI::ComputeBoundMaterial.
- Changing the population mask for a UsdStage now correctly releases resources
  used by objects that have been excluded from the stage.
- Fixed quadrangulation bug in Hydra with handling topology with degenerate or 
  hole faces.
- Fixed patch param refinement for Loop meshes.
- Several fixes to the nascent Hydra lights pipeline.
- Fixed bug in the usdExport AlembicChaser in the Maya plugin where primvars 
  that match the primvarprefix do not get exported. They are now exported with 
  constant interpolation, and using _AbcGeomScope is no longer required.
- Fixed bug in Katana plugin where infinite recursion would occur in pxrUsdIn
  when sources were outside the scope of the point instancer. 
  (Issue: [#286](https://www.github.com/PixarAnimationStudios/USD/issues/286))

## [0.8.3] - 2018-02-05

### Added
- Compression in .usdc files for integer arrays and scalar floating point 
  arrays. The latter are compressed if the values are all integers or there 
  are a small number of unique values. In the example Kitchen Set asset on 
  the USD website, the total size of the geometry layers decreased by ~46%, 
  from 25 MB to 14 MB.

  .usdc files with this new compression enabled are marked as version 0.6.0
  and are not readable by earlier releases. These files are not written by
  default; this may be enabled by setting the environment variable 
  `USD_WRITE_NEW_USDC_FILES_AS_VERSION` to "0.6.0".
- Ability to record and query API schemas that have been applied to a prim
  via new Apply method on API schema classes, UsdPrim::GetAppliedSchemas and
  UsdPrim::HasAPI. Custom API schemas should be updated with these new
  methods by re-running usdGenSchema.
- GetUnionedTimeSamples and GetUnionedTimeSamplesInInterval functions for
  UsdAttribute and UsdAttributeQuery.
- Ability to offset time for active value clips when using template clip 
  metadata via "templateActiveOffset" entry.
- UsdUtilsGetDirtyLayers for retrieving dirty layers used by a UsdStage.
- GetTimeSamplesInInterval functions for UsdGeomXformOp, UsdGeomXformable 
  and UsdGeomXformable::XformQuery.
- UsdShadeMaterialBindingAPI, which provides an interface for binding 
  materials to prims or collections of prims and computing the final bound
  material for a prim via "material resolution".
- Numerous features and documentation for UsdSkel schema.
- "Save Flattened As" functionality in usdview.
- Plugin mechanism in usdview that allows users to add custom commands 
  and menus. See new tutorial for more details.
- Partial support for RenderMan for Maya lights in the Maya plugin.
- PxrUsdIn.BootstrapMaterialGroup op in Katana plugin for more robustly reading 
  a Looks scope from a .usd file

### Changed
- Build now supports versioned OpenEXR and IlmBase shared libraries. 
  (Issue: [#71](https://www.github.com/PixarAnimationStudios/USD/issues/71))
- Layer identifiers may now include '?' characters. 
  (Issue: [#289](https://www.github.com/PixarAnimationStudios/USD/issues/289))
- UsdStage preserves payload load state when processing instancing changes.
- UsdListPosition enum values now specify the "append" or "prepend" list as 
  well as a position to provide users with finer-grained control.
- The various Add... methods in Usd that take a UsdListPosition argument
  now author entries to the back of the "prepend" list by default if no
  "explicit" list exists. The old behavior of authoring to the (now 
  deprecated) "added" list can be restored by setting the environment
  variable `USD_AUTHOR_OLD_STYLE_ADD` to 1.
- Standard schema conventions are more strictly-enforced in usdGenSchema.
- UsdCollectionAPI::AddCollection has been renamed ApplyCollection.
- Enabled authoring of new UsdShade encoding by default. Authoring the old
  (now deprecated) encoding can be restored by setting the environment variable 
  `USD_SHADE_WRITE_NEW_ENCODING` to 0.
- The "joints" relationship on the UsdSkelSkeleton, UsdSkelPackedJointAnimation,
  and UsdSkelBinding API schemas is now a token array-valued attribute.
- UsdRiStatements API schema has been renamed to UsdRiStatementsAPI.
- sdfdump utility now shows all specs in a layer, even if they have no fields.
- Various improvements to Hydra reprs and geometry processing.
- More improvements to Hydra's handling of invalid data.
- Ongoing work to prepare Hydra to fully consume UsdShade schemas.
- Refactored GL dependency out of Hd library.
- Built-in variables in usdview interpreter are now accessed through a separate
  usdviewApi object to avoid name collisions.
- Performance improvements in Maya plugin when in Viewport 2.0.
- Inclusion of info.usd.opArgs in Katana plugin is now parameterized; it will
  be authored to the location where a "setOpArgsToInfo" attribute exists and is
  set to 1.

### Deprecated
- The "added" list for list ops in scene description is deprecated in favor 
  of the "prepend" and "append" lists.

### Removed
- GfCamera::ZUp and GfCamera::YUp.
- UsdSkelJoint schema.

### Fixed
- build_usd.py ensures OpenImageIO build does not pick up OpenEXR from other 
  locations, which could have led to runtime errors. 
  (Issue: [#315](https://www.github.com/PixarAnimationStudios/USD/issues/315), 
  Issue: [#325](https://www.github.com/PixarAnimationStudios/USD/issues/325))
- Headers are now installed properly for monolithic builds. 
  (Issue: [#277](https://www.github.com/PixarAnimationStudios/USD/issues/277))
- Original install location will no longer be searched for plugins after
  relocating builds. 
  (Issue: [#363](https://www.github.com/PixarAnimationStudios/USD/issues/363))
- Fixed thread-safety issue where plugins with the same name but in different
  locations could be loaded twice. 
  (Issue: [#358](https://www.github.com/PixarAnimationStudios/USD/issues/358))
- Fixed bug where layers that were muted via SdfLayer::AddToMutedLayers before
  they were first opened could not be unmuted.
- Fixed bug in usdGenSchema where changing an existing property's type in a 
  schema would not be reflected in the generated code.
- Fixed bug where a large (> 1460) number of variants in a .usda file would
  cause a "memory exhausted" error when parsing that file.
- Fixed broken pread mode for .usdc files.
- Fixed bug that caused UsdStage::CreateNew to crash on Windows. 
  (Issue: [#364](https://www.github.com/PixarAnimationStudios/USD/issues/364))
- Fixed bug when using a UsdStagePopulationMask with prims beneath instances. 
  (Issue: [#312](https://www.github.com/PixarAnimationStudios/USD/issues/312))
- Fixed bug where setting float-valued attributes to +inf in Python would fail.
- UsdAttribute::GetTimeSamplesInInterval now properly accounts for layer 
  offsets. (Issue: [#352](https://www.github.com/PixarAnimationStudios/USD/issues/352))
- Internal references or empty asset paths no longer cause errors in 
  UsdUtilsFlattenLayerStack.
- Fixed bug where UsdGeomPrimvar::GetTimeSamples would miss time samples for
  indexed primvars.
- Disabled tiny prim culling in Hydra by default. It can be re-enabled by
  setting the environment variable `HD_ENABLE_TINY_PRIM_CULLING` to 1.
  (Issue: [#314](https://www.github.com/PixarAnimationStudios/USD/issues/314))
- Fixed issues with using non-file-backed asset paths in various utilities
  and Maya and Katana plugins.
- Houdini plugin explicitly links against required libraries to avoid runtime
  errors with Houdini 16.5.
- Fixed several bugs with point instancing support in Houdini plugin.

## [0.8.2] - 2017-12-01

Release 0.8.2 increments the file format version for .usdc files. New .usdc
files created in this release will not be readable by earlier releases. See
below for more details.

### Added
- SdfCopySpec for copying scene description specs in layers.
- Usd.GetVersion() for retrieving USD version in Python 
  (Issue: [#306](https://www.github.com/PixarAnimationStudios/USD/issues/306))
- UsdProperty::FlattenTo for copying a UsdProperty's resolved metadata/values.
- IsTyped and IsConcrete schema queries on UsdSchemaRegistry and UsdSchemaBase. 
- UsdCollectionAPI schema for representing collections of objects. This schema
  is more expressive than UsdGeomCollectionAPI and can represent large numbers
  of objects compactly.
- Utility functions in UsdUtils for determining compact representations of
  objects for UsdCollectionAPI.
- UsdUtilsCoalescingDiagnosticDelegate, a Tf diagnostic delegate that provides
  condensed output for diagnostic messages.
- UsdUtilsFlattenLayerStack for flattening a UsdStage's layer stack into a
  single layer, as well as a --flattenLayerStack option to usdcat.
- Support for named clip sets for clip stitching API in UsdUtils.
- Schema for cards rendering in UsdGeomModelAPI and imaging support. This
  provides a lightweight way to collapse model hierarchies into simple
  geometry for visualization of large scenes.
- Ability to encode familyType on owners of families of UsdGeomSubsets.
- Size properties for UsdLuxDiskLight, UsdLuxSphereLight, and UsdLuxRectLight. 
  (Issue: [#290](https://www.github.com/PixarAnimationStudios/USD/issues/290))
- Support for computing bounding boxes for UsdGeomPointInstancer instances 
  using UsdGeomBBoxCache.
- Support for residual GPU computations in Hydra.
- Sheer display mode in Hydra, a 'see-through' display mode akin to the
  mesh being covered in a sheer mesh fabric instead of its regular surface.
  Implemented as a regular grid stipple pattern of the surface at 20% opacity.
- Testing framework for usdview.
- PxrUsdMayaXformStack for representing transform stacks in the Maya plugin.
- Support for exporting Maya instancers to UsdGeomPointInstancers in the Maya
  plugin.
- Support for exporting groups of objects with the same material binding as
  collections encoded with UsdCollectionAPI in the Maya plugin.
- Support for vec4 primvars in the Katana plugin.
- Support for point instancers with varying topology that don't have authored 
  velocities in the Katana plugin.
- Support for importing collections encoded with new UsdCollectionAPI schema
  in the Katana plugin.
- Support for overriding data on a subset of point instancer instances in
  the Houdini plugin.

### Changed
- Build system now checks that the compiler being used supports 64-bit builds. 
- Tf diagnostic warnings and status messages issued in secondary threads are
  no longer printed to the terminal.
- Multiple delegates may now be registered with the Tf diagnostic system.
- SdfLayer::RemoveInertSceneDescription removes inert overs in variants.
- When saving a new .usdc file over an existing file, data is first saved to a 
  temporary file then renamed to the destination file. This provides some 
  protection for other processes that were reading the old file.
- Compression for structural sections in .usdc files is now enabled by default.
  New .usdc files will be marked as version 0.4.0 instead of 0.3.0 to 
  accommodate a bug fix described below. This means that any new .usdc files 
  created in this release will not be readable by previous USD releases.

  Users can disable the compression functionality by setting the environment
  variable `USD_WRITE_NEW_USDC_FILES_AS_VERSION` to "0.0.1". This will ensure 
  new .usdc files will be readable in older releases.

- UsdStage now uses the composed layer offset instead of its inverse when
  mapping times in layers to the stage. For example, to shift the time samples
  in a referenced layer stack forward by 10, users previously authored an
  offset of -10. Under the new behavior, users need to author an offset of 10
  instead. 

  Users can revert to the old behavior by setting the environment variable
  `USD_USE_INVERSE_LAYER_OFFSET` to 1. This will be deprecated in a future 
  release.

- UsdStage now allows a prim to be loaded without loading its descendants via
  a new UsdLoadPolicy argument supplied to UsdStage::Load and LoadAndUnload.
- UsdInherits, UsdSpecializes, and UsdReferences now map non-root prim paths
  to the namespace of the edit target.
- UsdAttribute::GetTimeSamplesInInterval now supports open/finite end points.
- UsdClipsAPI now authors the dictionary-style clips metadata introduced in
  release 0.7.6 by default.
- Schemas may now define fallback values for the majority of metadata fields,
  including custom metadata fields.
- Schemas may now have more than 256 tokens defined.
- usdedit now checks `USD_EDITOR` environment variable first when determining
  which editor to use.
- Ongoing refactoring to move code from hd to hdSt in preparation for material
  support in Hydra and use of Hydra in other renderer backends.
- Several performance optimizations in Hydra and UsdImaging.
- usdview now uses a new JSON-based format for its settings file. Settings
  files from older releases will be ignored.
- Several changes to provide better support for Viewport 2.0 in Maya plugin.
- Enum attributes in Maya now have their integer values exported to USD 
  instead of their descriptive names.
- Improved attribute transfer on USD Unpack node in Houdini plugin.
- usdvisible and usdactive attributes in Houdini plugin are now integers
  instead of strings.
- Several changes to Houdini plugin for compatibility with Houdini 16.5
  and improved HDK usage.

### Deprecated
- UsdGeomCollectionAPI, in favor of new UsdCollectionAPI.

### Removed
- TfDiagnosticNotice. The Tf diagnostic system no longer emits notices; 
  consumers relying on these notices should use diagnostic delegates instead.
- UsdGeomPointInstancer prototypeDrawMode attribute. Consumers should use
  the new drawMode attributes on UsdGeomModelAPI instead.

### Fixed
- Issue where users were forced to define certain preprocessor macros in 
  order to build code against USD. In particular, an issue introduced in 0.8.1
  caused users who did not #define PXR_ENABLE_PYTHON_SUPPORT to run into odd
  errors at runtime. 
  (Issue: [#304](https://www.github.com/PixarAnimationStudios/USD/issues/304))
- Bug that caused invalid .usdc files to be written. This affected all files
  with version 0.3.0; however, this file version was not enabled by default in
  release 0.8.1 and would only affect users who had explicitly enabled the
  new version via the `USD_WRITE_NEW_USDC_FILES_AS_VERSION` environment setting.
- Precision loss when writing time sample times to .usda files.
- Performance issue in UsdStage where memory would still be consumed by 
  child prims after their parent prim had been deactivated.
- Bug that caused UsdStage::ExpandPopulationMask to add invalid paths.
- Bug that caused UsdObject::GetMetadata to not fill in resolved paths for
  asset-valued metadata.
- Bug where layer offsets were incorrectly applied when authoring time samples.
- Bug in UsdUtilsStageCache that led to crashes during static destruction.
- Unpickable objects now act as occluders during picking in Hydra.
- Bug that caused usdview to stop drawing after "Reopen Stage" or "Open Stage".
- Bug that caused usdview to not play all frames if the step size was changed.
  (Issue: [#321](https://www.github.com/PixarAnimationStudios/USD/issues/321))
- Several performance and UI issues in usdview.
- Crash in Maya plugin with nested assemblies and variant set selections.
  (Issue: [#288](https://www.github.com/PixarAnimationStudios/USD/issues/288))
- Bug in Maya plugin where exporting over a previously-imported layer failed.
- Bug in Maya plugin that caused lights to flip between directional and 
  non-directional in legacy viewport.
- Various issues that prevented UsdKatana Python module from being imported.
  (Issue: [#323](https://www.github.com/PixarAnimationStudios/USD/issues/323))
- Material bindings above point instancer prototypes are now preserved in 
  the Katana plugin.
- Reworked how USD masters/sources are built in the Katana plugin so 
  that material bindings use the correct Katana paths.
- Hang in Houdini plugin when importing from variant paths. 
  (Issue: [#309](https://www.github.com/PixarAnimationStudios/USD/issues/309))

## [0.8.1] - 2017-10-02

### Added
- "append" and "prepend" operations for SdfListOp-valued fields.
  Scene description layers that contain these new operations will not be
  readable by earlier USD releases.
- `--validate` argument to sdfdump for checking file validity.
- Support for sub-root references and payloads.
- Ability to specify amount of data to prefetch when reading a .usdc
  file via the `USDC_MMAP_PREFETCH_KB` environment variable.
- Debugging output that shows memory mapping information for .usdc files.
  This can be enabled via the `USDC_DUMP_PAGE_MAPS` environment variable.
- UsdGeomMotionAPI schema.
- UsdGeomSubset schema for representing a subset of a geometric prim.
- Hydra GL support for displacement shaders
- Initial experimental support for PySide2 in usdview. By default, the 
  build will search for PySide, then PySide2. Alternatively, users may specify 
  `PYSIDE_USE_PYSIDE2=TRUE` to CMake to force the use of PySide2.
- Ability to view connections and relationship targets in usdview.
- Support for camera clipping range in Alembic plugin.
- Support for array attributes with "RfM Shaders" shading mode in Maya exporter.
- Support for angularVelocities attribute on point instancers in Katana plugin.
- Filtered view and improved selection to Tree View panel in Houdini plugin.
- Support for prototype offsets in point instancer exports in Houdini plugin.

### Changed
- Made Python dependency optional. Python support is enabled by default but
  may be disabled by specifying `PXR_ENABLE_PYTHON_SUPPORT=FALSE` to CMake
  or `--no-python` to build_usd.py.
- build_usd.py now requires that users install PyOpenGL manually. 
  (Issue: [#264](https://www.github.com/PixarAnimationStudios/USD/issues/264))
- Replaced addedOrExplicitItems attribute on Sdf.*ListOp in Python with
  GetAddedOrExplicitItems method.
- Renamed Append... methods on composition arc APIs (e.g. UsdReferences, 
  UsdInherits) to Add... and added a position argument to support new
  "append" and "prepend" operations. 

  Calling these functions with the default UsdListPositionTempDefault argument
  will author "add" operations, maintaining the previous behavior. We anticipate
  removing UsdListPositionTempDefault and replacing it with UsdListPositionFront
  so that "prepend" operations will be authored by default in 0.8.2. Users may 
  enable this behavior now by setting the `USD_AUTHOR_OLD_STYLE_ADD` environment
  variable to false.

- UsdStage now issues warnings for invalid layers it encounters instead of
  errors, which cause exceptions in Python.
- Code generation templates and schema.usda files used by usdGenSchema are 
  now installed as part of the build. Schemas can now use asset paths like
  "usdGeom/schema.usda" or "usdShade/schema.usda" instead of absolute paths
  to add installed schemas as sublayers. 
  (Issue: [#158](https://www.github.com/PixarAnimationStudios/USD/issues/158), 
   Issue: [#211](https://www.github.com/PixarAnimationStudios/USD/issues/211))
- Improved USD scenegraph instancing to allow more sharing of assets with
  inherits or specializes arcs. In one large example, this decreased stage load
  time by ~60%, stage memory usage by ~65%, and time to first image in usdview 
  by ~20%.
- Several optimizations to reduce I/O operations for .usda and .usdc files.
- Add support for compressed structural sections in .usdc files. This can
  significantly decrease file sizes for files that are dominated by prim and
  property hierarchy; one production shading file decreased in size by ~94%,
  from 21 MB to 1.3 MB.

  Compressed files are marked as version 0.3.0 and are not readable by earlier 
  USD releases. To help with transition, USD will not write these compressed 
  files by default until a future release. Users may enable this feature now 
  by setting the `USD_WRITE_NEW_USDC_FILES_AS_VERSION` environment variable 
  to "0.3.0". We plan to add compression for geometry data and time samples
  in a future release.

- UsdUtilsExtractExternalReferences now checks for references to external files
  in property values and metadata.
- Increased precision for rotations attribute in UsdSkelPackedJointAnimation 
  schema to 32-bit.
- UsdGeomPrimvar now allows primvar names with arbitrarily-nested namespaces.
- Sizeable refactor in Hydra to support sprims more like rprims in anticipation 
  of future lights and material support.
- Hydra no longer syncs rprims that are not visible.
- Several performance and stability fixes to point instancer handling in Hydra.
- "Reload All Layers" in usdview will try not to reset the camera or timeline.
- Numerous other improvements to usdview.
- The Alembic reader now converts single samples for properties in .abc files
  to single time samples in USD, instead of default values.
- Updated required Katana version to Katana 2.5v1.
- Improved bounds computations for point instancers in Katana plugin
- Account for motion sample time overrides when computing bounding boxes in
  Katana plugin.
- Improved stage cache in Houdini plugin, which can have significantly
  better performance in some circumstances.
- Improved handling of instanced USD primitives in Houdini plugin.

### Deprecated
- UsdGeomFaceSetAPI and related API in usdShade and usdLux in favor of new 
  UsdGeomSubset schema.

### Removed
- UsdStage::Close.
- Camera zUp-related functionality
- Support for faceVaryingInterpolateBoundary in UsdGeomMesh schema.
  See faceVaryingLinearInterpolation instead.

### Fixed
- Bug in build_usd.py where specifying `--no-embree` did the opposite.
  (Issue: [#276](https://www.github.com/PixarAnimationStudios/USD/issues/276))
- Bug in build_usd.py where specifying `--force-all` or `--force` on a
  Python dependency would cause the script to error out early. 
  (Issue: [#263](https://www.github.com/PixarAnimationStudios/USD/issues/263))
- Race condition between TfType consumers and TfType registry initialization.
- Composition bug where references with the same prim path and relative asset
  path would be considered duplicates even when anchoring location differed.
- Composition bug where payloads on arcs requiring ancestral opinions could
  result in errors.
- Change processing bug when removing sublayers specified by search path.
- Stack overflow crash when reading .usdc files due to excessive recursion.
- Possible crash in Hydra when using basis curves.
- Memory leak in Hydra task controller.
- Build issue for Alembic plugin with HDF5 support disabled when
  precompiled headers are enabled (which is the default on Windows).
- Several other bugs in Alembic reader.
- Performance issue in Katana plugin due to inefficiency in UsdShadeNodeGraph.

## [0.8.0] - 2017-07-29

### Added
- Added option to build_usd.py for building monolithic shared library
  and for building new sample Embree plugin.
- Added --mask option to usdcat, matching the --mask option in usdview.
- Added CPU-side external computation support to Hydra.
- Added Embree backend to serve as an example of how to add renderer 
  plugins to Hydra.
- Added camera gates to usdview.
- Added ability to Houdini plugin to set active state via usdactive 
  attribute and to write static data to default time via usdwritestatictopology,
  usdwritestaticprimvars, and usdwritestaticgeo attributes.
- Added UsdExportAttributes hda to Houdini plugin for setting primitive and 
  detail attributes which control various USD attributes or metadata.

### Changed
- Restored export of CMake targets from shared library builds.
- Asset paths in .usda files may now contain any printable character.
  (Issue: [#73](https://www.github.com/PixarAnimationStudios/USD/issues/73))
- Legacy special behavior for variant sets named "standin" has been disabled
  by default.
- Interpolation is now applied to time samples from clips when no sample
  exists at a specified clip time.
- Removed deprecated UsdShadePShader schema.
- Enabled vertex primvar sharing in Hydra. Hydra can now use significantly 
  less GPU memory.
- Many internal performance improvements to Hydra.
- usdview HUD now shows the backend renderer plugin used by Hydra
- Wireframe drawing now ignores opacity.
- Katana plugin properly interprets schema-supported triangle subdivision rule 
  when meshes are read in and rendered.

### Fixed
- Fixed several issues in build_usd.py related to building dependencies.
  (Issue: [#225](https://www.github.com/PixarAnimationStudios/USD/issues/225), 
   Issue: [#230](https://www.github.com/PixarAnimationStudios/USD/issues/230), 
   Issue: [#234](https://www.github.com/PixarAnimationStudios/USD/issues/234))
- Fixed bug where UsdGeomPrimvar::IsIndexed() would not work for attributes
  with only authored time samples. 
  (Issue: [#238](https://www.github.com/PixarAnimationStudios/USD/issues/238))
- Fixed small platform inconsistencies for Windows in Hydra.
- Fixed crash in Katana plugin when using point instancers that did not have
  scales or orientations specified. 
  (Issue: [#239](https://www.github.com/PixarAnimationStudios/USD/issues/239))
- Fixed issue in Houdini plugin where the "w" attribute was not converted
  to the "angularVelocities" attribute for point instancer prims.
	
## [0.7.6] - 2017-06-30

### Added
- Added `build_scripts/build_usd.py` for building USD and its dependencies.
- Added support for building static libraries or a single monolithic
  shared library. See [BUILDING.md](BUILDING.md#linker-options) for more
  details.
- Added support for color spaces to Usd. Color configuration and management 
  system can be specified on a UsdStage (via the stage's root layer), and 
  colorSpace metadata is available on any UsdAttribute.
- Added clip set functionality to Usd. This provides the ability to specify 
  multiple sets of value clips on the same prim, which allows users to compose 
  different sets of clips together. See UsdClipsAPI for more details.
- Added initial UsdLux schemas for representing interchangeable lights and 
  related concepts, and UsdRi schemas for Renderman-specific extensions. 
  UsdImaging/Hydra support will be coming in a future release.
- Added ability to specify smooth triangle subdivision scheme for catmullClark
  surfaces in UsdGeomMesh and imaging support in Hydra.
- Added backdrops for node layout description in UsdUI.
- Added support for sharing immutable primvar buffers to Hydra.
  This can greatly reduce the memory required on the GPU when displaying
  typical scenes. It is currently experimental and disabled by default. It
  can be enabled for testing with the environment variable
  `HD_ENABLE_SHARED_VERTEX_PRIMVAR`.
- Added support for uniform primvars for basis curves in Hydra.
- Added ability to Alembic plugin to control number of Ogawa streams used for 
  each opened archive via the environment variable `USD_ABC_NUM_OGAWA_STREAMS`,
  which defaults to 4.
- Added Katana plugin support for reading UsdLux lights.
- Added `--camera` command line argument to usdview to specify the initial 
  camera to use for viewing.
- Added display of instance index and (if authored) instance ID to rollover
  prim info in usdview. 
- Added initial version of Houdini plugin.

### Changed

- Removed unnecessary dependency on OpenImageIO binaries.
- Removed unnecessary dependency on boost::iostreams.
- Made HDF5 backend for Alembic plugin optional. HDF5 support is enabled by 
  default but may be disabled by specifying `PXR_ENABLE_HDF5_SUPPORT=FALSE`
  to CMake.
- Metadata fields for value clips feature in UsdStage (e.g. clipAssetPaths,
  clipTimes, etc.) have been deprecated in favor of the new clips metadata
  dictionary. Authoring APIs on UsdClipsAPI still write out the deprecated
  metadata unless the environment variable `USD_AUTHOR_LEGACY_CLIPS` is set 
  to 0. This will be disabled in a future release.
- When using value clips, Usd will now report time samples at each time 
  authored in the clip times metadata.
- Completed support for encoding UsdShade data using UsdAttribute connections.
  Authoring APIs still write out the old encoding, unless the environment 
  variable `USD_SHADE_WRITE_NEW_ENCODING` is set to 1. This will be enabled
  in a future release.
- Removed support for deprecated UsdShade schemas UsdShadeParameter and
  UsdShadeInterfaceAttribute, which have been superseded by UsdShadeInput.
- Removed deprecated UsdRiLookAPI, which has been replaced by UsdRiMaterialAPI.
- Removed deprecated Hydra scene delegate APIs.
- Significant refactoring and refinements to allow multiple backends to be
  plugged in to Hydra.
- UsdImaging now pushes GL_DEPTH_BUFFER_BIT to avoid any potential state
  pollution issues when integrating it with other renderers. 
- Improved error handling in Hydra for:
  - Invalid point instancer input
  - Inconsistent displayColor and displayOpacity primvars
  - Direct instances of imageable prims
- usdview more gracefully handles the situation where it doesn't get a
  valid GL context (e.g., due to limited resources on the GPU).
- Improved how edits are handled on assembly nodes in the Maya plugin.
- Improved UsdGeomPointInstancer support in the Katana plugin:
  - Overhauled methods for computing instance transforms and aggregate bounds. 
  - Backward motion and multi-sampled point instancer positions, orientations, 
    and scales are now supported.
  - Removed opArgs attributes that were used for configuring the reader's 
    behavior.
  - More robust primvar transfer to point instancer's Katana locations.
  - Restructured prototype building logic to preserve material bindings
    when creating prototype prims.
- Numerous changes and fixes for Mac and Windows ports.

### Fixed

- Fixed issue that caused IDEs and other tools to find headers in the build
  directories rather than the source directories.
- Fixed race condition in TfType.
- Fixed long-standing memory corruption bug in Pcp that surfaced when 
  building on MacOS with XCode 8.3.2.
- Fixed potential deadlocks in Usd and Pcp due to issues with TBB task group 
  isolation.
- Fixed bug where UsdStage::Flatten was not preserving attribute connections.
- Fixed infinite loop on glGetError in the presence of an invalid GL context.
  (Issue: [#198](https://www.github.com/PixarAnimationStudios/USD/issues/198))
- Fixed build issues that caused the Maya plugin to be unusable after building
  on MacOS.

## [0.7.5] - 2017-05-01

C++ namespaces are now enabled by default. The `PXR_ENABLE_NAMESPACES` CMake
option may be used to disable namespaces if needed.

This release adds initial experimental support for building the USD core
libraries on Windows.

### Added
- Added ability to build doxygen documentation. See
  [BUILDING.md](BUILDING.md#documentation) for more details.
- Added support for pre-compiled headers. By default, this is disabled
  for Linux and Mac builds and enabled for Windows builds. This may be
  configured with the `PXR_ENABLE_PRECOMPILED_HEADERS` CMake option.  
- Added many unit tests for core libraries
- Added UsdStage::Save and UsdStage::SaveSessionLayers
- Added attribute connection feature that allows consumers to describe
  connections between an attribute and other prims and properties.
- Added instance proxy feature that allows consumers to interact with
  objects on a UsdStage as if scenegraph instancing were not in use while
  retaining the performance benefits of instancing.
- Plugins providing Boundable schema types can now register functions for
  computing extents. This allows code in the USD core to compute extents for
  prims with types provided by external schemas.
- Added support for specializes arcs to usdShade.
- Added UsdUISceneGraphPrimAPI for representing prim display properties.
- Added `HD_DISABLE_TINY_PRIM_CULLING` flag for `TF_DEBUG` to make it easier
  to turn off tiny prim culling for debugging.
- Added support for generic metadata to GLSLFX, allowing hardware shaders to
  pass information to downstream rendering code.
- Added ability to adjust basic material properties in usdview.
- Plugins can now register MayaPrimWriter subclasses for exporting Maya 
  nodes to USD prims.
- Added ability to override default motion data (sample times, current time, 
  shutter open/close) on a per-scope basis in Katana plugin. Users can author
  these overrides by modifying the global graph state, then decide to use
  the default motion values with or without these overrides applied.

### Changed
- Removed OpenEXR dependency from core libraries. Note that OpenEXR is still
  required for imaging.
- Made Ptex dependency optional. It is only required if Ptex support for
  imaging is enabled. This is on by default, but can be disabled by specifying
  `PXR_ENABLE_PTEX_SUPPORT=FALSE` to CMake.
- Cleaned up and expanded doxygen documentation:
  - Added documentation for value clips and scenegraph instancing USD features.
  - Enabled XML generation for external tools
  - Fixed invalid tags and formatting in several places
- UsdTreeIterator has been replaced by UsdPrimRange, which provides 
  container-like semantics in C++ (e.g., the ability to use them in range-based
  for loops).
- Removed UsdPrim::GetPayload API
- UsdRelationship and UsdAttribute will no longer forward target/connection
  paths that point to objects within instances to objects in master prims.
  Users can use the new instance proxy functionality to work with these paths
  instead.
- Relationships belonging to prims in instances that point to the root of the
  instance no longer cause errors.
- Value clip metadata can now be sparsely overridden across composition arcs.
- Deprecated shading property schemas UsdShadeParameter and 
  UsdShadeInterfaceAttribute. These are replaced by UsdShadeInput, which can
  represent both input parameters on shaders and interface inputs on node
  graphs.
- The `--compose` command line parameter for usddiff is now `--flatten`
- Numerous cleanup changes for Hydra API. In particular:
  - HdEngine::Draw has been retired in favor of HdEngine::Execute.
  - HdSceneDelegate::IsInCollection has been deprecated in favor of render-tag
    based API.
- Meshes with subdivisionScheme = none will no longer be bilinearly subdivided.
- usdview now displays prim hierarchies beneath instances inline in the 
  browser. The "Jump to Master" command has been removed in favor of this
  new functionality.
- The default specular response in usdview has been reduced by a factor of 5.
- The Maya plugin now excludes attributes during import if they are tagged
  with customData specifying them as generated.
- Numerous changes for ongoing Mac and Windows port efforts.
- Changed scoping of built-out point instancer prototypes in Katana plugin
  so that their hierarchy more closely matches the original USD hierarchy.

### Fixed
- Added workaround to avoid redefined "_XOPEN_SOURCE" and "_POSIX_C_SOURCE" 
  macro warnings due to Python includes on Linux. 
  (Issue: [#1](https://www.github.com/PixarAnimationStudios/USD/issues/1))
- Fixed issue where code would be generated directly into the install location 
  when running cmake, interfering with the use of make's DESTDIR functionality.
  (Issue: [#84](https://www.github.com/PixarAnimationStudios/USD/issues/84))
- Fixed memory leak when converting Python tuples to C++ GfVec objects.
- Fixed thread-safety issue with `PCP_PRIM_INDEX` and `PCP_PRIM_INDEX_GRAPHS`
  `TF_DEBUG` flags. 
  (Issue: [#157](https://www.github.com/PixarAnimationStudios/USD/issues/157))
- Fixed invalid memory access bug in Pcp
- Fixed memory leak when saving .usdc files.
- Fixed bug with removing an attribute's last time sample in a .usdc file.
- Fixed bug where instance prims with locally-defined variants would incorrectly
  share the same master prim.
- Reader-writer locks are now used for various registry to improve performance.
- Improved speed of adding an empty sublayer to an opened stage. In one large
  test case, time to add an empty sublayer went from ~30 seconds to ~5 seconds.
- Fixed issues with opening .usda files and files larger than 4GB on Windows.
  (Issue: [#189](https://www.github.com/PixarAnimationStudios/USD/issues/189))
- Fixed backwards compatibility issues with UsdShadeInput and 
  UsdShadeConnectableAPI.
- Memory footprint has been significantly reduced in Hydra's adjacency tables
  for subdivs that contain vertices of high valence.
- Fixed crash in imaging due to invalid textures.
- Fixed shadow banding artifacts when using vertex color with basis curves.
- Fixed buffer overrun when using GPU smooth normals.
- Fixed issue in Maya plugin where multiple assemblies that reference the
  same file might all be affected by the edits of one particular assembly 
  when viewing them via a proxy.
- Fixed issue in Maya plugin where path resolution was being done by the
  plugin itself rather than deferring to Usd and Sdf.

## [0.7.4] - 2017-03-03

USD now supports C++ namespaces. They are disabled by default in this release
to ease transition, but we anticipate enabling them by default in the next 
release. 

Specify the `PXR_ENABLE_NAMESPACES` CMake option to enable namespaces.
See documentation in [BUILDING.md](BUILDING.md#c-namespace-configuration) for more details.

### Added
- Added GfHalf type to represent half-precision numeric values. This type 
  is currently an alias to the "half" type supplied by the ilmbase library 
  USD is built against. This will be changed in an upcoming release to point 
  to a version of "half" that is included in the USD codebase, which will
  allow us to remove the USD core's dependency on ilmbase.
- Added UsdShadeInput class to represent shader or NodeGraph input.
  These are encoded as attributes in the "inputs:" namespace. By default, 
  this encoding will not be written out via UsdShadeConnectableAPI unless 
  the 'USD_SHADE_WRITE_NEW_ENCODING' environment variable is set.
- Added "Composition" tab to usdview that allows users to inspect the 
  composition structure of a selected prim.
- Added tests for Hdx. These tests are currently not run as part of the unit 
  test suite, but are being included for users to try on their platforms and 
  to serve as example code.
- Added icons for the USD assemblies in Maya.
- Added initial implementation of Katana plugin for reading 
  UsdGeomPointInstancer locations. 

### Changed
- CMake module now additionally looks for the pyside-uic binary under the name 
  pyside-uic-2.7 to accommodate package managers on OSX.
- Removed GLUT dependency.
- Removed double-conversion dependency.
- Modified all uses of "half" type from ilmbase to use new GfHalf type.
- Renamed UsdShadeSubgraph to UsdShadeNodeGraph to conform with final
  MaterialX terminology.
- Numerous changes for in-progress work on refactoring Hydra to support render 
  delegates.
- Updated UI in usdview to display legends for text coloring in collapsible 
  panes and to include more information.
- The import UI in the Maya plugin now enables reading of anim data by default.
- Updated API in Maya plugin that referred to "look" to "material".
- Numerous changes for ongoing Mac and Windows porting efforts.

### Fixed
- Fixed several issues when building with C++ namespaces enabled.
- Fixed bug with selection highlighting for instanced prims in usdview.
- Fixed crash that occurred when an invalid color primvar was encountered while
  using usdview's "simple" renderer.
- Fixed issue with TF_REGISTRY_DEFINE* macros that caused build failures
  for the Maya plugin on OSX. 
  (Issue: [#162](https://www.github.com/PixarAnimationStudios/USD/issues/162))

## [0.7.3] - 2017-02-05

### Added
- Added support for RelWithDebInfo and MinSizeRel release types.
- Added initial support for C++ namespaces. This feature is 
  currently experimental but can be enabled by specifying the 
  `PXR_ENABLE_NAMESPACES` option to CMake. See documentation in
  BUILDING.md for more details.
- Added population masking to UsdStage. 
  - Consumers can specify what parts of a stage to populate and use
    to reduce resources used by the stage.
  - usdview now has a --mask option allowing users to view just the
    specified portion of a stage.
- Added UsdPrim API for collecting relationship targets authored 
  in a given prim subtree.
- Added UsdShade API to allow creating and connecting outputs
  on subgraphs and shaders instead of terminals.
- Added support for bool shader parameters in Hydra. 
  (Issue: [#104](https://www.github.com/PixarAnimationStudios/USD/issues/104))
- Added ability to independently toggle display of geometry with
  purpose=proxy in usdview.
- Added initial set of unit tests for pxrUsd library in the Maya plugin.
- The Maya plugin now supports multi-sampling of frames during export.
- Attributes in Maya may now be tagged to be converted from double
  to single precision during export.
- Alembic tagging for attributes in Maya are now respected during 
  export. This includes support for primvars and custom prefixes.
- Added support for playing back animation in assemblies via Hydra
  when the Playback representation is activated in Maya.
- Added support for reading material references in Katana plugin.

### Changed
- CMake will no longer look for X11 on OSX. 
  (Issue: [#72](https://www.github.com/PixarAnimationStudios/USD/issues/72))
- The build system now uses relative paths for baking in plugin
  search paths, allowing for relocatable builds as long as all
  build products are moved together as one unit.
- Removed hard-coded /usr/local/share directory from plugin search path.
- Optimized creation of prim specs in Sdf. Creating 100k prim specs 
  previously took 190s, it now takes 0.9s.
- SdfPath::GetVariantSelection now returns a variant selection only
  for variant selection paths.
- UsdStage no longer issues an error if requested to unload a path that
  does not identify an unloadable object.
- Changed API on UsdInherits, UsdReferences, UsdRelationship,
  UsdSpecializes, UsdVariantSet, and UsdVariantSets for clarity.
- Hydra GPU compute settings have been consolidated under a single
  HD_ENABLE_GPU_COMPUTE environment setting which is enabled by
  default if the OpenSubdiv being used supports the GLSL compute kernel.
- Numerous changes for in-progress work on refactoring Hydra to
  support render delegates.
- Changed 'Display' menu in usdview to 'Display Purposes...' and menu
  items to match USD terminology.
- Updated required Maya version to Maya 2016 EXT2 SP2.
- Normals are now emitted by default when exporting polygonal meshes in Maya.
- Katana plugin now supports deactivating prims directly on
  UsdStage for improved efficiency.
- Continued work on deprecating UsdShadeLook in favor of UsdShadeMaterial.
- Numerous changes for ongoing Mac and Windows porting efforts.
- Changed coding style to use symbol-like logical operators
  instead of the keyword-like form (e.g., '&&' instead of 'and').
- Various cleanup changes to fix comments, compiler warnings, and
  to remove unused/old code.

### Fixed
- Fixed composition bug where nested variants with the same name
  were composed incorrectly.
- Fixed composition bug where variant selections from classes
  were not being applied in certain cases. 
  (Issue: [#156](https://www.github.com/PixarAnimationStudios/USD/issues/156))
- Fixed .usda file output to write out floating point values
  using the correct precision and to use exponential representation
  for values greater than 1e15 (instead of 1e21) to avoid parsing
  issues.
- Fixed bug where negative double values that could be represented as
  floats were not stored optimally in .usdc files.
- Fixed bug where UsdPrim::Has/GetProperty would return incorrect results.
- Fixed bug where UsdObject::GetMetadata would return incorrect results
  for dictionary-valued metadata in certain cases.
- Fixed bug where reading certain .usdc files while forcing single-threaded
  processing via PXR_WORK_THREAD_LIMIT would lead to a stack overflow.
- Fixed missing garbage collection for shader and texture resource
  registries in Hydra.
- Several fixes for nested instancing support in Hydra (native
  USD instances containing other instances or point instancers, etc.)
- Fixed "Jump to Bound Material" in "Pick Models" selection mode in usdview.
- Fixed bug where USD assemblies would not be drawn in Maya under VP2.0.
- Workaround for performance issue when drawing proxies in Katana plugin.
- Fixed issue causing USD Alembic plugin to fail to build with
  Alembic 1.7. 
  (Issue: [#106](https://www.github.com/PixarAnimationStudios/USD/issues/106))
- Various other bug fixes and performance improvements.

## [0.7.2] - 2016-12-02

### Added
- Added ability to apply a custom prefix to shared library names
  by specifying the `PXR_LIB_PREFIX` option to CMake.
- Added ability to disable tests by using the `PXR_BUILD_TESTS` option
  with CMake.
- usddiff can now diff two directories containing USD files.
- Value clips on a prim may now be specified using 'template' clip
  metadata. This is a more compact representation for simple cases
  where each value clip corresponds to a single time code.
- Better support for non-file-backed asset resolution
  - Made ArDefaultResolver public, so that subclasses can layer
    behavior on top of it.
  - Added ArResolver API for retrieving files from data store when
    needed.
- Initial version of UsdGeomPointInstancer schema and preliminary
  support in usdImaging.
- pxrUsdReferenceAssemblies in Maya can now import animation from a USD scene.
  See http://openusd.org/docs/Maya-USD-Plugins.html#MayaUSDPlugins-ImportingasAssemblies 
  for details. 
- Core USD metadata (e.g. hidden, instanceable, kind) now roundtrips through 
  Maya, stored as "USD_XXX" attributes on corresponding Maya nodes.

### Changed
- Removed dependency on Qt. Note that PySide is still a dependency for 
  usdview.
  - Removed imaging/glfq library as part of this work.
- Modified variant behavior in composition:
  - Variants may now be nested within other variants in scene description.
    Nested variants may be authored by using nested UsdEditContext objects
    returned by UsdVariantSet::GetVariantEditContext.
  - Weaker variants may now introduce variant selections that affect
    stronger variants. Previously, these selections would be ignored 
    and the variant fallbacks would be used instead.
- Optimized .usdc file format performance in certain cases:
  - Output is buffered in memory to improve write times for layers 
    containing many nested dictionaries. In one such example, wall-clock
    time improved by ~64% and system time improved by 89%.
  - Time samples are now written out grouped by time to improve read 
    performance for frame-by-frame access patterns.
- Several optimizations to improve USD scenegraph authoring and composition 
  speed and reduce memory consumption. In some use cases, we observed:
  - ~5-7x less memory consumed by Pcp dependency tracking structures
  - ~40% less time spent to author large stages
- Exporting USD from the Maya Export window now produces binary .usd
  files by default instead of .usda files.

### Fixed
- Fixed bug where edits to a layer opened in Maya would persist when
  reopening that layer in a new scene.
- Fixed issue where exporting USD from the Maya Export window would
  produce files ending with ".usda", even if a different format
  was specified. The exporter should now respect any extension that
  is entered, so long as the "Default file extensions" option is
  turned off.
- Fixed several issues with example plugins being improperly built, making
  them unusuable, and missing functionality.
- Various other bug fixes and performance improvements.

## [0.7.1] - 2016-09-21

### Added
- UsdMaterial schema for shading, intended to replace UsdLook. Also added
  support for this new schema to Hydra.
- Initial version of UsdUI schema library, intended for encoding GUI 
  information on USD prims.
- Parallel teardown of some data members in UsdStage, speeding up overall
  teardown by ~2x.
- Support for packed vertex normals to Hydra, reducing GPU memory consumption
  by 20-30% in some cases.
- Ability to compare two composed stages to usddiff by specifying the
  "--compose" or "-c" options.
- Support for soft-selection for collapsed USD assemblies in Maya.
- Support for exporting color sets and UV sets as indexed primvars from Maya.

### Changed
- Refactored Hydra libraries to move higher-level concepts out of the core
  hd library and into hdx.
- Removed use of opensubdiv3/ include path from imaging code in favor of
  standard opensubdiv/ include.
- Modified UsdStage to automatically load newly-discovered payloads if
  their nearest ancestor's payload was also loaded. For example, consumers
  will no longer have to explicitly call UsdStage::Load to ensure payloads
  are loaded when switching variants on a prim, so long as their nearest
  ancestor is also loaded.
- Refactoring and other changes to help with Mac and Windows ports.
- Updated doxygen and other documentation.

### Fixed
- Fixed issue that caused Alembic plugin (usdAbc) to be misconfigured at
  build time, which required users to manually update its plugInfo.json
  and set an environment variable at runtime to make it work. This plugin
  is now installed to $PREFIX/plugin/usd/ and requires no additional
  steps to use once it has been built.
- Fixed issue with .usdc files that resulted in corrupted files on Windows.
  The file structure was changed, so the .usdc version has been bumped
  from 0.0.1 to 0.1.0. USD will continue to write 0.0.1 files by default,
  but will begin writing 0.1.0 files in the near future.

  Users may choose to write files with the new version immediately
  by setting the environment variable `USD_WRITE_NEW_USDC_FILES_AS_VERSION`
  to "0.1.0".

  Note that 0.0.1 files are still readable using this release, 
  except for those that have been generated on Windows using 0.7.0. 
  Early testers on Windows will need to regenerate those files
  with this release.
- Fixed issue that caused usdGenSchema to generate files that could
  not be compiled.
- Added a workaround for TBB-related issue in Maya that caused hangs when 
  using the USD Maya plugin. This can be enabled at build time by 
  specifying the `PXR_MAYA_TBB_BUG_WORKAROUND` option to CMake.
- Various other bug fixes and performance improvements.

## [0.7.0] - 2016-08-01

Initial release
