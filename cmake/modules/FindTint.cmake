#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
# Try to find Tint include and library directories.
#
# After successful discovery, this will set for inclusion where needed:
# TINT_INCLUDE_DIRS - containing the Tint headers
# TINT_LIBRARIES - containing Tint library
#[=======================================================================[.rst:
FindTint
--------------
Find Tint headers and libraries.
This file highly depends on what tint commit it is being tested on.
The last revision was for the commit 5b68c1266eb91bd29c26751a820919f1afa31260
Imported Targets
^^^^^^^^^^^^^^^^
``Tint::api``
  The Tint library, if found.
Result Variables
^^^^^^^^^^^^^^^^
This will define the following variables in your project:
``Tint_FOUND``
  true if (the requested version of) Tint is available.
``Tint_LIBRARIES``
  the libraries to link against to use Tint.
``Tint_INCLUDE_DIRS``
  where to find the Tint headers.
#]=======================================================================]

find_path(Tint_API_INCLUDE_DIR
        NAMES tint/tint.h
        PATHS ${Tint_ROOT}/include
        $ENV{Tint_ROOT}/include
        NO_CMAKE_FIND_ROOT_PATH
)
find_path(Tint_SRC_INCLUDE_DIR
        NAMES src/tint/api/tint.h
        PATHS ${Tint_ROOT}/include
        $ENV{Tint_ROOT}/include
        NO_CMAKE_FIND_ROOT_PATH
)

find_library(Tint_LIBRARY
        NAMES tint_api
        PATHS ${Tint_ROOT}/lib
        $ENV{Tint_ROOT}/lib
        NO_CMAKE_FIND_ROOT_PATH
)
set(Tint_LIBRARIES)
if (Tint_API_INCLUDE_DIR AND Tint_SRC_INCLUDE_DIR AND Tint_LIBRARY)
    set(Tint_INCLUDE_DIRS ${Tint_API_INCLUDE_DIR} ${Tint_SRC_INCLUDE_DIR})
    set(Tint_LIBRARIES ${Tint_LIBRARY})
    get_filename_component(TINT_LIBRARY_DIR "${Tint_LIBRARY}" DIRECTORY)
    add_library(Tint::api UNKNOWN IMPORTED GLOBAL)
    set_target_properties(Tint::api PROPERTIES
            IMPORTED_LOCATION "${Tint_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${Tint_API_INCLUDE_DIR};${Tint_SRC_INCLUDE_DIR}"
    )
endif ()

unset(Tint_FOUND CACHE)
set(Tint_FOUND ON)

set(TINT_WRITER_COMPONENTS
        printer
        raise
        common
)

set(TINT_READER_COMPONENTS
        ast_lower
        ast_parser
        common
)

set(TINT_COMMON_COMPONENTS
        api_common
        lang_core
        lang_core_constant
        lang_core_intrinsic
        lang_core_ir
        lang_core_ir_analysis
        lang_core_ir_transform
        lang_core_ir_type
        lang_core_type
        # TODO: Are all these libraries required in all cases?
        lang_wgsl
        lang_wgsl_ast
        lang_wgsl_ast_transform
        lang_wgsl_inspector
        lang_wgsl_intrinsic
        lang_wgsl_ir
        lang_wgsl_program
        lang_wgsl_reader
        lang_wgsl_reader_lower
        lang_wgsl_reader_parser
        lang_wgsl_reader_program_to_ir
        lang_wgsl_resolver
        lang_wgsl_sem
        lang_wgsl_writer
        lang_wgsl_writer_ast_printer
        lang_wgsl_writer_ir_to_program
        lang_wgsl_writer_raise
        lang_wgsl_writer_syntax_tree_printer
        lang_spirv
        lang_spirv_ir
        lang_spirv_intrinsic
        lang_spirv_reader_ast_parser
        lang_spirv_reader_common
        lang_spirv_type
        utils
        utils_containers
        utils_diagnostic
        utils_ice
        utils_macros
        utils_math
        utils_memory
        utils_rtti
        utils_strconv
        utils_symbol
        utils_system
        utils_text
        utils_text_generator
)

set(TINT_REQUESTED_COMPONENTS
    ${Tint_FIND_COMPONENTS}
    ${TINT_COMMON_COMPONENTS}
)

