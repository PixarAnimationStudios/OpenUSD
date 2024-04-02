#!/usr/bin/env bash

set -x

export ROOT=$PWD

if [[ $PWD == */bin ]]; then
    export ROOT=$ROOT/..
fi

. $ROOT/bin/env.sh

cp -r $INSTALL/bin/usdotio $ROOT/pxr/usd/bin/usdotio/usdotio.py

sed -i -e 's%#!/usr/bin/python.?%#!/pxrpythonsubst%' $ROOT/pxr/usd/bin/usdotio/usdotio.py

cp -r $INSTALL/lib/python/usdotio/* $ROOT/pxr/usd/bin/usdotio/lib/python

#
# Remove __pycache__ directories
#
rm -rf $ROOT/pxr/usr/bin/usdotio/lib/python/__pycache__
rm -rf $ROOT/pxr/usr/bin/usdotio/lib/python/schema/__pycache__
