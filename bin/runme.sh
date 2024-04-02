#!/usr/bin/env bash

export ROOT=$PWD

if [[ $PWD == */bin ]]; then
    export ROOT=$ROOT/..
fi

python3.10 $ROOT/build_scripts/build_usd.py $ROOT/install --build-args USD,"-G Ninja -DPXR_BUILD_TESTS=ON -DPXR_BUILD_EXAMPLES=OFF" --verbose
