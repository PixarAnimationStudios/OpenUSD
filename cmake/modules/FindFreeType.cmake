##################################
# Find Freetype components
##################################
if(NOT DEFINED FREETYPE_ROOT_DIR)
    set(FREETYPE_ROOT_DIR ${CMAKE_INSTALL_PREFIX})
endif()

FIND_PATH(FREETYPE_INCLUDE_DIR
    NAMES
        freetype/freetype.h
    PATHS
        "${FREETYPE_ROOT_DIR}"
        "$ENV{FREETYPE_ROOT_DIR}"
    NO_DEFAULT_PATH
    PATH_SUFFIXES
        include/freetype2
)

SET(FREETYPE_LIB_COMP freetype)
find_library(FREETYPE_LIB
    NAMES
        ${FREETYPE_LIB_COMP}
        ${FREETYPE_LIB_COMP}d
    PATHS
        "${FREETYPE_ROOT_DIR}"
        "$ENV{FREETYPE_ROOT_DIR}"
    NO_DEFAULT_PATH
    PATH_SUFFIXES
        lib
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(FreeType
    REQUIRED_VARS
        FREETYPE_INCLUDE_DIR
        FREETYPE_LIB
)