unset(Tint_LIBRARIES_NOT_FOUND)
foreach(COMPONENT ${TINT_REQUESTED_COMPONENTS})
    find_library(TINT_${COMPONENT}_LIBRARY
            NAMES tint_${COMPONENT}
            HINTS ${TINT_LIBRARY_DIR}
            NO_DEFAULT_PATH
            NO_CMAKE_FIND_ROOT_PATH
    )
    if (TINT_${COMPONENT}_LIBRARY)
        list(APPEND Tint_LIBRARIES ${TINT_${COMPONENT}_LIBRARY})
        add_library(Tint::${COMPONENT} UNKNOWN IMPORTED)
        set_target_properties(Tint::${COMPONENT} PROPERTIES
                IMPORTED_LOCATION "${TINT_${COMPONENT}_LIBRARY}"
        )
    else ()
        list(APPEND Tint_LIBRARIES_NOT_FOUND tint_${COMPONENT})
        if (Tint_FIND_REQUIRED_${COMPONENT})
            set(Tint_FOUND OFF)
        endif ()

    endif ()

    string(FIND ${COMPONENT} "reader" IS_READER)
    string(FIND ${COMPONENT} "writer" IS_WRITER)
    string(FIND ${COMPONENT} "wgsl" IS_WGSL)
    # Excluding WGSL libraries as they have their own definitions
    if (IS_WGSL LESS 0 AND (${IS_READER} GREATER -1 OR ${IS_WRITER} GREATER -1))
        # This two options should be mutually exclusive
        if (${IS_READER} GREATER -1)
            set(EXTRA_COMPONENTS ${TINT_READER_COMPONENTS})
        else()
            set(EXTRA_COMPONENTS ${TINT_WRITER_COMPONENTS})
        endif()
        foreach (LANG_COMPONENT ${EXTRA_COMPONENTS})
            set(TINT_LIB tint_${COMPONENT}_${LANG_COMPONENT})
            find_library(TINT_${TINT_LIB}_LIBRARY
                    NAMES ${TINT_LIB}
                    HINTS ${TINT_LIBRARY_DIR}
                    NO_DEFAULT_PATH
                    NO_CMAKE_FIND_ROOT_PATH
            )
            if (TINT_${TINT_LIB}_LIBRARY)
                list(APPEND Tint_LIBRARIES ${TINT_${TINT_LIB}_LIBRARY})
                add_library(Tint::lang_${COMPONENT}_${LANG_COMPONENT} UNKNOWN IMPORTED)
                set_target_properties(Tint::lang_${COMPONENT}_${LANG_COMPONENT} PROPERTIES
                        IMPORTED_LOCATION "${TINT_${TINT_LIB}_LIBRARY}"
                )
                target_link_libraries(Tint::${COMPONENT} INTERFACE Tint::lang_${COMPONENT}_${LANG_COMPONENT})
            else ()
                list(APPEND Tint_LIBRARIES_NOT_FOUND ${TINT_LIB})
                if (Tint_FIND_REQUIRED_${COMPONENT})
                    set(Tint_FOUND OFF)
                endif ()
            endif ()
            mark_as_advanced(TINT_${TINT_LIB}_LIBRARY)
        endforeach ()
    endif ()
    mark_as_advanced(TINT_${COMPONENT}_LIBRARY)
endforeach ()


set(ABSL_DEPENDENCIES
        absl_strings
)
foreach(ABSL_DEP ${ABSL_DEPENDENCIES})
    find_library(ABSL_${ABSL_DEP}_LIBRARY
            NAMES ${ABSL_DEP}
            PATHS ${TINT_LIBRARY_DIR}
    )
    if(ABSL_${ABSL_DEP}_LIBRARY)
        list(APPEND Tint_LIBRARIES ${ABSL_${ABSL_DEP}_LIBRARY})
    else()
        set(Tint_FOUND OFF)
        list(APPEND Tint_LIBRARIES_NOT_FOUND "${ABSL_DEP}")
    endif()
endforeach ()

if (NOT Tint_FIND_QUIETLY)
    if (Tint_LIBRARIES)
        message(STATUS "Found the following Tint libraries:")
        foreach (found ${Tint_LIBRARIES})
            message(STATUS " ${found}")
        endforeach ()
    endif ()
    if (Tint_LIBRARIES_NOT_FOUND)
        message(STATUS "Tint libraries not found")
        foreach (found ${Tint_LIBRARIES_NOT_FOUND})
            message(STATUS " ${found}")
        endforeach ()
    endif ()
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Tint
        FOUND_VAR Tint_FOUND
        REQUIRED_VARS
        Tint_SRC_INCLUDE_DIR
        Tint_API_INCLUDE_DIR
        Tint_LIBRARIES
)
