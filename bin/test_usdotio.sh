#!/usr/bin/env bash

test_usdotio()
{
    otio=$1
    usd=$2

    usd_out=/tmp/test.usda
    otio_out=/tmp/test.otio

    otio_cat=/tmp/orig_cat.otio
    otio_out_cat=${otio_out}

    echo "Creating ${usd_out}"
    usdotio add -o "${usd_out}" "${otio}" "${usd}" -y -n -v quiet
    
    rm -f "${otio_out}"
    
    usdotio save "${otio_out}" "${usd_out}" -v quiet

    otiocat "${otio}" > "${otio_cat}"

    echo -n "Testing ${otio_out} ${otio} ....."
    error=`diff -w ${otio_cat} ${otio_out_cat}`
    if [[ "${error}" != "" ]]; then
	echo "FAILED"
	echo "usd file ${usd_out}"
	echo "${otio}" "${otio_out}"
	meld "${otio_cat}" "${otio_out_cat}"
	exit 1
    else
	echo "PASSED"
    fi
}


export ROOT=$PWD

if [[ $PWD == */bin ]]; then
    export ROOT=$ROOT/..
fi

. $ROOT/bin/env.sh

cd ~/Movies
for i in *.otio; do
    test_usdotio $i ~/assets/sphere.usda
done

cd ~/code/applications/tlRender/etc/SampleData
for i in *.otio; do
    test_usdotio $i ~/assets/sphere.usda
done
