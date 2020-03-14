set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdRecursivePayloadsExample)

pxr_plugin(${PXR_PACKAGE}
    LIBRARIES
        tf
        gf
        sdf
        usd

    CPPFILES
        fileFormat.cpp

    RESOURCE_FILES
        plugInfo.json
        root.usda:usdRecursivePayloadsExample/root.usda
        params.usda:usdRecursivePayloadsExample/params.usda
        payload.usda:usdRecursivePayloadsExample/payload.usda
        cone.usdrecursivepayloadsexample:usdRecursivePayloadsExample/cone.usdrecursivepayloadsexample
        sphere.usdrecursivepayloadsexample:usdRecursivePayloadsExample/sphere.usdrecursivepayloadsexample

    DISABLE_PRECOMPILED_HEADERS
)

pxr_test_scripts(
     testenv/testUsdRecursivePayloadsExample.py
)

pxr_install_test_dir(
    SRC testenv/testUsdRecursivePayloadsExample
    DEST testUsdRecursivePayloadsExample
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
    pxr_register_test(testUsdRecursivePayloadsExample
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdRecursivePayloadsExample"
        DIFF_COMPARE flattenedContents.usda newFlattenedContents.usda
        EXPECTED_RETURN_CODE 0
        ENV
            PXR_PLUGINPATH_NAME=${CMAKE_INSTALL_PREFIX}/share/usd/examples/plugin/usdRecursivePayloadsExample/resources
    )
else()
    pxr_register_test(testUsdRecursivePayloadsExample
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdRecursivePayloadsExample"
        EXPECTED_RETURN_CODE 0
        ENV
            PXR_PLUGINPATH_NAME=${CMAKE_INSTALL_PREFIX}/share/usd/examples/plugin/usdRecursivePayloadsExample/resources
    )
endif()
