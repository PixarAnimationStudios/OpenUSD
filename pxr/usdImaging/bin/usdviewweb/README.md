# What is USDViewWeb?

It is a simple application for testing USD Hydra Storm raster renderer with HGI WebGPU backend in web browser.

# How to build and deploy?

## Get the emscripten SDK

Download and Install emscripten from [HERE](https://emscripten.org/docs/getting_started/downloads.html). (FYI [Official web site](https://emscripten.org))

Set up the environment (i.e. source the required emsdk_env script). Current experiment and testing are based on emsdk v3.1.33.

### MacOS:

```
# Download and install the latest SDK tools.
./emsdk install 3.1.33

# Make the specific SDK "active" for the current user. (writes .emscripten file)
./emsdk activate 3.1.33

# Activate PATH and other environment variables in the current terminal
source ./emsdk_env.sh
```

### Windows:

```
# Download and install the latest SDK tools.
emsdk install 3.1.33

# Make the specific SDK for the current user. (writes .emscripten file)
emsdk activate 3.1.33
```

## Build USD to WebAssembly

```
# Clone the USD source code
git clone --recursive https://git.autodesk.com/autodesk-forks/usd/tree/adsk/feature/webgpu

# Go into the root of usd source repo, if the folder name is "usd_repo"
cd usd_repo

# Build USD with the --emscripten flag, for example "./build_dir" is your local build folder
python3 ./build_scripts/build_usd.py --emscripten ./build_dir
```

This also builds UsdViewWeb application that you could use to test Hydra Storm with WebGPU backend.

## Set up web service

```
# Start a web server in the build dir with the appropriate flags, easiest to just use the python script here: scripts/wasm-server.py Go into the TestHgiWebGPU folder first, make sure it is python 3.X
python3 scripts/wasm-server.py
```

## Browser Requirements

- Chrome v114 and upper version

## Launch Storm in browser

Launch Chrome browser and navigate to the test home page for HdStorm pipeline: http://localhost:8080/usdviewweb.html. The html file usdviewweb.html is located in "./build_dir/bin".

