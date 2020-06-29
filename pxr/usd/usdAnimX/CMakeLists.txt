set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdAnimX)

pxr_library(${PXR_PACKAGE}
    LIBRARIES
        arch
        tf
        gf
        trace
        vt
        work
        pcp
        sdf
        usd
        ${Boost_PYTHON_LIBRARY}
        ${TBB_tbb_LIBRARY}

    PUBLIC_CLASSES
        fileFormat
        fCurve
        fCurveOp
        
    PUBLIC_HEADERS
        api.h
        tokens.h
        animx.h
        keyframe.h

    PRIVATE_CLASSES
        tokens
        animx
        keyframe
        curve
        data

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapFCurve.cpp
        wrapFCurveOp.cpp
        wrapTokens.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
        schema.usda:usdAnimX/schema.usda
        animX.usda:usdAnimX/animX.usda
        empty.animx:usdAnimX/empty.animx
)

pxr_test_scripts(
     testenv/testUsdAnimX.py
)

pxr_install_test_dir(
    SRC testenv/testUsdAnimX
    DEST testUsdAnimX
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
    pxr_register_test(testUsdAnimX
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdAnimX"
        DIFF_COMPARE dynamicContents.usda newDynamicContents.usda
        EXPECTED_RETURN_CODE 0
        ENV
            PXR_PLUGINPATH_NAME=${CMAKE_INSTALL_PREFIX}/share/usd/examples/plugin/usdAnimX/resources
    )
else()
    pxr_register_test(testUsdAnimX
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdAnimX"
        EXPECTED_RETURN_CODE 0
        ENV
            PXR_PLUGINPATH_NAME=${CMAKE_INSTALL_PREFIX}/share/usd/examples/plugin/usdAnimX/resources
    )
endif()
