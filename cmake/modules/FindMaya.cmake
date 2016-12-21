# - Maya finder module
# This module searches for a valid Maya instalation.
# It searches for Maya's devkit, libraries, executables
# and related paths (scripts)
#
# Variables that will be defined:
# MAYA_FOUND          Defined if a Maya installation has been detected
# MAYA_EXECUTABLE     Path to Maya's executable
# MAYA_<lib>_FOUND    Defined if <lib> has been found
# MAYA_<lib>_LIBRARY  Path to <lib> library
# MAYA_INCLUDE_DIRS   Path to the devkit's include directories
# MAYA_API_VERSION    Maya version (6 digits)
#
# IMPORTANT: Currently, there's only support for OSX platform and Maya version 2017 because of ABI issues with libc++.

#=============================================================================
# Copyright 2011-2012 Francisco Requena <frarees@gmail.com>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

if(APPLE)
    find_path(MAYA_BASE_DIR
            ../../devkit/include/maya/MFn.h
        HINTS
            "${MAYA_LOCATION}"
            "$ENV{MAYA_LOCATION}"
            "/Applications/Autodesk/maya2017/Maya.app/Contents"
            "/Applications/Autodesk/maya2015/Maya.app/Contents"
            "/Applications/Autodesk/maya2014/Maya.app/Contents"
            "/Applications/Autodesk/maya2013.5/Maya.app/Contents"
            "/Applications/Autodesk/maya2013/Maya.app/Contents"
            "/Applications/Autodesk/maya2012.17/Maya.app/Contents"
            "/Applications/Autodesk/maya2012/Maya.app/Contents"
            "/Applications/Autodesk/maya2011/Maya.app/Contents"
            "/Applications/Autodesk/maya2010/Maya.app/Contents"
    )
    find_path(MAYA_LIBRARY_DIR libOpenMaya.dylib
        HINTS
            "${MAYA_LOCATION}"
            "$ENV{MAYA_LOCATION}"
            "${MAYA_BASE_DIR}"
        PATH_SUFFIXES
            Maya.app/contents/MacOS/
        DOC
            "Maya's libraries path"
    )
elseif(UNIX)
    find_path(MAYA_BASE_DIR
            include/maya/MFn.h
        HINTS
            "${MAYA_LOCATION}"
            "$ENV{MAYA_LOCATION}"
            "/usr/autodesk/maya2015-x64"
            "/usr/autodesk/maya2014-x64"
            "/usr/autodesk/maya2013-x64"
            "/usr/autodesk/maya2012.17-x64"
            "/usr/autodesk/maya2012-x64"
            "/usr/autodesk/maya2011-x64"
            "/usr/autodesk/maya2010-x64"
    )
    find_path(MAYA_LIBRARY_DIR
            libOpenMaya.so
        HINTS
            "${MAYA_LOCATION}"
            "$ENV{MAYA_LOCATION}"
            "${MAYA_BASE_DIR}"
        PATH_SUFFIXES
            lib/
        DOC
            "Maya's libraries path"
    )
elseif(WIN32)
    find_path(MAYA_BASE_DIR
            include/maya/MFn.h
        HINTS
            "${MAYA_LOCATION}"
            "$ENV{MAYA_LOCATION}"
            "C:/Program Files/Autodesk/Maya2016"
            "C:/Program Files/Autodesk/Maya2015.5-x64"
            "C:/Program Files/Autodesk/Maya2015.5"
            "C:/Program Files/Autodesk/Maya2014.5-x64"
            "C:/Program Files/Autodesk/Maya2014.5"
            "C:/Program Files/Autodesk/Maya2013.5-x64"
            "C:/Program Files/Autodesk/Maya2013.5"
            "C:/Program Files (x86)/Autodesk/Maya2013.5"
            "C:/Autodesk/maya-2013.5x64"
            "C:/Program Files/Autodesk/Maya2013-x64"
            "C:/Program Files/Autodesk/Maya2013"
            "C:/Program Files (x86)/Autodesk/Maya2013"
            "C:/Autodesk/maya-2013x64"
            "C:/Program Files/Autodesk/Maya2012-x64"
            "C:/Program Files/Autodesk/Maya2012"
            "C:/Program Files (x86)/Autodesk/Maya2012"
            "C:/Autodesk/maya-2012x64"
            "C:/Program Files/Autodesk/Maya2011-x64"
            "C:/Program Files/Autodesk/Maya2011"
            "C:/Program Files (x86)/Autodesk/Maya2011"
            "C:/Autodesk/maya-2011x64"
            "C:/Program Files/Autodesk/Maya2010-x64"
            "C:/Program Files/Autodesk/Maya2010"
            "C:/Program Files (x86)/Autodesk/Maya2010"
            "C:/Autodesk/maya-2010x64"
    )
    find_path(MAYA_LIBRARY_DIR
            OpenMaya.lib
        HINTS
            "${MAYA_LOCATION}"
            "$ENV{MAYA_LOCATION}"
            "${MAYA_BASE_DIR}"
        PATH_SUFFIXES
            lib/
        DOC
            "Maya's libraries path"
    )
