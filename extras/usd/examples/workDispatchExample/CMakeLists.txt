cmake_minimum_required(VERSION 3.27)

project(workDispatchExample
    LANGUAGES CXX
)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set up library target.
add_library(workDispatchExample SHARED)

target_sources(
    workDispatchExample
    PRIVATE
        detachedTask.cpp
        dispatcher.cpp
    PUBLIC
        FILE_SET HEADERS
            FILES
                api.h
                detachedTask.h
                dispatcher.h
                impl.h
                loops.h
                reduce.h
                sort.h
                threadLimits.h
                withScopedParallelism.h
)

target_include_directories(
    workDispatchExample
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_definitions(
    workDispatchExample
    PRIVATE
        "WORK_DISPATCH_EXAMPLE_EXPORT"
)

# Install artifacts and set up package config file
install(
    TARGETS workDispatchExample
    EXPORT workDispatchExampleTargets
    FILE_SET HEADERS
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/workDispatchExample"
)

install(
    EXPORT workDispatchExampleTargets
    FILE workDispatchExampleTargets.cmake
    NAMESPACE workDispatchExample::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/workDispatchExample"
)

configure_package_config_file(
    workDispatchExampleConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/workDispatchExampleConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/workDispatchExample"
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/workDispatchExampleConfig.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/workDispatchExample"
)
