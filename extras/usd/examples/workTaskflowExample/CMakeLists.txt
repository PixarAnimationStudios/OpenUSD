cmake_minimum_required(VERSION 3.26)

project(workTaskflowExample
    LANGUAGES CXX
)

include(CMakePackageConfigHelpers)
include(FetchContent)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Fetch and install Taskflow unless the user has already supplied that
# dependency and it can be found via a call to find_package. This would
# typically happen if the user made Taskflow's config file package visible
# to cmake, e.g. by setting Taskflow_DIR to the directory where Taskflow's
# config files are installed.
#
# Note that by default FetchContent doesn't provide any diagnostic output,
# but users can specify FETCHCONTENT_QUIET=OFF when running cmake to get
# more information.
FetchContent_Declare(
  Taskflow
  GIT_REPOSITORY https://github.com/taskflow/taskflow.git
  GIT_TAG        2dfa50a567d48b8439807f5da8a041ba64d4fb63 # v3.10.0
  FIND_PACKAGE_ARGS CONFIG
)

# Disable tests and examples since we don't need them and they can
# be slow-ish to build.
set(TF_BUILD_EXAMPLES OFF CACHE INTERNAL BOOL)
set(TF_BUILD_TESTS OFF CACHE INTERNAL BOOL)

FetchContent_MakeAvailable(Taskflow)

# Make sure that Taskflow_DIR is set to the location of the _installed_
# Taskflow package config file used to build this library. We'll use this
# below when setting up our package config file.
# 
# If we found an externally-supplied Taskflow package above, Taskflow_FOUND
# will be set and Taskflow_DIR will already be set to the location we want.
#
# If we built Taskflow ourselves, Taskflow_FOUND will not be set and
# Taskflow_DIR will be set to some internal build location. We want to
# reset that to the location where Taskflow will be installed.
if (NOT Taskflow_FOUND)
    set(Taskflow_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/Taskflow")
endif()

# Set up library target.
add_library(workTaskflowExample SHARED)

target_link_libraries(
    workTaskflowExample
    PUBLIC
        Taskflow::Taskflow
)

target_sources(
    workTaskflowExample
    PRIVATE
        detachedTask.cpp
        dispatcher.cpp
    PUBLIC
        FILE_SET HEADERS
            FILES
                api.h
                detachedTask.h
                dispatcher.h
                executorStack.h
                impl.h
                loops.h
                reduce.h
                sort.h
                threadLimits.h
                withScopedParallelism.h
)

target_include_directories(
    workTaskflowExample
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_definitions(
    workTaskflowExample
    PRIVATE
        "WORK_TASKFLOW_EXAMPLE_EXPORT"
)

# Install artifacts and set up package config file
install(
    TARGETS workTaskflowExample
    EXPORT workTaskflowExampleTargets
    FILE_SET HEADERS
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/workTaskflowExample"
)

install(
    EXPORT workTaskflowExampleTargets
    FILE workTaskflowExampleTargets.cmake
    NAMESPACE workTaskflowExample::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/workTaskflowExample"
)

configure_package_config_file(
    workTaskflowExampleConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/workTaskflowExampleConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/workTaskflowExample"
    PATH_VARS Taskflow_DIR
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/workTaskflowExampleConfig.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/workTaskflowExample"
)
