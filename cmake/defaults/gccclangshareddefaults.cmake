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

# By default, Release flavor builds in cmake set NDEBUG, which
# breaks things internally.  Turn it off.
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

# Enable all warnings
_add_warning_flag("all")
# We use hash_map, suppress deprecation warning.
_add_warning_flag("no-deprecated")
_add_warning_flag("no-deprecated-declarations")
# Suppress unused typedef warnings eminating from boost.
_add_warning_flag("no-unused-local-typedefs")

# Turn on C++11, pxr won't build without it. 
set(_PXR_GCC_CLANG_SHARED_CXX_FLAGS "-std=c++11")
