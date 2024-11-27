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

# Disable warning C4996 regarding fopen(), strcpy(), etc.
_add_define("_CRT_SECURE_NO_WARNINGS")

# Disable warning C4996 regarding unchecked iterators for std::transform,
# std::copy, std::equal, et al.
_add_define("_SCL_SECURE_NO_WARNINGS")

# Make sure WinDef.h does not define min and max macros which
# will conflict with std::min() and std::max().
_add_define("NOMINMAX")

# Needed to prevent YY files trying to include unistd.h
# (which doesn't exist on Windows)
_add_define("YY_NO_UNISTD_H")

# Forces all libraries that have separate source to be linked as
# DLL's rather than static libraries on Microsoft Windows, unless
# explicitly told otherwise.
if (NOT Boost_USE_STATIC_LIBS)
    _add_define("BOOST_ALL_DYN_LINK")
endif()

# Suppress automatic boost linking via pragmas, as we must not rely on
# a heuristic, but upon the tool set we have specified in our build.
_add_define("BOOST_ALL_NO_LIB")

if(${PXR_USE_DEBUG_PYTHON})
    _add_define("BOOST_DEBUG_PYTHON")
    _add_define("BOOST_LINKING_PYTHON")
endif()

# Need half::_toFloat and half::_eLut.
_add_define("OPENEXR_DLL")

# Exclude headers from unnecessary Windows APIs to improve build
# times and avoid annoying conflicts with macros defined in those
# headers.
_add_define("WIN32_LEAN_AND_MEAN")
