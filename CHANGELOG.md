# Change Log

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
