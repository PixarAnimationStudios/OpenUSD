# Change Log

## [18.11] - 2018-10-10

### Added
USD:
- [usdVol] UsdVol schema for representing volumes. (PR #567)
- [usdShade] Ability to define shaders in the shader definition registry 
   using UsdShade. 
- [usdShaders] Shader definition registry plugin for core nodes defined in 
   the UsdPreviewSurface specification.

Imaging:
- [hd, usdImaging] Hydra support for UsdVol schema. (PR #567)
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
  (PR #527)

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
  (PR #373, #453, #481, #484, #488)

USD:
- [usd] UsdAPISchemaBase is now an abstract base class as originally intended.
- [usdGeom] Improved performance of UsdGeomPointBased::ComputeExtent. In one 
  example, this provided a ~30% improvement. (PR #588)
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
- [hd] Properly set up both the shadow projection and view matrices. (PR #583)
- [hd] Optimization to not run garbage collection if a texture hasn't change.
- [hd, hdSt, usdImaging] Update wrap mode fallback behavior to useMetadata.
- [hdx] Improved shadow support in HdxTaskController. (PR #541)
- [hdx] "Enable Hardware Shading" entry point is now "Enable Scene Materials".
- [hdSt] Support to generate GLSL-compatible names for resources/primvars.
- [usdImagingGL] Better support for animated textures. (PR #488)
- [usdview] Better error reporting if an unsupported backend is selected. 
  (PR #635)
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
  (PR #614)
- [cmake] Installed headers could take precedence over source headers, leading 
  to build failures. (Issue #83) 

USD:
- [tf] TfEnvSetting crash on MSVC 2017.
- [ar] ArDefaultResolver::AnchorRelativePath did not call IsRelativePath on
  derived resolvers. (PR #426)
- [sdf] SdfCopySpec did not handle copying prim specs to variants and vice-versa.
- [sdf] Incorrect notification was sent when removing subtrees of inert specs.
- [usd] Prim payload was not automatically loaded when a deactivated ancestor 
  prim with a payload was activated. (Issue #604)
- [usd] Incorrect lists of time samples would be returned in certain cases
  with value clips.
- [usd, usdzip] .usdz files were not fully compliant with the zip file format 
  specification. 
- [usdUtils, usdzip] Creating an ARKit .usdz file with a .usda asset would 
  create an invalid file.
- [usdMtlx] MaterialX plugin build failures on Windows and macOS. (Issue #611)

Imaging:
- [hdx] Picking with disabled scene materials generating shader errors.
- [hdSt] Fix immediate draw batch invalidation on buffer migration.
- [hdSt] Fix garbage collection on buffer migration.
- [hdSt] Bindless texture loading not being deferred.
- [usdImaging] Wrong transforms were being used when using usdLux light 
  transforms (Issue #612).
- [usdImaging] Fix for usdImaging's TextureId not accounting for origin.

Houdini plugin:
- [gusd] Several fixes for building with Houdini 17. (PR #549)
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
    static boost libraries may lead to issues with Python bindings. (Issue #407)

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
  - UsdStage::GetObjectAtPath for retrieving a generic UsdObject. (PR #390)
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
  - Option to apply Euler filtering to transforms during export. (PR #299)
  - Option to filter out certain Maya node types during export. (PR #475)

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
    if it's available to avoid TLS 1.2 issues. (Issue #449)
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
    (PR #463)

- Maya plugin:
  - MAYA_SCRIPT_PATH and XBMLANGPATH environment variable settings have changed
    due to new resource file install location documented above. Refer to Maya
    plugin documentation for new values.
  - Large refactoring and cleanup of usdMaya. USD importers and exporters for
    built-in Maya nodes have been moved to pxrUsdTranslators plugin.
  - Proxy shape now redraws in response to changes on UsdStage.
  - Diagnostic messages are now batched together for better reporting behavior
    and to prevent log spewage.
  - Improved primvar export. (PR #330)
  - Camera shake now applied to USD cameras on export. (PR #366)

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
  - Issue preventing use of NMake generator on Windows. (PR #519)
  - Several fixes for static library builds.

- USD:
  - Bug where SdfLayer::GetDisplayName would return an empty string for 
    anonymous layers if the tag contained a ":". (PR #440)
  - Bug where SdfLayer::UpdateExternalReferences would not handle variants and 
    payloads correctly.
  - Incorrect load state handling in UsdStage for payloads introduced across 
    sub-root references, inherits, and specializes arcs.
  - Incorrect value clip time mapping during value resolution on UsdStage.
  - Change processing bug when adding a new inert prim spec.
  - Load/unload change processing bug with nested instances on UsdStage.
  - Crash when opening a UsdStage with a UsdStagePopulationMask containing 
    instance prims. (Issue #497)
  - Test failures when USD_EDITOR environment variable is set. (Issue #505)
  - Bug where non-constant primvars were inherited down namespace in
    UsdGeomPrimvarsAPI.

- Imaging:
  - Triangulation/quadrangulation of face varying for refined meshes.
  - Several selection and picking issues.

- UsdImaging:
  - Material binding to instanced prims.
  - Change processing for UsdShadeShader prims beneath UsdShadeMaterial prims.

- Alembic plugin:
  - Texture coordinate indices are now preserved. (PR #520)

- Katana plugin:
  - Several asset resolution fixes. (Issue #535)
  - Bug preventing correct inheritance of motion sample times overrides as
    authored by PxrUsdInDefaultMotionSamples and PxrUsdInMotionOverrides in
    Katana plugin.

- Houdini plugin:
  - Fixed several display and update issues with treeview panel.
  - Fixed bug in batched loading of masked prims.

## [0.8.5a] - 2018-05-21

### Fixed
- Fixed broken URL in build_usd.py for downloading libtiff. (Issue #498)

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
- Removed GLUT dependency for libtiff in build_usd.py. (Issue #402)
- build_usd.py will now use cURL to download dependencies if it's
  installed in the user's PATH. This can help avoid TLS v1.2 errors
  when downloading from certain sites. (Issue #449)
- Numerous documentation additions and fixes throughout the codebase.
- Improved performance of SdfCopySpec and UsdUtils stitching API; one test
  case showed a 35% speed improvement, from 160s to 118s.
- SdfLayer now uses the resolved path provided by Ar for a given identifier
  to determine the layer file format. (Issue #144)
- Simplified API for setting layer data in SdfFileFormat subclasses.
- Adding or removing invalid sublayers now results in composition errors.
- UsdStage::CreateNew and CreateInMemory now accept an InitialLoadSet argument.
  This controls whether new payloads are loaded automatically. (Issue #267)
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
- Error when extracting boost on Windows in build_usd.py. (Issue #308)
- The build now prefers Alembic library specified at cmake time over any
  Alembic library found in PATH. (Issue #409)
- Several compile-time warnings on clang and other compilers.
- Bug where list-op valued metadata was not emitted by UsdStage::Flatten.
- Missing notifications for master prims affected by metadata/property changes.
- Crash in UsdCollectionAPI::ApplyCollection when given an invalid collection 
  name. (Issue #425)
- Change processing for changes to drawMode property in imaging.
- Numerous fixes to cards draw mode.
- Change processing bug when removing nested point instancers.
- Crashes in pxOsd due to incorrect authored crease data.
- Bug where usdview would not redraw after switching renderer.
- Crash in Alembic plugin when reading files with object names beginning 
  with numerals.
- Change processing bug when reloading an Alembic layer. (Issue #429)
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
  when sources were outside the scope of the point instancer. (Issue #286)

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
- Build now supports versioned OpenEXR and IlmBase shared libraries. (Issue #71)
- Layer identifiers may now include '?' characters. (Issue #289)
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
  locations, which could have led to runtime errors. (Issue #315, Issue #325)
- Headers are now installed properly for monolithic builds. (Issue #277)
- Original install location will no longer be searched for plugins after
  relocating builds. (Issue #363)
- Fixed thread-safety issue where plugins with the same name but in different
  locations could be loaded twice. (Issue #358)
- Fixed bug where layers that were muted via SdfLayer::AddToMutedLayers before
  they were first opened could not be unmuted.
- Fixed bug in usdGenSchema where changing an existing property's type in a 
  schema would not be reflected in the generated code.
- Fixed bug where a large (> 1460) number of variants in a .usda file would
  cause a "memory exhausted" error when parsing that file.
- Fixed broken pread mode for .usdc files.
- Fixed bug that caused UsdStage::CreateNew to crash on Windows. (Issue #364)
- Fixed bug when using a UsdStagePopulationMask with prims beneath instances. 
  (Issue #312)
- Fixed bug where setting float-valued attributes to +inf in Python would fail.
- UsdAttribute::GetTimeSamplesInInterval now properly accounts for layer 
  offsets. (Issue #352)
- Internal references or empty asset paths no longer cause errors in 
  UsdUtilsFlattenLayerStack.
- Fixed bug where UsdGeomPrimvar::GetTimeSamples would miss time samples for
  indexed primvars.
- Disabled tiny prim culling in Hydra by default. It can be re-enabled by
  setting the environment variable `HD_ENABLE_TINY_PRIM_CULLING` to 1.
  (Issue #314)
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
- Usd.GetVersion() for retrieving USD version in Python (Issue #306)
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
  (Issue #290)
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
  errors at runtime. (Issue #304)
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
  (Issue #321)
- Several performance and UI issues in usdview.
- Crash in Maya plugin with nested assemblies and variant set selections.
  (Issue #288)
- Bug in Maya plugin where exporting over a previously-imported layer failed.
- Bug in Maya plugin that caused lights to flip between directional and 
  non-directional in legacy viewport.
- Various issues that prevented UsdKatana Python module from being imported.
  (Issue #323)
- Material bindings above point instancer prototypes are now preserved in 
  the Katana plugin.
- Reworked how USD masters/sources are built in the Katana plugin so 
  that material bindings use the correct Katana paths.
- Hang in Houdini plugin when importing from variant paths. (Issue #309)

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
- build_usd.py now requires that users install PyOpenGL manually. (Issue #264)
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
  to add installed schemas as sublayers. (Issue #158, #211)
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
  (Issue #276)
- Bug in build_usd.py where specifying `--force-all` or `--force` on a
  Python dependency would cause the script to error out early. (Issue #263)
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
  (Issue #73)
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
  (Issue #225, #230, #234)
- Fixed bug where UsdGeomPrimvar::IsIndexed() would not work for attributes
  with only authored time samples. (Issue #238)
- Fixed small platform inconsistencies for Windows in Hydra.
- Fixed crash in Katana plugin when using point instancers that did not have
  scales or orientations specified. (Issue #239)
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
  (Issue #198)
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
  macro warnings due to Python includes on Linux. (Issue #1)
- Fixed issue where code would be generated directly into the install location 
  when running cmake, interfering with the use of make's DESTDIR functionality.
  (Issue #84)
- Fixed memory leak when converting Python tuples to C++ GfVec objects.
- Fixed thread-safety issue with `PCP_PRIM_INDEX` and `PCP_PRIM_INDEX_GRAPHS`
  `TF_DEBUG` flags. (Issue #157)
- Fixed invalid memory access bug in Pcp
- Fixed memory leak when saving .usdc files.
- Fixed bug with removing an attribute's last time sample in a .usdc file.
- Fixed bug where instance prims with locally-defined variants would incorrectly
  share the same master prim.
- Reader-writer locks are now used for various registry to improve performance.
- Improved speed of adding an empty sublayer to an opened stage. In one large
  test case, time to add an empty sublayer went from ~30 seconds to ~5 seconds.
- Fixed issues with opening .usda files and files larger than 4GB on Windows.
  (Issue #189)
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
  for the Maya plugin on OSX. (Issue #162)

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
- Added support for bool shader parameters in Hydra. (Issue #104)
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
- CMake will no longer look for X11 on OSX. (Issue #72)
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
  were not being applied in certain cases. (Issue #156)
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
  Alembic 1.7. (Issue #106)
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
