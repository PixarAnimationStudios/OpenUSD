#
# Copyright 2023 Pixar
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

# The following code is a modified version of ios-cmake which only includes
# Items needed for building in USD. https://github.com/leetal/ios-cmake
# Following is the copyright notice for the project which follows BSD-3
# https://github.com/leetal/ios-cmake/blob/master/LICENSE.md
# Copyright (c) 2011-2014, Andrew Fischer andrew@ltengsoft.com
# Copyright (c) 2017, Alexander Widerberg widerbergaren@gmail.com

# List of supported platform values
list(APPEND _supported_platforms
        "OS" "OS64" "OS64COMBINED")

# Cache what generator is used
set(USED_CMAKE_GENERATOR "${CMAKE_GENERATOR}")

# Check if using a CMake version capable of building combined FAT builds (simulator and target slices combined in one static lib)
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.14")
  set(MODERN_CMAKE YES)
endif()

# Get the Xcode version being used.
# Problem: CMake runs toolchain files multiple times, but can't read cache variables on some runs.
# Workaround: On the first run (in which cache variables are always accessible), set an intermediary environment variable.
#
# NOTE: This pattern is used in many places in this toolchain to speed up checks of all sorts
if(DEFINED XCODE_VERSION_INT)
  # Environment variables are always preserved.
  set(ENV{_XCODE_VERSION_INT} "${XCODE_VERSION_INT}")
elseif(DEFINED ENV{_XCODE_VERSION_INT})
  set(XCODE_VERSION_INT "$ENV{_XCODE_VERSION_INT}")
elseif(NOT DEFINED XCODE_VERSION_INT)
  find_program(XCODEBUILD_EXECUTABLE xcodebuild)
  if(NOT XCODEBUILD_EXECUTABLE)
    message(FATAL_ERROR "xcodebuild not found. Please install either the standalone commandline tools or Xcode.")
  endif()
  execute_process(COMMAND ${XCODEBUILD_EXECUTABLE} -version
          OUTPUT_VARIABLE XCODE_VERSION_INT
          ERROR_QUIET
          OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REGEX MATCH "Xcode [0-9\\.]+" XCODE_VERSION_INT "${XCODE_VERSION_INT}")
  string(REGEX REPLACE "Xcode ([0-9\\.]+)" "\\1" XCODE_VERSION_INT "${XCODE_VERSION_INT}")
  set(XCODE_VERSION_INT "${XCODE_VERSION_INT}" CACHE INTERNAL "")
endif()

# Check if the platform variable is set
if(DEFINED PLATFORM)
  # Environment variables are always preserved.
  set(ENV{_PLATFORM} "${PLATFORM}")
elseif(DEFINED ENV{_PLATFORM})
  set(PLATFORM "$ENV{_PLATFORM}")
elseif(NOT DEFINED PLATFORM)
  message(FATAL_ERROR "PLATFORM argument not set. Bailing configure since I don't know what target you want to build for!")
endif ()

if(PLATFORM MATCHES ".*COMBINED" AND NOT CMAKE_GENERATOR MATCHES "Xcode")
  message(FATAL_ERROR "The combined builds support requires Xcode to be used as a generator via '-G Xcode' command-line argument in CMake")
endif()

# Safeguard that the platform value is set and is one of the supported values
list(FIND _supported_platforms ${PLATFORM} contains_PLATFORM)
if("${contains_PLATFORM}" EQUAL "-1")
  string(REPLACE ";"  "\n * " _supported_platforms_formatted "${_supported_platforms}")
  message(FATAL_ERROR " Invalid PLATFORM specified! Current value: ${PLATFORM}.\n"
          " Supported PLATFORM values: \n * ${_supported_platforms_formatted}")
endif()

# Fix for PThread library not in path
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)

