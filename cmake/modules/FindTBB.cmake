# The MIT License (MIT)
#
# Copyright (c) 2015 Justus Calvin
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

#
# FindTBB
# -------
#
# Find TBB include directories and libraries.
#
# Usage:
#
#  find_package(TBB [major[.minor]] [EXACT]
#               [QUIET] [REQUIRED]
#               [[COMPONENTS] [components...]]
#               [OPTIONAL_COMPONENTS components...]) 
#
# where the allowed components are tbbmalloc and tbb_preview. Users may modify 
# the behavior of this module with the following variables:
#
# * TBB_ROOT_DIR          - The base directory the of TBB installation.
# * TBB_INCLUDE_DIR       - The directory that contains the TBB headers files.
# * TBB_LIBRARY           - The directory that contains the TBB library files.
# * TBB_<library>_LIBRARY - The path of the TBB the corresponding TBB library. 
#                           These libraries, if specified, override the 
#                           corresponding library search results, where <library>
#                           may be tbb, tbb_debug, tbbmalloc, tbbmalloc_debug,
#                           tbb_preview, or tbb_preview_debug.
# * TBB_USE_DEBUG_BUILD   - The debug version of tbb libraries, if present, will
#                           be used instead of the release version.
#
# Users may modify the behavior of this module with the following environment
# variables:
#
# * TBB_INSTALL_DIR 
# * TBBROOT
# * LIBRARY_PATH
#
# This module will set the following variables:
#
# * TBB_FOUND             - Set to false, or undefined, if we haven’t found, or
#                           don’t want to use TBB.
# * TBB_<component>_FOUND - If False, optional <component> part of TBB sytem is
#                           not available.
# * TBB_VERSION           - The full version string
# * TBB_VERSION_MAJOR     - The major version
# * TBB_VERSION_MINOR     - The minor version
# * TBB_INTERFACE_VERSION - The interface version number defined in 
#                           tbb/tbb_stddef.h.
# * TBB_<library>_LIBRARY_RELEASE - The path of the TBB release version of 
#                           <library>, where <library> may be tbb, tbb_debug,
#                           tbbmalloc, tbbmalloc_debug, tbb_preview, or 
#                           tbb_preview_debug.
# * TBB_<library>_LIBRARY_DEGUG - The path of the TBB release version of 
#                           <library>, where <library> may be tbb, tbb_debug,
#                           tbbmalloc, tbbmalloc_debug, tbb_preview, or 
#                           tbb_preview_debug.
#
# The following varibles should be used to build and link with TBB:
#
# * TBB_INCLUDE_DIRS - The include directory for TBB.
# * TBB_LIBRARIES    - The libraries to link against to use TBB.
# * TBB_DEFINITIONS  - Definitions to use when compiling code that uses TBB.

include(FindPackageHandleStandardArgs)

