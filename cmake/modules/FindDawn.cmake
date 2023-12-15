# Copyright (c) 2020 Sony Interactive Entertainment Inc.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name of Intel Corporation nor the names of its contributors may
#   be used to endorse or promote products derived from this software without
#   specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# Try to find Dawn include and library directories.
#
# After successful discovery, this will set for inclusion where needed:
# Dawn_INCLUDE_DIRS - containing the Dawn headers
# Dawn_LIBRARIES - containing the Dawn library
#[=======================================================================[.rst:
FindDawn
--------------
Find Dawn headers and libraries.
Imported Targets
^^^^^^^^^^^^^^^^
``Dawn::dawn``
  The Dawn library, if found.
``Dawn::native``
  The Dawn Native library, if found.
``Dawn::wire```
  The Dawn Wire library, if found.
``Dawn::platform```
  The Dawn platform library, if found.
``Dawn::cpp```
  The Dawn CPP library, if found.
Result Variables
^^^^^^^^^^^^^^^^
This will define the following variables in your project:
``Dawn_FOUND``
  true if (the requested version of) Dawn is available.
``Dawn_LIBRARIES``
  the libraries to link against to use Dawn.
``Dawn_INCLUDE_DIRS``
  where to find the Dawn headers.
``Dawn_COMPILE_OPTIONS``
  this should be passed to target_compile_options(), if the
  target is not used for linking
#]=======================================================================]
find_path(WebGPU_INCLUDE_DIR
        NAMES webgpu/webgpu.h
        PATHS ${Dawn_ROOT}/include
)
find_path(Dawn_INCLUDE_DIR
        NAMES dawn/dawn_proc.h
        PATHS ${Dawn_ROOT}/include
)
find_library(Dawn_LIBRARY
        NAMES ${Dawn_NAMES} dawn_proc
        PATHS ${Dawn_ROOT}/lib
)
find_library(WebGPU_Dawn_LIBRARY
        NAMES webgpu_dawn
        PATHS ${Dawn_ROOT}/lib
)
# Find components
set(Dawn_LIBRARIES)
if (WebGPU_INCLUDE_DIR AND Dawn_INCLUDE_DIR AND Dawn_LIBRARY AND WebGPU_Dawn_LIBRARY)
    set(Dawn_LIBRARIES ${Dawn_LIBRARY} ${WebGPU_Dawn_LIBRARY})
    if (WIN32)
        find_library(Dawn_common_LIBRARY
                NAMES dawn_common
                PATHS ${Dawn_ROOT}/lib
        )
        if(Dawn_common_LIBRARY)
            set(_Dawn_REQUIRED_LIBS_FOUND ON)
            list(APPEND Dawn_LIBRARIES ${Dawn_common_LIBRARY})
            get_filename_component(Dawn_LIBRARY_DIR "${Dawn_LIBRARY}" DIRECTORY)
            set(Dawn_LIBS_FOUND "Dawn (required): ${Dawn_LIBRARY} ${WebGPU_Dawn_LIBRARY} ${Dawn_common_LIBRARY}")
        else()
            set(_Dawn_REQUIRED_LIBS_FOUND OFF)
        endif()

    else()
        set(_Dawn_REQUIRED_LIBS_FOUND ON)
        get_filename_component(Dawn_LIBRARY_DIR "${Dawn_LIBRARY}" DIRECTORY)
        set(Dawn_LIBS_FOUND "Dawn (required): ${Dawn_LIBRARY} ${WebGPU_Dawn_LIBRARY}")
    endif()
else ()
    set(_Dawn_REQUIRED_LIBS_FOUND OFF)
    set(Dawn_LIBS_NOT_FOUND "Dawn (required)")
endif ()

set(Dawn_INCLUDE_DIRS)
set(REQUESTED_DAWN_COMPONENTS ${Dawn_FIND_COMPONENTS})

if ("cpp" IN_LIST REQUESTED_DAWN_COMPONENTS)
    list(REMOVE_ITEM REQUESTED_DAWN_COMPONENTS "cpp")

    find_path(WebGPU_CPP_INCLUDE_DIR
            NAMES webgpu/webgpu_cpp.h
            PATHS ${Dawn_ROOT}/include ${WebGPU_INCLUDE_DIR}
    )
    find_library(WebGPU_CPP_LIBRARY
            NAMES dawncpp
            PATHS ${Dawn_ROOT}/lib ${Dawn_LIBRARY}
    )
    if (WebGPU_CPP_INCLUDE_DIR AND WebGPU_CPP_LIBRARY)
        list(APPEND Dawn_LIBRARIES ${WebGPU_CPP_LIBRARY})
        list(APPEND Dawn_INCLUDE_DIRS ${WebGPU_CPP_INCLUDE_DIR})
        if (Dawn_FIND_REQUIRED_cpp)
            list(APPEND Dawn_LIBS_FOUND "dawncpp (required): ${WebGPU_CPP_LIBRARY}")
        else ()
            list(APPEND Dawn_LIBS_FOUND "dawncpp (optional): ${WebGPU_CPP_LIBRARY}")
        endif ()
    else ()
        if (Dawn_FIND_REQUIRED_cpp)
            set(_Dawn_REQUIRED_LIBS_FOUND OFF)
            list(APPEND Dawn_LIBS_NOT_FOUND "dawncpp (required)")
        else ()
            list(APPEND Dawn_LIBS_NOT_FOUND "dawncpp (optional)")
        endif ()
    endif ()
endif ()

set(AVAILABLE_COMPONENTS native wire platform)
foreach(COMPONENT ${REQUESTED_DAWN_COMPONENTS})
    if (${COMPONENT} IN_LIST AVAILABLE_COMPONENTS)
        find_path(Dawn_${COMPONENT}_INCLUDE_DIR
                NAMES dawn/${COMPONENT}/dawn_${COMPONENT}_export.h
                PATHS ${Dawn_INCLUDE_DIR} ${Dawn_ROOT}/include
        )
        find_library(Dawn_${COMPONENT}_LIBRARY
                NAMES dawn_${COMPONENT}
                PATHS ${Dawn_LIBRARY_DIR}
        )
        if (Dawn_${COMPONENT}_INCLUDE_DIR AND Dawn_${COMPONENT}_LIBRARY)
            list(APPEND Dawn_LIBRARIES ${Dawn_${COMPONENT}_LIBRARY})
            list(APPEND Dawn_INCLUDE_DIRS ${Dawn_${COMPONENT}_LIBRARY})
            add_library(Dawn::${COMPONENT} UNKNOWN IMPORTED GLOBAL)
            set_target_properties(Dawn::${COMPONENT} PROPERTIES
                    IMPORTED_LOCATION "${Dawn_${COMPONENT}_LIBRARY}"
                    INTERFACE_INCLUDE_DIRECTORIES "${Dawn_INCLUDE_DIR};${Dawn_${COMPONENT}_INCLUDE_DIR}"
            )
            if (Dawn_FIND_REQUIRED_${COMPONENT})
                list(APPEND Dawn_LIBS_FOUND "${COMPONENT} (required): ${Dawn_${COMPONENT}_LIBRARY}")
            else ()
                list(APPEND Dawn_LIBS_FOUND "${COMPONENT} (optional): ${Dawn_${COMPONENT}_LIBRARY}")
            endif ()
        else ()
            if (Dawn_FIND_REQUIRED_${COMPONENT})
                set(_Dawn_REQUIRED_LIBS_FOUND OFF)
                list(APPEND Dawn_LIBS_NOT_FOUND "${COMPONENT} (required)")
            else ()
                list(APPEND Dawn_LIBS_NOT_FOUND "${COMPONENT} (optional)")
            endif ()
        endif ()
        mark_as_advanced(
                Dawn_${COMPONENT}_INCLUDE_DIR
                Dawn_${COMPONENT}_LIBRARY
        )
    else()
        message(FATAL_ERROR "Unknown component ${COMPONENT}")
    endif ()
endforeach ()

if (NOT Dawn_FIND_QUIETLY)
    if (Dawn_LIBS_FOUND)
        message(STATUS "Found the following Dawn libraries:")
        foreach (found ${Dawn_LIBS_FOUND})
            message(STATUS " ${found}")
        endforeach ()
    endif ()
    if (Dawn_LIBS_NOT_FOUND)
        message(STATUS "The following Dawn libraries were not found:")
        foreach (found ${Dawn_LIBS_NOT_FOUND})
            message(STATUS " ${found}")
        endforeach ()
    endif ()
endif ()
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Dawn
        FOUND_VAR Dawn_FOUND
        REQUIRED_VARS
        WebGPU_INCLUDE_DIR
        Dawn_INCLUDE_DIR
        Dawn_LIBRARY
        _Dawn_REQUIRED_LIBS_FOUND
)