# Specify named language support defaults.
if(NOT DEFINED NAMED_LANGUAGE_SUPPORT AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16")
  set(NAMED_LANGUAGE_SUPPORT ON)
  message(STATUS "[DEFAULTS] Using explicit named language support! E.g., enable_language(CXX) is needed in the project files.")
elseif(NOT DEFINED NAMED_LANGUAGE_SUPPORT AND ${CMAKE_VERSION} VERSION_LESS "3.16")
  set(NAMED_LANGUAGE_SUPPORT OFF)
  message(STATUS "[DEFAULTS] Disabling explicit named language support. Falling back to legacy behavior.")
elseif(DEFINED NAMED_LANGUAGE_SUPPORT AND ${CMAKE_VERSION} VERSION_LESS "3.16")
  message(FATAL_ERROR "CMake named language support for OBJC and OBJCXX was added in CMake 3.16.")
endif()
set(NAMED_LANGUAGE_SUPPORT_INT ${NAMED_LANGUAGE_SUPPORT} CACHE BOOL
        "Whether or not to enable explicit named language support" FORCE)

# Specify the minimum version of the deployment target.
if(NOT DEFINED DEPLOYMENT_TARGET)
  set(DEPLOYMENT_TARGET "11.0")
  message(STATUS "[DEFAULTS] Using the default min-version since DEPLOYMENT_TARGET not provided!")
endif()

# Store the DEPLOYMENT_TARGET in the cache
set(DEPLOYMENT_TARGET "${DEPLOYMENT_TARGET}" CACHE INTERNAL "")

# Handle the case where we are targeting iOS and a version above 10.3.4 (32-bit support dropped officially)
if(PLATFORM STREQUAL "OS" AND DEPLOYMENT_TARGET VERSION_GREATER_EQUAL 10.3.4)
  set(PLATFORM "OS64")
  message(STATUS "Targeting minimum SDK version ${DEPLOYMENT_TARGET}. Dropping 32-bit support.")
endif()

set(PLATFORM_INT "${PLATFORM}")

if(DEFINED ARCHS)
  string(REPLACE ";" "-" ARCHS_SPLIT "${ARCHS}")
endif()

# Determine the platform name and architectures for use in xcodebuild commands
# from the specified PLATFORM_INT name.
if(PLATFORM_INT STREQUAL "OS")
  set(SDK_NAME iphoneos)
  if(NOT ARCHS)
    set(ARCHS armv7 armv7s arm64)
    set(APPLE_TARGET_TRIPLE_INT arm-apple-ios${DEPLOYMENT_TARGET})
  else()
    set(APPLE_TARGET_TRIPLE_INT ${ARCHS_SPLIT}-apple-ios${DEPLOYMENT_TARGET})
  endif()
elseif(PLATFORM_INT STREQUAL "OS64")
  set(SDK_NAME iphoneos)
  if(NOT ARCHS)
    if (XCODE_VERSION_INT VERSION_GREATER 10.0)
      set(ARCHS arm64) # FIXME: Add arm64e when Apple has fixed the integration issues with it, libarclite_iphoneos.a is currently missing bitcode markers for example
    else()
      set(ARCHS arm64)
    endif()
    set(APPLE_TARGET_TRIPLE_INT aarch64-apple-ios${DEPLOYMENT_TARGET})
  else()
    set(APPLE_TARGET_TRIPLE_INT ${ARCHS_SPLIT}-apple-ios${DEPLOYMENT_TARGET})
  endif()
elseif(PLATFORM_INT STREQUAL "OS64COMBINED")
  set(SDK_NAME iphoneos)
  if(MODERN_CMAKE)
    if(NOT ARCHS)
      if (XCODE_VERSION_INT VERSION_GREATER 10.0)
        set(ARCHS arm64 x86_64) # FIXME: Add arm64e when Apple has fixed the integration issues with it, libarclite_iphoneos.a is currently missing bitcode markers for example
        set(CMAKE_XCODE_ATTRIBUTE_ARCHS[sdk=iphoneos*] "arm64")
        set(CMAKE_XCODE_ATTRIBUTE_VALID_ARCHS[sdk=iphoneos*] "arm64")
      else()
        set(ARCHS arm64 x86_64)
        set(CMAKE_XCODE_ATTRIBUTE_ARCHS[sdk=iphoneos*] "arm64")
        set(CMAKE_XCODE_ATTRIBUTE_VALID_ARCHS[sdk=iphoneos*] "arm64")
      endif()
      set(APPLE_TARGET_TRIPLE_INT aarch64-x86_64-apple-ios${DEPLOYMENT_TARGET})
    else()
      set(APPLE_TARGET_TRIPLE_INT ${ARCHS_SPLIT}-apple-ios${DEPLOYMENT_TARGET})
    endif()
  else()
    message(FATAL_ERROR "Please make sure that you are running CMake 3.14+ to make the OS64COMBINED setting work")
  endif()
else()
  message(FATAL_ERROR "Invalid PLATFORM: ${PLATFORM_INT}")
endif()

string(REPLACE ";" " " ARCHS_SPACED "${ARCHS}")

if(MODERN_CMAKE AND PLATFORM_INT MATCHES ".*COMBINED" AND NOT CMAKE_GENERATOR MATCHES "Xcode")
  message(FATAL_ERROR "The COMBINED options only work with Xcode generator, -G Xcode")
endif()

set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "${DEPLOYMENT_TARGET}")
if(NOT PLATFORM_INT MATCHES ".*COMBINED")
  set(CMAKE_XCODE_ATTRIBUTE_ARCHS[sdk=${SDK_NAME}*] "${ARCHS_SPACED}")
  set(CMAKE_XCODE_ATTRIBUTE_VALID_ARCHS[sdk=${SDK_NAME}*] "${ARCHS_SPACED}")
