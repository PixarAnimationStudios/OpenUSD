#!/usr/bin/env bash

# To be sourced

export ROOT=$PWD

if [[ $PWD == */bin ]]; then
    export ROOT=$ROOT/..
fi

export INSTALL=$ROOT/install


export BUILD=$INSTALL/build/OpenUSD

export PYTHONPATH=$INSTALL/lib/python:$PYTHONPATH
export PXR_PLUGINPATH_NAME=$INSTALL/share/usd/examples/plugin/usdotio:$PXR_PLUGINPATH_NAME
export PATH=$INSTALL/bin:$PATH

