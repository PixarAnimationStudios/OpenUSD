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

# This file contains a set of flags/settings shared between our 
# GCC and Clang configs. This allows clangdefaults and gccdefaults
# to remain minimal, marking the points where divergence is required.
include(Options)

# Turn on C++11; pxr won't build without it. 
set(_PXR_GCC_CLANG_SHARED_CXX_FLAGS "${_PXR_GCC_CLANG_SHARED_CXX_FLAGS} -std=c++11")

# Enable all warnings.
set(_PXR_GCC_CLANG_SHARED_CXX_FLAGS "${_PXR_GCC_CLANG_SHARED_CXX_FLAGS} -Wall")

# Errors are warnings in strict build mode.
if (${PXR_STRICT_BUILD_MODE})
    set(_PXR_GCC_CLANG_SHARED_CXX_FLAGS "${_PXR_GCC_CLANG_SHARED_CXX_FLAGS} -Werror")
endif()

# We use hash_map, suppress deprecation warning.
_disable_warning("deprecated")
_disable_warning("deprecated-declarations")

# Suppress unused typedef warnings emanating from boost.
_disable_warning("unused-local-typedefs")

if (${PXR_MAYA_TBB_BUG_WORKAROUND})
    set(_PXR_GCC_CLANG_SHARED_CXX_FLAGS "${_PXR_GCC_CLANG_SHARED_CXX_FLAGS} -Wl,-Bsymbolic")
endif()