if(NOT TBB_FOUND)

  ##################################
  # Check the build type
  ##################################
  
  if(NOT DEFINED TBB_USE_DEBUG_BUILD)
    if(CMAKE_BUILD_TYPE MATCHES "Debug|DEBUG|debug|RelWithDebInfo|RELWITHDEBINFO|relwithdebinfo")
      set(TBB_USE_DEBUG_BUILD TRUE)
    else()
      set(TBB_USE_DEBUG_BUILD FALSE)
    endif()
  endif()
  
  ##################################
  # Set the TBB search directories
  ##################################
  
  # Define search paths based on user input and environment variables
  set(TBB_SEARCH_DIR ${TBB_ROOT_DIR} $ENV{TBB_INSTALL_DIR} $ENV{TBBROOT})
  
  # Define the search directories based on the current platform
  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(TBB_DEFAULT_SEARCH_DIR "C:/Program Files/Intel/TBB"
                               "C:/Program Files (x86)/Intel/TBB")
    # TODO: Set the proper suffix paths based on compiler introspection.
  
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # OS X
    set(TBB_DEFAULT_SEARCH_DIR "/opt/intel/tbb")
    
    # TODO: Check to see which C++ library is being used by the compiler.
    if(NOT ${CMAKE_SYSTEM_VERSION} VERSION_LESS 13.0)
      # The default C++ library on OS X 10.9 and later is libc++
      set(TBB_LIB_PATH_SUFFIX "lib/libc++")
    else()
      set(TBB_LIB_PATH_SUFFIX "lib")
    endif()
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Linux
    set(TBB_DEFAULT_SEARCH_DIR "/opt/intel/tbb")
    
    # TODO: Check compiler version to see the suffix should be <arch>/gcc4.1 or
    #       <arch>/gcc4.1. For now, assume that the compiler is more recent than
    #       gcc 4.4.x or later.
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
      set(TBB_LIB_PATH_SUFFIX "lib/intel64/gcc4.4")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^i.86$")
      set(TBB_LIB_PATH_SUFFIX "lib/ia32/gcc4.4")
    endif()
  endif()
  
  ##################################
  # Find the TBB include dir
  ##################################
  
  find_path(TBB_INCLUDE_DIRS tbb/tbb.h
      HINTS ${TBB_INCLUDE_DIR} ${TBB_SEARCH_DIR}
      PATHS ${TBB_DEFAULT_SEARCH_DIR}
      PATH_SUFFIXES include)
  
  ##################################
  # Find TBB components
  ##################################

  # Find each component
  foreach(_comp tbb_preview tbbmalloc tbb)
    # Search for the libraries
    find_library(TBB_${_comp}_LIBRARY_RELEASE ${_comp}
        HINTS ${TBB_LIBRARY} ${TBB_SEARCH_DIR}
        PATHS ${TBB_DEFAULT_SEARCH_DIR}
        PATH_SUFFIXES "${TBB_LIB_PATH_SUFFIX}")

    find_library(TBB_${_comp}_LIBRARY_DEBUG ${_comp}_debug
        HINTS ${TBB_LIBRARY} ${TBB_SEARCH_DIR}
        PATHS ${TBB_DEFAULT_SEARCH_DIR} ENV LIBRARY_PATH
        PATH_SUFFIXES "${TBB_LIB_PATH_SUFFIX}")

    
    # Set the library to be used for the component
    if(NOT TBB_${_comp}_LIBRARY)
      if(TBB_USE_DEBUG_BUILD AND TBB_${_comp}_LIBRARY_DEBUG)
        set(TBB_${_comp}_LIBRARY "${TBB_${_comp}_LIBRARY_DEBUG}")
      elseif(TBB_${_comp}_LIBRARY_RELEASE)
        set(TBB_${_comp}_LIBRARY "${TBB_${_comp}_LIBRARY_RELEASE}")
      elseif(TBB_${_comp}_LIBRARY_DEBUG)
        set(TBB_${_comp}_LIBRARY "${TBB_${_comp}_LIBRARY_DEBUG}")
      endif()
    endif()
    
    # Set the TBB library list and component found variables
    if(TBB_${_comp}_LIBRARY)
      list(APPEND TBB_LIBRARIES "${TBB_${_comp}_LIBRARY}")
      set(TBB_${_comp}_FOUND TRUE)
    else()
      set(TBB_${_comp}_FOUND FALSE)
    endif()
    
    mark_as_advanced(TBB_${_comp}_LIBRARY_RELEASE)
    mark_as_advanced(TBB_${_comp}_LIBRARY_DEBUG)
    mark_as_advanced(TBB_${_comp}_LIBRARY)
    
  endforeach()
  
  ##################################
  # Set compile flags
  ##################################
  
  if(TBB_tbb_LIBRARY MATCHES "debug")
    set(TBB_DEFINITIONS "-DTBB_USE_DEBUG=1")
  endif()
  
  ##################################
  # Set version strings
  ##################################
  
  if(TBB_INCLUDE_DIRS)
    file(READ "${TBB_INCLUDE_DIRS}/tbb/tbb_stddef.h" _tbb_version_file)
    string(REGEX REPLACE ".*#define TBB_VERSION_MAJOR ([0-9]+).*" "\\1"
            TBB_VERSION_MAJOR "${_tbb_version_file}")
    string(REGEX REPLACE ".*#define TBB_VERSION_MINOR ([0-9]+).*" "\\1"
            TBB_VERSION_MINOR "${_tbb_version_file}")
    string(REGEX REPLACE ".*#define TBB_INTERFACE_VERSION ([0-9]+).*" "\\1"
            TBB_INTERFACE_VERSION "${_tbb_version_file}")
    set(TBB_VERSION "${TBB_VERSION_MAJOR}.${TBB_VERSION_MINOR}")
  endif()
  
  find_package_handle_standard_args(TBB 
      REQUIRED_VARS TBB_INCLUDE_DIRS TBB_LIBRARIES
      HANDLE_COMPONENTS
      VERSION_VAR TBB_VERSION)
  
  mark_as_advanced(TBB_INCLUDE_DIRS TBB_LIBRARIES)

endif()
