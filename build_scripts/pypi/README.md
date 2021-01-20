# PyPI Build and Packaging Scripts

This directory contains scripts and other files used to make wheel packages for
distributing via standard python applications like pip.

The build is set up to run on azure-pipelines, and can be triggered by changing
any of the files here and committing to dev or release.

## Generating New Packages

To start a new package build, first update any parameters that need to be
updated.  This may include things like the version number or supported python
versions in setup.py. At a minimum you should increase the version number for
each build, so that pip can manage installs and upgrades as expected (see
[PEP440](https://www.python.org/dev/peps/pep-0440)).

Other parameters in setup.py are 
[documented here](https://packaging.python.org/guides/distributing-packages-using-setuptools).

Once these changes are committed run the pipeline. It will create an artifact
called `dist` that contains the wheel packages for each platform and python
version. The pipeline tests that these can be installed and used, to test
locally you can download the dist artifact, extract it, and run

```
python -m pip install --no-index --find-links=file:///path/to/dist usd-core
```

## Steps to Publish

From the azure pipeline download the artifact file called dist.zip. On a Linux
host you could publish like so, in the directory with the dist file.

```
unzip dist.zip
python3 -m pip install twine
python3 -m twine upload --repository-url https://test.pypi.org/legacy/ dist/*
```

The name, version number and other properties of your package will be extracted
from the whl files that are uploaded. The above publishes to the test server,
to publish to the main PyPI archive remove the `repository-url` parameter.

## Steps to Add Support For New Python Versions

To support a new version of python 3 it should be as easy as adding a new build
to the matrix for each platform you want to support. If the build environment has
support for the python version, just updating those string parameters should be
all it takes. setup.py should also be updated to indicate that this package 
supports the new version of python.

Support for python 2.7 would be similar, but on Windows it may require getting
a [specific version of visual studio](https://wiki.python.org/moin/WindowsCompilers)

## Steps to Update Docker Container on Linux

Linux builds are done in [Python's manylinux build
environment](https://github.com/pypa/manylinux). This helps guarantee that we
can run the binaries on many different linux distros. The default build
environment can be used in a docker container, but the container does not
include cmake in a compatible version for building USD. Also, azure pipelines
has some specific requirements for docker images that need to be met to build
there.

For that reason we have a docker environment defined in the docker folder in
this directory. If that Dockerfile is updated, it must also be built, uploaded
to a docker registry where azure can find it, and the pipeline xml file may
need to be updated to point to the new docker image when building.

For example, after editing the Dockerfile
```
cd build_scripts/pypi/docker
docker build . -t usd-build-env-name:v2.0
docker push usd-build-env-name:v2.0
```
Then update the azure pipeline xml file to change from
```
container:
  image: old-env:latest
```
To
```
container:
  image: usd-build-env-name:v2.0
```
That should kick off a new packaging build using the new docker image.
