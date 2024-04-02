#!/usr/bin/env bash


export ROOT=$PWD

if [[ $PWD == */bin ]]; then
    export ROOT=$ROOT/..
fi

. $ROOT/bin/env.sh

cd $ROOT

rm -rf $INSTALL/lib/python/usdotio/__pycache__
rm -rf $INSTALL/lib/python/usdotio/schema/__pycache__

cd $INSTALL
zip -r usdotio.zip bin/usdotio bin/usdotio.cmd lib/python/usdotio share/usd/examples/plugin/usdotio scripts/usdotio.bat usdotio_README.txt
