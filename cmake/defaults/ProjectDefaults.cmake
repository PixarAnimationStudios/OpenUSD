#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
if(APPLE)
    set(OSX_ARCHITECTURES "x86_64" CACHE STRING "Build architectures for OSX")
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_DYLIB_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib" CACHE STRING "install_name path for dylib.")
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)

    # Workaround for code signing issues that show up as warnings at the end
    # of the build like:
    # 
    #   install_name_tool: warning: changes being made to the file will invalidate the code signature in: ...
    #
    # On Apple silicon this issue would prevent binaries from being used since
    # the OS requires them to have a valid signature. This option is available
    # on macOS 11+, which corresponds to kernel version 20+.
    #
    # See https://gitlab.kitware.com/cmake/cmake/-/issues/21854
    if (CMAKE_HOST_SYSTEM_VERSION VERSION_GREATER_EQUAL 20)
        set(CMAKE_XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "-o linker-signed")
    endif()
endif()

# Allow local includes from source directory.
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Turn on folder usage
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

if (PXR_BUILD_TESTS)
    # Enable CTest
    enable_testing()
endif()
