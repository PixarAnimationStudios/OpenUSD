#
# Copyright 2016 Pixar
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

# Make sure WinDef.h does not define min and max macros which
# will conflict with std::min() and std::max().
add_definitions("/DNOMINMAX")

# TBB conflict
# This needs to be enabled only for debug builds
# add_definitions("/DTBB_USE_DEBUG=1")

# Needed for consistency with Linux for the use of hashmap and hashset
add_definitions("/DTF_NO_GNU_EXT")

# Needed to prevent YY files trying to import Unistd.h
# (which doesn't exist on Windows)
add_definitions("/DYY_NO_UNISTD_H")

# Forces all libraries that have separate source, to be linked as
# DLL's rather than static libraries on Microsoft Window.
add_definitions("/DBOOST_ALL_DYN_LINK")

add_definitions("/DBUILD_OPTLEVEL_OPT")

add_definitions(/"DFLAVOR=\"win64\"")

add_definitions(/"DOPENEXR_DLL")

# Set a decent warning level
add_definitions("/W3")

# trunctation from 'double' to 'float' due to matrix and vector classes in `Gf`
add_definitions("/wd4244")
add_definitions("/wd4305")

# conversion from size_t to int. Whilst we don't want this enabled
# its in the Python headers. So all the Python wrap code is affected.
add_definitions("/wd4267")

# no definition for inline function
# this affects Glf only
add_definitions("/wd4506")

# Enable PDB generation
add_definitions("/Zi")

# These files require /bigobj compiler flag
#   Vt/arrayPyBuffer.cpp
#   Usd/crateFile.cpp
#   Usd/stage.cpp
# Until we can set the flag on a per file basis, we'll have to enable it
# for all translation units.
add_definitions("/bigobj")

if (${PXR_STRICT_BUILD_MODE})
    # Treat all warnings as errors
    add_definitions("/WX")
endif()

if (${PXR_HYBRID_BUILD_MODE})
    # Effectively release with symbols
    add_definitions("/Od")
    add_definitions("/Ob0")

    # Enable minimum builds
    add_definitions("/Gm")
else()
    # Enable multi-processor compilation
    add_definitions("/MP")
endif()
