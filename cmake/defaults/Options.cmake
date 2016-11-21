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
option(PXR_STRICT_BUILD_MODE "Turn on additional warnings. Enforce all warnings as errors." OFF)
option(PXR_VALIDATE_GENERATED_CODE "Validate script generated code" OFF)
option(PXR_BUILD_TESTS "Build tests" ON)
option(PXR_BUILD_IMAGING "Build imaging components" ON)
option(PXR_BUILD_USD_IMAGING "Build USD imaging components" ON)
option(PXR_BUILD_KATANA_PLUGIN "Build usd katana plugin" ON)
option(PXR_BUILD_MAYA_PLUGIN "Build usd maya plugin" ON)
option(PXR_BUILD_ALEMBIC_PLUGIN "Build the Alembic plugin for USD" ON)
option(PXR_MAYA_TBB_BUG_WORKAROUND "Turn on linker flag (-Wl,-Bsymbolic) to work around a Maya TBB bug" OFF)
option(PXR_HYBRID_BUILD_MODE "Build USD in release with symbols and optimisations disabled" ON)

set(PXR_INSTALL_LOCATION ""
    CACHE
    STRING
    "Intended final location for plugin resource files."
)

set(PXR_ALL_LIBS ""
    CACHE
    INTERNAL
    "Aggregation of all built libraries."
)
set(PXR_LIB_PREFIX "lib"
    CACHE
    STRING
    "Prefix for build library name"
)
if (${PXR_BUILD_USD_IMAGING} AND NOT ${PXR_BUILD_IMAGING})
    message(STATUS
        "PXR_BUILD_IMAGING=OFF implies PXR_BUILD_USD_IMAGING=OFF"
    )
endif()

