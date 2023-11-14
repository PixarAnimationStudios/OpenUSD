# What is USDViewWeb?

It is a simple application for testing USD Hydra Storm raster renderer with HGI WebGPU backend in web browser.

# How to build and deploy?

## Get the emscripten SDK

Download and Install [emscripten](https://emscripten.org) from [HERE](https://emscripten.org/docs/getting_started/downloads.html).

Set up the environment (i.e. source the required emsdk_env script). Current experiment and testing are based on emsdk v3.1.47.

### MacOS:

```
# Download and install the latest SDK tools.
./emsdk install 3.1.47

# Make the specific SDK "active" for the current user. (writes .emscripten file)
./emsdk activate 3.1.47

# Activate PATH and other environment variables in the current terminal
source ./emsdk_env.sh
```

### Windows:

```
# Download and install the latest SDK tools.
emsdk install 3.1.47

# Make the specific SDK for the current user. (writes .emscripten file)
emsdk activate 3.1.47
```

## Build USD to WebAssembly

```
# Clone the USD source code
git clone --recursive https://git.autodesk.com/autodesk-forks/usd/tree/adsk/feature/webgpu

# Go into the root of usd source repo, if the folder name is "usd_repo"
cd usd_repo

# Build USD with the --emscripten flag, for example "../build_dir" is your local build folder
python3 ./build_scripts/build_usd.py --emscripten ../build_dir
```

This also builds the UsdViewWeb application, which you can use to test Hydra Storm with the WebGPU backend.

## Set up web service
Start a web server in the build dir with the appropriate flags.

Go into the <build_dir>/bin folder first.

```
python ./wasm-server.py
```

## Browser Requirements

- Chrome v114 and upper version

## Launch Storm in browser
Launch Chrome browser and navigate to the HdStorm web example: http://localhost:8080/usdviewweb.html. 