endif()

# If the user did not specify the SDK root to use, then query xcodebuild for it.
if(DEFINED CMAKE_OSX_SYSROOT_INT)
  # Environment variables are always preserved.
  set(ENV{_CMAKE_OSX_SYSROOT_INT} "${CMAKE_OSX_SYSROOT_INT}")
elseif(DEFINED ENV{_CMAKE_OSX_SYSROOT_INT})
  set(CMAKE_OSX_SYSROOT_INT "$ENV{_CMAKE_OSX_SYSROOT_INT}")
elseif(NOT DEFINED CMAKE_OSX_SYSROOT_INT)
  execute_process(COMMAND ${XCODEBUILD_EXECUTABLE} -version -sdk ${SDK_NAME} Path
          OUTPUT_VARIABLE CMAKE_OSX_SYSROOT_INT
          ERROR_QUIET
          OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

if (NOT DEFINED CMAKE_OSX_SYSROOT_INT AND NOT DEFINED CMAKE_OSX_SYSROOT)
  message(SEND_ERROR "Please make sure that Xcode is installed and that the toolchain"
          "is pointing to the correct path. Please run:"
          "sudo xcode-select -s /Applications/Xcode.app/Contents/Developer"
          "and see if that fixes the problem for you.")
  message(FATAL_ERROR "Invalid CMAKE_OSX_SYSROOT: ${CMAKE_OSX_SYSROOT} "
          "does not exist.")
elseif(DEFINED CMAKE_OSX_SYSROOT_INT)
  set(CMAKE_OSX_SYSROOT_INT "${CMAKE_OSX_SYSROOT_INT}" CACHE INTERNAL "")
  # Specify the location or name of the platform SDK to be used in CMAKE_OSX_SYSROOT.
  set(CMAKE_OSX_SYSROOT "${CMAKE_OSX_SYSROOT_INT}" CACHE INTERNAL "")
endif()

# Use bitcode or not
if(NOT DEFINED ENABLE_BITCODE AND NOT ARCHS MATCHES "((^|;|, )(i386|x86_64))+")
  # Unless specified, enable bitcode support by default
  message(STATUS "[DEFAULTS] Enabling bitcode support by default. ENABLE_BITCODE not provided!")
  set(ENABLE_BITCODE ON)
elseif(NOT DEFINED ENABLE_BITCODE)
  message(STATUS "[DEFAULTS] Disabling bitcode support by default on simulators. ENABLE_BITCODE not provided for override!")
  set(ENABLE_BITCODE OFF)
endif()
set(ENABLE_BITCODE_INT ${ENABLE_BITCODE} CACHE BOOL
        "Whether or not to enable bitcode" FORCE)
# Use ARC or not
if(NOT DEFINED ENABLE_ARC)
  # Unless specified, enable ARC support by default
  set(ENABLE_ARC ON)
  message(STATUS "[DEFAULTS] Enabling ARC support by default. ENABLE_ARC not provided!")
endif()
set(ENABLE_ARC_INT ${ENABLE_ARC} CACHE BOOL "Whether or not to enable ARC" FORCE)
# Use hidden visibility or not
if(NOT DEFINED ENABLE_VISIBILITY)
  # Unless specified, disable symbols visibility by default
  set(ENABLE_VISIBILITY OFF)
  message(STATUS "[DEFAULTS] Hiding symbols visibility by default. ENABLE_VISIBILITY not provided!")
endif()
set(ENABLE_VISIBILITY_INT ${ENABLE_VISIBILITY} CACHE BOOL "Whether or not to hide symbols from the dynamic linker (-fvisibility=hidden)" FORCE)
# Set strict compiler checks or not
if(NOT DEFINED ENABLE_STRICT_TRY_COMPILE)
  # Unless specified, disable strict try_compile()
  set(ENABLE_STRICT_TRY_COMPILE OFF)
  message(STATUS "[DEFAULTS] Using NON-strict compiler checks by default. ENABLE_STRICT_TRY_COMPILE not provided!")
endif()
set(ENABLE_STRICT_TRY_COMPILE_INT ${ENABLE_STRICT_TRY_COMPILE} CACHE BOOL
        "Whether or not to use strict compiler checks" FORCE)

# Get the SDK version information.
if(DEFINED SDK_VERSION)
  # Environment variables are always preserved.
  set(ENV{_SDK_VERSION} "${SDK_VERSION}")
elseif(DEFINED ENV{_SDK_VERSION})
  set(SDK_VERSION "$ENV{_SDK_VERSION}")
elseif(NOT DEFINED SDK_VERSION)
  execute_process(COMMAND ${XCODEBUILD_EXECUTABLE} -sdk ${CMAKE_OSX_SYSROOT_INT} -version SDKVersion
          OUTPUT_VARIABLE SDK_VERSION
          ERROR_QUIET
          OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

# Find the Developer root for the specific iOS platform being compiled for
# from CMAKE_OSX_SYSROOT.  Should be ../../ from SDK specified in
# CMAKE_OSX_SYSROOT. There does not appear to be a direct way to obtain
# this information from xcrun or xcodebuild.
if (NOT DEFINED CMAKE_DEVELOPER_ROOT AND NOT CMAKE_GENERATOR MATCHES "Xcode")
  get_filename_component(PLATFORM_SDK_DIR ${CMAKE_OSX_SYSROOT_INT} PATH)
  get_filename_component(CMAKE_DEVELOPER_ROOT ${PLATFORM_SDK_DIR} PATH)
  if (NOT EXISTS "${CMAKE_DEVELOPER_ROOT}")
    message(FATAL_ERROR "Invalid CMAKE_DEVELOPER_ROOT: ${CMAKE_DEVELOPER_ROOT} does not exist.")
  endif()
endif()

# Find the C & C++ compilers for the specified SDK.
if(DEFINED CMAKE_C_COMPILER)
  # Environment variables are always preserved.
  set(ENV{_CMAKE_C_COMPILER} "${CMAKE_C_COMPILER}")
elseif(DEFINED ENV{_CMAKE_C_COMPILER})
  set(CMAKE_C_COMPILER "$ENV{_CMAKE_C_COMPILER}")
  set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
elseif(NOT DEFINED CMAKE_C_COMPILER)
  execute_process(COMMAND xcrun -sdk ${CMAKE_OSX_SYSROOT_INT} -find clang
          OUTPUT_VARIABLE CMAKE_C_COMPILER
          ERROR_QUIET
          OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
endif()
if(DEFINED CMAKE_CXX_COMPILER)
  # Environment variables are always preserved.
  set(ENV{_CMAKE_CXX_COMPILER} "${CMAKE_CXX_COMPILER}")
elseif(DEFINED ENV{_CMAKE_CXX_COMPILER})
  set(CMAKE_CXX_COMPILER "$ENV{_CMAKE_CXX_COMPILER}")
elseif(NOT DEFINED CMAKE_CXX_COMPILER)
  execute_process(COMMAND xcrun -sdk ${CMAKE_OSX_SYSROOT_INT} -find clang++
          OUTPUT_VARIABLE CMAKE_CXX_COMPILER
          ERROR_QUIET
          OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
# Find (Apple's) libtool.
if(DEFINED BUILD_LIBTOOL)
  # Environment variables are always preserved.
  set(ENV{_BUILD_LIBTOOL} "${BUILD_LIBTOOL}")
elseif(DEFINED ENV{_BUILD_LIBTOOL})
  set(BUILD_LIBTOOL "$ENV{_BUILD_LIBTOOL}")
elseif(NOT DEFINED BUILD_LIBTOOL)
  execute_process(COMMAND xcrun -sdk ${CMAKE_OSX_SYSROOT_INT} -find libtool
          OUTPUT_VARIABLE BUILD_LIBTOOL
          ERROR_QUIET
          OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
# Find the toolchain's provided install_name_tool if none is found on the host
if(DEFINED CMAKE_INSTALL_NAME_TOOL)
  # Environment variables are always preserved.
  set(ENV{_CMAKE_INSTALL_NAME_TOOL} "${CMAKE_INSTALL_NAME_TOOL}")
elseif(DEFINED ENV{_CMAKE_INSTALL_NAME_TOOL})
  set(CMAKE_INSTALL_NAME_TOOL "$ENV{_CMAKE_INSTALL_NAME_TOOL}")
elseif(NOT DEFINED CMAKE_INSTALL_NAME_TOOL)
  execute_process(COMMAND xcrun -sdk ${CMAKE_OSX_SYSROOT_INT} -find install_name_tool
          OUTPUT_VARIABLE CMAKE_INSTALL_NAME_TOOL_INT
          ERROR_QUIET
          OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(CMAKE_INSTALL_NAME_TOOL ${CMAKE_INSTALL_NAME_TOOL_INT} CACHE INTERNAL "")
endif()

# Configure libtool to be used instead of ar + ranlib to build static libraries.
# This is required on Xcode 7+, but should also work on previous versions of
# Xcode.
get_property(languages GLOBAL PROPERTY ENABLED_LANGUAGES)
foreach(lang ${languages})
  set(CMAKE_${lang}_CREATE_STATIC_LIBRARY "${BUILD_LIBTOOL} -static -o <TARGET> <LINK_FLAGS> <OBJECTS> " CACHE INTERNAL "")
endforeach()

# CMake 3.14+ support building for iOS
if(MODERN_CMAKE)
  if(SDK_NAME MATCHES "iphone")
    set(CMAKE_SYSTEM_NAME iOS)
  endif()
  # Provide flags for a combined FAT library build on newer CMake versions
  if(PLATFORM_INT MATCHES ".*COMBINED")
    set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "NO")
    set(CMAKE_IOS_INSTALL_COMBINED YES)
  endif()
elseif(NOT DEFINED CMAKE_SYSTEM_NAME AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.10")
  # Legacy code path prior to CMake 3.14 or fallback if no CMAKE_SYSTEM_NAME specified
  set(CMAKE_SYSTEM_NAME iOS)
elseif(NOT DEFINED CMAKE_SYSTEM_NAME)
  # Legacy code path before CMake 3.14 or fallback if no CMAKE_SYSTEM_NAME specified
  set(CMAKE_SYSTEM_NAME Darwin)
endif()
# Standard settings.
set(CMAKE_SYSTEM_VERSION ${SDK_VERSION} CACHE INTERNAL "")
set(UNIX ON CACHE BOOL "")
set(APPLE ON CACHE BOOL "")
set(IOS ON CACHE BOOL "")
set(CMAKE_AR ar CACHE FILEPATH "" FORCE)
set(CMAKE_RANLIB ranlib CACHE FILEPATH "" FORCE)
set(CMAKE_STRIP strip CACHE FILEPATH "" FORCE)
# Set the architectures for which to build.
set(CMAKE_OSX_ARCHITECTURES ${ARCHS} CACHE INTERNAL "")
# Change the type of target generated for try_compile() so it'll work when cross-compiling, weak compiler checks
if(NOT ENABLE_STRICT_TRY_COMPILE_INT)
  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
endif()
# All iOS/Darwin specific settings - some may be redundant.
if (NOT DEFINED CMAKE_MACOSX_BUNDLE)
  set(CMAKE_MACOSX_BUNDLE YES)
endif()
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
set(CMAKE_SHARED_LIBRARY_PREFIX "lib")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".dylib")
set(CMAKE_SHARED_MODULE_PREFIX "lib")
set(CMAKE_SHARED_MODULE_SUFFIX ".so")
set(CMAKE_C_COMPILER_ABI ELF)
set(CMAKE_CXX_COMPILER_ABI ELF)
set(CMAKE_C_HAS_ISYSROOT 1)
set(CMAKE_CXX_HAS_ISYSROOT 1)
set(CMAKE_MODULE_EXISTS 1)
set(CMAKE_DL_LIBS "")
set(CMAKE_C_OSX_COMPATIBILITY_VERSION_FLAG "-compatibility_version ")
set(CMAKE_C_OSX_CURRENT_VERSION_FLAG "-current_version ")
set(CMAKE_CXX_OSX_COMPATIBILITY_VERSION_FLAG "${CMAKE_C_OSX_COMPATIBILITY_VERSION_FLAG}")
set(CMAKE_CXX_OSX_CURRENT_VERSION_FLAG "${CMAKE_C_OSX_CURRENT_VERSION_FLAG}")

if(ARCHS MATCHES "((^|;|, )(arm64|arm64e|x86_64))+")
  set(CMAKE_C_SIZEOF_DATA_PTR 8)
  set(CMAKE_CXX_SIZEOF_DATA_PTR 8)
  if(ARCHS MATCHES "((^|;|, )(arm64|arm64e))+")
    set(CMAKE_SYSTEM_PROCESSOR "aarch64")
  else()
    set(CMAKE_SYSTEM_PROCESSOR "x86_64")
  endif()
else()
  set(CMAKE_C_SIZEOF_DATA_PTR 4)
  set(CMAKE_CXX_SIZEOF_DATA_PTR 4)
  set(CMAKE_SYSTEM_PROCESSOR "arm")
endif()

# Note that only Xcode 7+ supports the newer more specific:
# -m${SDK_NAME}-version-min flags, older versions of Xcode use:
# -m(ios/ios-simulator)-version-min instead.
if(${CMAKE_VERSION} VERSION_LESS "3.11")
  if(PLATFORM_INT STREQUAL "OS" OR PLATFORM_INT STREQUAL "OS64")
    if(XCODE_VERSION_INT VERSION_LESS 7.0)
      set(SDK_NAME_VERSION_FLAGS
              "-mios-version-min=${DEPLOYMENT_TARGET}")
    else()
      # Xcode 7.0+ uses flags we can build directly from SDK_NAME.
      set(SDK_NAME_VERSION_FLAGS
              "-m${SDK_NAME}-version-min=${DEPLOYMENT_TARGET}")
    endif()
  else()
    # SIMULATOR or SIMULATOR64 both use -mios-simulator-version-min.
    set(SDK_NAME_VERSION_FLAGS
            "-mios-simulator-version-min=${DEPLOYMENT_TARGET}")
  endif()
else()
  set(CMAKE_OSX_DEPLOYMENT_TARGET ${DEPLOYMENT_TARGET})
endif()

if(DEFINED APPLE_TARGET_TRIPLE_INT)
  set(APPLE_TARGET_TRIPLE ${APPLE_TARGET_TRIPLE_INT} CACHE INTERNAL "")
  set(CMAKE_C_COMPILER_TARGET ${APPLE_TARGET_TRIPLE})
  set(CMAKE_CXX_COMPILER_TARGET ${APPLE_TARGET_TRIPLE})
  set(CMAKE_ASM_COMPILER_TARGET ${APPLE_TARGET_TRIPLE})
endif()

if(PLATFORM_INT MATCHES "^MAC_CATALYST")
  set(C_TARGET_FLAGS "-isystem ${CMAKE_OSX_SYSROOT_INT}/System/iOSSupport/usr/include -iframework ${CMAKE_OSX_SYSROOT_INT}/System/iOSSupport/System/Library/Frameworks")
endif()

if(ENABLE_BITCODE_INT)
  set(BITCODE "-fembed-bitcode")
  set(CMAKE_XCODE_ATTRIBUTE_BITCODE_GENERATION_MODE "bitcode")
  set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE "YES")
else()
  set(BITCODE "")
  set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE "NO")
endif()

if(ENABLE_ARC_INT)
  set(FOBJC_ARC "-fobjc-arc")
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES")
else()
  set(FOBJC_ARC "-fno-objc-arc")
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "NO")
endif()

if(NAMED_LANGUAGE_SUPPORT_INT)
  set(OBJC_VARS "-fobjc-abi-version=2 -DOBJC_OLD_DISPATCH_PROTOTYPES=0")
  set(OBJC_LEGACY_VARS "")
else()
  set(OBJC_VARS "")
  set(OBJC_LEGACY_VARS "-fobjc-abi-version=2 -DOBJC_OLD_DISPATCH_PROTOTYPES=0")
endif()

if(NOT ENABLE_VISIBILITY_INT)
  foreach(lang ${languages})
    set(CMAKE_${lang}_VISIBILITY_PRESET "hidden" CACHE INTERNAL "")
  endforeach()
  set(CMAKE_XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "YES")
  set(VISIBILITY "-fvisibility=hidden -fvisibility-inlines-hidden")
else()
  foreach(lang ${languages})
    set(CMAKE_${lang}_VISIBILITY_PRESET "default" CACHE INTERNAL "")
  endforeach()
  set(CMAKE_XCODE_ATTRIBUTE_GCC_SYMBOLS_PRIVATE_EXTERN "NO")
  set(VISIBILITY "-fvisibility=default")
endif()

if(DEFINED APPLE_TARGET_TRIPLE)
  set(APPLE_TARGET_TRIPLE_FLAG "-target ${APPLE_TARGET_TRIPLE}")
endif()

#Check if Xcode generator is used since that will handle these flags automagically
if(CMAKE_GENERATOR MATCHES "Xcode")
  message(STATUS "Not setting any manual command-line buildflags, since Xcode is selected as the generator. Modifying the Xcode build-settings directly instead.")
else()
  set(CMAKE_C_FLAGS "${C_TARGET_FLAGS} ${APPLE_TARGET_TRIPLE_FLAG} ${SDK_NAME_VERSION_FLAGS} ${OBJC_LEGACY_VARS} ${BITCODE} ${VISIBILITY} ${CMAKE_C_FLAGS}")
  set(CMAKE_C_FLAGS_DEBUG "-O0 -g ${CMAKE_C_FLAGS_DEBUG}")
  set(CMAKE_C_FLAGS_MINSIZEREL "-DNDEBUG -Os ${CMAKE_C_FLAGS_MINSIZEREL}")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO "-DNDEBUG -O2 -g ${CMAKE_C_FLAGS_RELWITHDEBINFO}")
  set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O3 ${CMAKE_C_FLAGS_RELEASE}")
  set(CMAKE_CXX_FLAGS "${C_TARGET_FLAGS} ${APPLE_TARGET_TRIPLE_FLAG} ${SDK_NAME_VERSION_FLAGS} ${OBJC_LEGACY_VARS} ${BITCODE} ${VISIBILITY} ${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g ${CMAKE_CXX_FLAGS_DEBUG}")
  set(CMAKE_CXX_FLAGS_MINSIZEREL "-DNDEBUG -Os ${CMAKE_CXX_FLAGS_MINSIZEREL}")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-DNDEBUG -O2 -g ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
  set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3 ${CMAKE_CXX_FLAGS_RELEASE}")
  if(NAMED_LANGUAGE_SUPPORT_INT)
    set(CMAKE_OBJC_FLAGS "${C_TARGET_FLAGS} ${APPLE_TARGET_TRIPLE_FLAG} ${SDK_NAME_VERSION_FLAGS} ${BITCODE} ${VISIBILITY} ${FOBJC_ARC} ${OBJC_VARS} ${CMAKE_OBJC_FLAGS}")
    set(CMAKE_OBJC_FLAGS_DEBUG "-O0 -g ${CMAKE_OBJC_FLAGS_DEBUG}")
    set(CMAKE_OBJC_FLAGS_MINSIZEREL "-DNDEBUG -Os ${CMAKE_OBJC_FLAGS_MINSIZEREL}")
    set(CMAKE_OBJC_FLAGS_RELWITHDEBINFO "-DNDEBUG -O2 -g ${CMAKE_OBJC_FLAGS_RELWITHDEBINFO}")
    set(CMAKE_OBJC_FLAGS_RELEASE "-DNDEBUG -O3 ${CMAKE_OBJC_FLAGS_RELEASE}")
    set(CMAKE_OBJCXX_FLAGS "${C_TARGET_FLAGS} ${APPLE_TARGET_TRIPLE_FLAG} ${SDK_NAME_VERSION_FLAGS} ${BITCODE} ${VISIBILITY} ${FOBJC_ARC} ${OBJC_VARS} ${CMAKE_OBJCXX_FLAGS}")
    set(CMAKE_OBJCXX_FLAGS_DEBUG "-O0 -g ${CMAKE_OBJCXX_FLAGS_DEBUG}")
    set(CMAKE_OBJCXX_FLAGS_MINSIZEREL "-DNDEBUG -Os ${CMAKE_OBJCXX_FLAGS_MINSIZEREL}")
    set(CMAKE_OBJCXX_FLAGS_RELWITHDEBINFO "-DNDEBUG -O2 -g ${CMAKE_OBJCXX_FLAGS_RELWITHDEBINFO}")
    set(CMAKE_OBJCXX_FLAGS_RELEASE "-DNDEBUG -O3 ${CMAKE_OBJCXX_FLAGS_RELEASE}")
  endif()
  set(CMAKE_C_LINK_FLAGS "${C_TARGET_FLAGS} ${SDK_NAME_VERSION_FLAGS} -Wl,-search_paths_first ${CMAKE_C_LINK_FLAGS}")
  set(CMAKE_CXX_LINK_FLAGS "${C_TARGET_FLAGS} ${SDK_NAME_VERSION_FLAGS}  -Wl,-search_paths_first ${CMAKE_CXX_LINK_FLAGS}")
  if(NAMED_LANGUAGE_SUPPORT_INT)
    set(CMAKE_OBJC_LINK_FLAGS "${C_TARGET_FLAGS} ${SDK_NAME_VERSION_FLAGS} -Wl,-search_paths_first ${CMAKE_OBJC_LINK_FLAGS}")
    set(CMAKE_OBJCXX_LINK_FLAGS "${C_TARGET_FLAGS} ${SDK_NAME_VERSION_FLAGS} -Wl,-search_paths_first ${CMAKE_OBJCXX_LINK_FLAGS}")
  endif()
  set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp -arch ${CMAKE_OSX_ARCHITECTURES} ${APPLE_TARGET_TRIPLE_FLAG}")
endif()

# Set global properties
set_property(GLOBAL PROPERTY PLATFORM "${PLATFORM}")
set_property(GLOBAL PROPERTY APPLE_TARGET_TRIPLE "${APPLE_TARGET_TRIPLE_INT}")
set_property(GLOBAL PROPERTY SDK_VERSION "${SDK_VERSION}")
set_property(GLOBAL PROPERTY XCODE_VERSION "${XCODE_VERSION_INT}")
set_property(GLOBAL PROPERTY OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}")

# Export configurable variables for the try_compile() command.
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
        PLATFORM
        XCODE_VERSION_INT
        SDK_VERSION
        NAMED_LANGUAGE_SUPPORT
        DEPLOYMENT_TARGET
        CMAKE_DEVELOPER_ROOT
        CMAKE_OSX_SYSROOT_INT
        ENABLE_BITCODE
        ENABLE_ARC
        CMAKE_ASM_COMPILER
        CMAKE_C_COMPILER
        CMAKE_C_COMPILER_TARGET
        CMAKE_CXX_COMPILER
        CMAKE_CXX_COMPILER_TARGET
        BUILD_LIBTOOL
        CMAKE_INSTALL_NAME_TOOL
        CMAKE_C_FLAGS
        CMAKE_C_DEBUG
        CMAKE_C_MINSIZEREL
        CMAKE_C_RELWITHDEBINFO
        CMAKE_C_RELEASE
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_LINK_FLAGS
        CMAKE_CXX_LINK_FLAGS
        CMAKE_ASM_FLAGS
        )

