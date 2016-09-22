# Change Log

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
