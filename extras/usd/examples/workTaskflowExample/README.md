workTaskflowExample
===================
`workTaskflowExample` is an example demonstrating how to create and build USD
against a custom task management implementation. It is an example only and is
unsupported for general use.

`workTaskflowExample` is based on the open-source `Taskflow` library available
at https://github.com/taskflow/taskflow.

## Building `workTaskflowExample`

Run cmake to configure and build the library. With the example command line
below, its headers and shared library will be installed under
`/path/to/install`.

```
cmake --install-prefix /path/to/install -B /path/to/build -S OpenUSD/pxr/extras/usd/examples/workTaskflowExample
cmake --build /path/to/build --target install --config Release
```

By default, the `Taskflow` library will automatically be downloaded and
installed under the same location as `workTaskflowExample` itself. This can
be overridden to use a pre-existing installation by setting the
`Taskflow_DIR` option to point to the directory containing its cmake package
config file.

## Building USD with `workTaskflowExample`

Specify `workTaskflowExample` as the custom task management implementation
by setting the `PXR_WORK_IMPL` option to `workTaskflowExample` when running
cmake. The `workTaskflowExample_DIR` option must also be set to the directory
containing the package config file named `workTaskflowExampleConfig.cmake`
that was installed as part of the build above.

```
cmake -DPXR_WORK_IMPL=workTaskflowExample -DworkTaskflowExample_DIR=/path/to/install/lib64/cmake/workTaskflowExample ...
```

If you are building USD with build_usd.py instead of using cmake directly,
these arguments can be passed through using `--build-args`:

```
build_usd.py --build-args USD,"-DPXR_WORK_IMPL=workTaskflowExample -DworkTaskflowExample_DIR=/path/to/install/lib64/cmake/workTaskflowExample" ...
```