if(NAMED_LANGUAGE_SUPPORT_INT)
  list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
          CMAKE_OBJC_FLAGS
          CMAKE_OBJC_DEBUG
          CMAKE_OBJC_MINSIZEREL
          CMAKE_OBJC_RELWITHDEBINFO
          CMAKE_OBJC_RELEASE
          CMAKE_OBJCXX_FLAGS
          CMAKE_OBJCXX_DEBUG
          CMAKE_OBJCXX_MINSIZEREL
          CMAKE_OBJCXX_RELWITHDEBINFO
          CMAKE_OBJCXX_RELEASE
          CMAKE_OBJC_LINK_FLAGS
          CMAKE_OBJCXX_LINK_FLAGS
          )
endif()

set(CMAKE_PLATFORM_HAS_INSTALLNAME 1)
set(CMAKE_SHARED_LINKER_FLAGS "-rpath @executable_path/Frameworks -rpath @loader_path/Frameworks")
set(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "-dynamiclib -Wl,-headerpad_max_install_names")
set(CMAKE_SHARED_MODULE_CREATE_C_FLAGS "-bundle -Wl,-headerpad_max_install_names")
set(CMAKE_SHARED_MODULE_LOADER_C_FLAG "-Wl,-bundle_loader,")
set(CMAKE_SHARED_MODULE_LOADER_CXX_FLAG "-Wl,-bundle_loader,")
set(CMAKE_FIND_LIBRARY_SUFFIXES ".tbd" ".dylib" ".so" ".a")
set(CMAKE_SHARED_LIBRARY_SONAME_C_FLAG "-install_name")

