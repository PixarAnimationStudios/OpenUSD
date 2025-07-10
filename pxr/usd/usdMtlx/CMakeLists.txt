set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdMtlx)

set(MATERIALX_STDLIB_RESOURCES "")
if (MATERIALX_STDLIB_DIR)
    add_definitions(-DPXR_MATERIALX_STDLIB_DIR="${MATERIALX_STDLIB_DIR}")

    # We copy the MaterialX standard library files out of the MaterialX 
    # build as a fallback in case the MaterialX standard library directory 
    # cannot be found at runtime.

    # Gather all the files in the MaterialX standard library
    file(GLOB_RECURSE MATERIALX_STDLIB_RESOURCES "${MATERIALX_STDLIB_DIR}/*")
    # Exclude files matching .txt and .md extensions to avoid installing
    # CMakeLists and documentation files
    list(FILTER MATERIALX_STDLIB_RESOURCES EXCLUDE REGEX ".*\\.(txt|md)$")

    if (WIN32)
        foreach(file ${MATERIALX_STDLIB_RESOURCES})
            # Create the relative path for the destination in the resource destination
            file(RELATIVE_PATH _resource_relative ${MATERIALX_STDLIB_DIR} ${file})

            # XXX Create the path for the source file relative to the current source
            # directory. This is done to avoid absolute paths which is not compatible with
            # _install_resource_files in Private.cmake which uses : as a separator. This breaks 
            # for absolute paths on windows which uses them for drive letters (c:\blah)
            # This will still fail if MATERIALX_STDLIB_DIR points to assets on a different
            # drive than the usd source directory on windows
            file(RELATIVE_PATH _source_relative ${CMAKE_CURRENT_SOURCE_DIR} ${file})

            # Make a list of : separated paths of source files and destination files for
            # the materialx library files so they can be installed in the resource
            # directory for the plugin
            list(APPEND MATERIALX_STDLIB_FILES "${_source_relative}:libraries/${_resource_relative}")
        endforeach()

        set(MATERIALX_STDLIB_RESOURCES ${MATERIALX_STDLIB_FILES})
    endif()
endif()

pxr_library(usdMtlx
    INCLUDE_SCHEMA_FILES

    LIBRARIES
        arch
        gf
        sdf
        sdr
        tf
        vt
        usd
        usdGeom
        usdShade
        usdUI
        usdUtils
        MaterialXCore
        MaterialXFormat

    CPPFILES
        discovery.cpp
        parser.cpp

    PRIVATE_CLASSES
        backdoor
        fileFormat

    PUBLIC_CLASSES
        debugCodes
        reader
        utils

    PUBLIC_HEADERS
        api.h

    PYMODULE_CPPFILES
        wrapBackdoor.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        ${MATERIALX_STDLIB_RESOURCES}

    DOXYGEN_FILES
        overview.dox
)

pxr_test_scripts(
    testenv/testUsdMtlxDiscovery.py
    testenv/testUsdMtlxFileFormat.py
    testenv/testUsdMtlxParser.py
)

pxr_install_test_dir(
    SRC testenv/testUsdMtlxDiscovery.testenv
    DEST testUsdMtlxDiscovery
)

pxr_install_test_dir(
    SRC testenv/testUsdMtlxFileFormat.testenv
    DEST testUsdMtlxFileFormat
)

pxr_install_test_dir(
    SRC testenv/testUsdMtlxParser.testenv
    DEST testUsdMtlxParser
)

pxr_register_test(testUsdMtlxDiscovery
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdMtlxDiscovery"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdMtlxFileFormat
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdMtlxFileFormat"
    DIFF_COMPARE Looks.usda NodeGraphs.usda GraphlessNodes.usda usd_preview_surface_gold.usda Include.usda Include_From_Usdz.usda LocalCustomNodes.usda NestedLocalCustomNodes.usda
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdMtlxParser
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdMtlxParser"
    EXPECTED_RETURN_CODE 0
)
