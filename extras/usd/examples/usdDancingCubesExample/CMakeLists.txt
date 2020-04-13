set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdDancingCubesExample)

pxr_plugin(${PXR_PACKAGE}
    LIBRARIES
        tf
        gf
        vt
        pcp
        sdf
        usd

    PUBLIC_CLASSES
        fileFormat

    PRIVATE_CLASSES
        data
        dataImpl

    RESOURCE_FILES
        plugInfo.json
        dancingCubes.usda:usdDancingCubesExample/dancingCubes.usda
        empty.usddancingcubesexample:usdDancingCubesExample/empty.usddancingcubesexample

    DISABLE_PRECOMPILED_HEADERS
)

pxr_test_scripts(
     testenv/testUsdDancingCubesExample.py
)

pxr_install_test_dir(
    SRC testenv/testUsdDancingCubesExample
    DEST testUsdDancingCubesExample
)

# XXX:
# Baselines for this test were generated on a build on Linux
# using GCC. The results may differ on other platforms and compilers
# due to floating point precision issues. So, we only perform the
# comparison on matching platforms. Ideally, we'd have a diff tool
# with numerical tolerances that would allow using the same baseline
# on all platforms.
if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux" AND
    "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    pxr_register_test(testUsdDancingCubesExample
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdDancingCubesExample"
        DIFF_COMPARE dynamicContents.usda newDynamicContents.usda
        EXPECTED_RETURN_CODE 0
        ENV
            PXR_PLUGINPATH_NAME=${CMAKE_INSTALL_PREFIX}/share/usd/examples/plugin/usdDancingCubesExample/resources
    )
else()
    pxr_register_test(testUsdDancingCubesExample
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdDancingCubesExample"
        EXPECTED_RETURN_CODE 0
        ENV
            PXR_PLUGINPATH_NAME=${CMAKE_INSTALL_PREFIX}/share/usd/examples/plugin/usdDancingCubesExample/resources
    )
endif()
