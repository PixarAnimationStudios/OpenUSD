# Change Log

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