# Set the find root to the SDK developer roots.
# Note: CMAKE_FIND_ROOT_PATH is only useful when cross-compiling. Thus, do not set on macOS builds.
if(NOT PLATFORM_INT MATCHES "^MAC.*$")
  list(APPEND CMAKE_FIND_ROOT_PATH "${CMAKE_OSX_SYSROOT_INT}" CACHE INTERNAL "")
  set(CMAKE_IGNORE_PATH "/System/Library/Frameworks;/usr/local/lib" CACHE INTERNAL "")
endif()

# Default to searching for frameworks first.
set(CMAKE_FIND_FRAMEWORK FIRST)

# Set up the default search directories for frameworks.
set(CMAKE_FRAMEWORK_PATH
        ${CMAKE_DEVELOPER_ROOT}/Library/PrivateFrameworks
        ${CMAKE_OSX_SYSROOT_INT}/System/Library/Frameworks
        ${CMAKE_FRAMEWORK_PATH} CACHE INTERNAL "")

# By default, search both the specified iOS SDK and the remainder of the host filesystem.
if(NOT CMAKE_FIND_ROOT_PATH_MODE_PROGRAM)
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH CACHE INTERNAL "")
endif()
if(NOT CMAKE_FIND_ROOT_PATH_MODE_LIBRARY)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH CACHE INTERNAL "")
endif()
if(NOT CMAKE_FIND_ROOT_PATH_MODE_INCLUDE)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH CACHE INTERNAL "")
endif()
if(NOT CMAKE_FIND_ROOT_PATH_MODE_PACKAGE)
  set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH CACHE INTERNAL "")
endif()

#
# Some helper-macros below to simplify and beautify the CMakeFile
#

# This little macro lets you set any Xcode specific property.
macro(set_xcode_property TARGET XCODE_PROPERTY XCODE_VALUE XCODE_RELVERSION)
  set(XCODE_RELVERSION_I "${XCODE_RELVERSION}")
  if(XCODE_RELVERSION_I STREQUAL "All")
    set_property(TARGET ${TARGET} PROPERTY XCODE_ATTRIBUTE_${XCODE_PROPERTY} "${XCODE_VALUE}")
  else()
    set_property(TARGET ${TARGET} PROPERTY XCODE_ATTRIBUTE_${XCODE_PROPERTY}[variant=${XCODE_RELVERSION_I}] "${XCODE_VALUE}")
  endif()
endmacro(set_xcode_property)

# This macro lets you find executable programs on the host system.
macro(find_host_package)
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)
  set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)
  set(_TOOLCHAIN_IOS ${IOS})
  set(IOS OFF)
  find_package(${ARGN})
  set(IOS ${_TOOLCHAIN_IOS})
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
  set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
endmacro(find_host_package)