endif()

find_path(MAYA_INCLUDE_DIR
        maya/MFn.h
    HINTS
        "${MAYA_LOCATION}"
        "$ENV{MAYA_LOCATION}"
        "${MAYA_BASE_DIR}"
    PATH_SUFFIXES
        ../../devkit/include/
        include/
    DOC
        "Maya's devkit headers path"
)

find_path(MAYA_LIBRARY_DIR
        OpenMaya
    HINTS
        "${MAYA_LOCATION}"
        "$ENV{MAYA_LOCATION}"
        "${MAYA_BASE_DIR}"
    PATH_SUFFIXES
        ../../devkit/include/
        include/
    DOC
        "Maya's devkit headers path"
)

list(APPEND MAYA_INCLUDE_DIRS ${MAYA_INCLUDE_DIR})

find_path(MAYA_DEVKIT_INC_DIR
       GL/glext.h
    HINTS
        "${MAYA_LOCATION}"
        "$ENV{MAYA_LOCATION}"
        "${MAYA_BASE_DIR}"
    PATH_SUFFIXES
        ../../devkit/plug-ins/
    DOC
        "Maya's devkit headers path"
)

list(APPEND MAYA_INCLUDE_DIRS ${MAYA_DEVKIT_INC_DIR})

foreach(MAYA_LIB
    OpenMaya
    OpenMayaAnim
    OpenMayaFX
    OpenMayaRender
    OpenMayaUI
    Image
    Foundation
    IMFbase
    tbb
    cg
    cgGL)

    find_library(MAYA_${MAYA_LIB}_LIBRARY
            ${MAYA_LIB}
        HINTS
            "${MAYA_LOCATION}"
            "$ENV{MAYA_LOCATION}"
            "${MAYA_BASE_DIR}"
        PATH_SUFFIXES
            MacOS/
            lib/
        DOC
            "Maya's ${MAYA_LIB} library path"
        # NO_CMAKE_SYSTEM_PATH needed to avoid conflicts between
        # Maya's Foundation library and OSX's framework.
        NO_CMAKE_SYSTEM_PATH
    )


    if (MAYA_${MAYA_LIB}_LIBRARY)
        list(APPEND MAYA_LIBRARIES ${MAYA_${MAYA_LIB}_LIBRARY})
    endif()
endforeach(MAYA_LIB)

find_program(MAYA_EXECUTABLE
        maya
    HINTS
        "${MAYA_LOCATION}"
        "$ENV{MAYA_LOCATION}"
        "${MAYA_BASE_DIR}"
    PATH_SUFFIXES
        MacOS/
        bin/
    DOC
        "Maya's executable path"
)

if(MAYA_INCLUDE_DIRS AND EXISTS "${MAYA_INCLUDE_DIR}/maya/MTypes.h")

    # Tease the MAYA_API_VERSION numbers from the lib headers
    file(STRINGS ${MAYA_INCLUDE_DIR}/maya/MTypes.h TMP REGEX "#define MAYA_API_VERSION.*$")
    string(REGEX MATCHALL "[0-9]+" MAYA_API_VERSION ${TMP})
endif()

# handle the QUIETLY and REQUIRED arguments and set MAYA_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)

# Allow Maya plug-in to compiled without having Maya installed
if (${PXR_MAYA_API_ONLY})
    find_package_handle_standard_args(Maya
        REQUIRED_VARS
            MAYA_INCLUDE_DIRS
            MAYA_LIBRARIES
        VERSION_VAR
            MAYA_API_VERSION
    )
else()
    find_package_handle_standard_args(Maya
        REQUIRED_VARS
            MAYA_EXECUTABLE
            MAYA_INCLUDE_DIRS
            MAYA_LIBRARIES
        VERSION_VAR
            MAYA_API_VERSION
    )
endif()
