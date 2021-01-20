#!/bin/bash

clear
echo "Building USD"
export USD_VERSION=20
export USD_ROOT=/usr/local/USD.$USD_VERSION

echo "Building USD Core"
python build_usd.py -v -v $USD_ROOT/USD-core
echo "Done building USD! Happy times!"
