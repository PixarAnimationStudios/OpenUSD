# OpenUSD Pyproject Packaging

This directory contains a `pyproject.toml` based wheel build flow.

The OpenUSD build is created with `build_scripts/build_usd.py`, and a PEP 517 backend stages files from that install tree into a wheel.
In order to provide robust wheels with proper isolation, ownership of libraries, and dependency links, the wheels are modified.
This approach was chosen over installing all wheels to a common namespace because of fragility with uninstallation, upgrades, versions, and layout conflicts.
The cmake build was not extended to support a wheel native build, so the wheels needs to be fixed up by the backend.
The RPATHs are corrected for the installed layout of the wheels, and platform repair steps still use `auditwheel`, `delocate`, and
`build_scripts/pypi/updatePluginfos.py`.

The backend is designed to be extensible and support more package definitions.


## Packages

The number and content of the packages is easily configurable, the following separation is the current implementation:

- `packaging/usd-core`
- `packaging/usd-imaging`
- `packaging/usd-tools`
- `packaging/usdview`

Each package owns its corresponding shared libraries, bindings, python modules, resources, and metadata.

These packages are staged from an installed OpenUSD tree using explicit manifest globs.
The file ownership boundaries are intentionally easy to revise during review.

On Linux, native shared-library ownership now follows the package dependency graph during repair:

- `usd-core` repairs as a self-contained base wheel
- higher-level wheels exclude libraries already exported by their OpenUSD
  dependencies
- the backend patches ELF `NEEDED` entries and RPATHs so later wheels resolve
  back into sibling dependency `*.libs` directories after install

That keeps `usd-imaging`, `usd-tools`, and `usdview` from vendoring duplicate
copies of the same OpenUSD shared libraries.

For command-line tools, the wheel keeps the real executable under
`site-packages` and exposes the usual command name through a generated Python
launcher. That avoids relying on `pip` to preserve executable-to-library
relative paths when it installs scripts into the environment's `bin/`
directory.

## Local Build Entry Point

The backend supports two build modes:

- Build OpenUSD directly by invoking `build_scripts/build_usd.py`.
- Reuse a prebuilt install tree by setting `OPENUSD_PREBUILT_INSTALL_ROOT`.

The second mode is intended for CI integration where the install tree is
already produced elsewhere in the workflow.

## Helper Script

`packaging/build_wheels.sh` is a convenience entry point for local development.
It will:

- create a virtual environment,
- install the packaging prerequisites,
- build one shared OpenUSD install tree unless `--reuse-install` is set,
- build one or more wheels into a chosen output directory.

Example usage:

```bash
packaging/build_wheels.sh
packaging/build_wheels.sh --packages usd-core,usd-imaging
packaging/build_wheels.sh --reuse-install --install-root /tmp/openusd-inst
packaging/build_wheels.sh --build-usd-arg --no-tests
```

The helper installs the wheel-building tools it needs, and any necessary pip
packages for OpenUSD. That currently includes `Jinja2` when `usd-tools` is
requested so the schema-generation tools such as `usdGenSchema` and
`usdInitSchema` are built, and the Qt-for-Python dependencies needed for
`usdview`.

On Linux, the repair step uses `auditwheel` and requires `patchelf`. The helper
installs both into the packaging virtual environment when repair is enabled.

## Alternatives

### Common namespace packages

There are a subset of the OpenUSD libraries on pypi as individual packages (pxr-ar, pxr-sdf, etc.).
These all install into a common site-packages/pxr/.libs tree, eliminating the need for wheel repair.
There are downsides, though. Ownership of the files is not managed, uninstallation/upgrade of the packages is brittle,
and there are collision risks.

Because additional wheel build configuration is needed anyways to support the CLI tools because RPATHs will be broken due to default install layouts,
this approach was not used here. 

### Single Runtime Package

Similar pypi projects sometimes use a single runtime package with all native code in it, and packages built on top of it are pure python/resources/metadata.
