#
# Copyright 2020 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#
import setuptools

import glob, os, platform, re, shutil

# This setup.py script expects to be run from an inst directory in a typical
# USD build run from build_usd.py.
#
# The output of this setup will be a wheel package that does not work. After
# building the wheel package, you will also need to relocate dynamic library
# dependencies into the package, and you will need to update the pluginfo files
# to point to the new locations of those shared library dependencies. How this
# is done depends on platform, and is mostly accomplished by steps in the CI
# system.

def windows():
    return platform.system() == "Windows"

WORKING_ROOT = '.'
USD_BUILD_OUTPUT = os.path.join(WORKING_ROOT, 'inst')
BUILD_DIR = os.path.join(WORKING_ROOT, 'pypi')

# Copy everything in lib over before we start making changes
shutil.copytree(os.path.join(USD_BUILD_OUTPUT, 'lib'), os.path.join(BUILD_DIR, 'lib'))

# Move the pluginfos into a directory contained inside pxr, for easier
# distribution. This breaks the relative paths in the pluginfos, but we'll need
# to update them later anyway after running "auditwheel repair", which will
# move the libraries to a new directory
shutil.move(os.path.join(BUILD_DIR, 'lib/usd'), os.path.join(BUILD_DIR, 'lib/python/pxr/pluginfo'))

if windows():
    # On windows we also need dlls from the bin directory
    shutil.copytree(os.path.join(USD_BUILD_OUTPUT, 'bin'), os.path.join(BUILD_DIR, 'bin'))

    # On Linux and Mac there are tools that do this for us (auditwheel and
    # delocate) On Windows we'll move these here in setup. This is simpler
    # because there are no RPATHs to worry about.
    dll_files = glob.glob(os.path.join(BUILD_DIR, "lib/*.dll"))
    dll_files.extend(glob.glob(os.path.join(BUILD_DIR, "bin/*.dll")))
    for f in dll_files:
        shutil.move(f, os.path.join(BUILD_DIR, "lib/python/pxr"))

    # Because there are no RPATHS, patch __init__.py
    # See this thread and related conversations
    # https://mail.python.org/pipermail/distutils-sig/2014-September/024962.html
    with open(os.path.join(BUILD_DIR, 'lib/python/pxr/__init__.py'), 'a+') as init_file:
        init_file.write('''

# appended to this file for the windows PyPI package
import os, sys
dllPath = os.path.split(os.path.realpath(__file__))[0]
if sys.version_info >= (3, 8, 0):
    os.add_dll_directory(dllPath)
else:
    os.environ['PATH'] = dllPath + os.pathsep + os.environ['PATH']
''')

# Get the readme text
with open("README.md", "r") as fh:
    long_description = fh.read()

# Get the library version number from the installed pxr.h header.
with open(os.path.join(USD_BUILD_OUTPUT, "include/pxr/pxr.h"), "r") as fh:
    for line in fh:
        m = re.match("#define PXR_MINOR_VERSION (\d+)", line)
        if m:
            minorVersion = m.groups(1)[0]
            continue

        m = re.match("#define PXR_PATCH_VERSION (\d+)", line)
        if m:
            patchVersion = m.groups(1)[0]
            continue

version = "{}.{}".format(minorVersion, patchVersion)

# Config
setuptools.setup(
    name="usd-core",
    version=version,
    author="Pixar Animation Studios",
    author_email="pixar.oss+usd_pypi@gmail.com",
    description="Pixar's Universal Scene Description library",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://graphics.pixar.com/usd/docs/index.html",
    project_urls={
        "Documentation": "https://graphics.pixar.com/usd/docs/index.html",
        "Developer Docs": "https://graphics.pixar.com/usd/docs/USD-Developer-API-Reference.html",
        "Source": "https://github.com/PixarAnimationStudios/USD",
        "Discussion Group": "https://groups.google.com/g/usd-interest"
    },
    packages=setuptools.find_packages(os.path.join(BUILD_DIR, 'lib/python')),
    package_dir={"": os.path.join(BUILD_DIR, 'lib/python')},
    package_data={
        "": ["*.so", "*.dll", "*.pyd"],
        "pxr": ["pluginfo/*", "pluginfo/*/*", "pluginfo/*/*/*"],
    },
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: Other/Proprietary License",
        "Operating System :: POSIX :: Linux",
        "Operating System :: MacOS :: MacOS X",
        "Operating System :: Microsoft :: Windows :: Windows 10",
        "Environment :: Console",
        "Topic :: Multimedia :: Graphics",
    ],
    python_requires='>=3.6, <3.8',
)
