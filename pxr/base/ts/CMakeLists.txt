set(PXR_PREFIX pxr/base)
set(PXR_PACKAGE ts)

set(libs
    vt
    gf
    tf
)

set(include
    ""
)

set(classes
    binary
    debugCodes
    eval
    knot
    knotData
    knotMap
    raii
    regressionPreventer
    spline
    splineData
    tangentConversions
    typeHelpers
    types
)

set(headers
    api.h
    valueTypeDispatch.h
)

set(pyfiles
    __init__.py
)

set(pycpp
    module.cpp
    wrapKnot.cpp
    wrapKnotMap.cpp
    wrapRaii.cpp
    wrapRegressionPreventer.cpp
    wrapSpline.cpp
    wrapTangentConversions.cpp
    wrapTypes.cpp
)

if (${PXR_BUILD_TESTS})
    list(APPEND classes
        tsTest_Museum
        tsTest_SampleBezier
        tsTest_SampleTimes
        tsTest_SplineData
        tsTest_TsEvaluator
    )
    list(APPEND headers
        tsTest_Types.h
    )
    list(APPEND pycpp
        wrapTsTest_Museum.cpp
        wrapTsTest_SampleBezier.cpp
        wrapTsTest_SampleTimes.cpp
        wrapTsTest_SplineData.cpp
        wrapTsTest_TsEvaluator.cpp
        wrapTsTest_Types.cpp
    )
    list(APPEND pyfiles
        TsTest_Comparator.py
        TsTest_Baseliner.py
        TsTest_Grapher.py
    )
endif()

if (${PXR_BUILD_MAYAPY_TESTS})
    list(APPEND pyfiles
        TsTest_MayapyDriver.py
        TsTest_MayapyEvaluator.py
    )
endif()

if (${PXR_BUILD_ANIMX_TESTS})
    list(APPEND libs ${ANIMX_LIBRARY})
    list(APPEND include ${ANIMX_INCLUDES})
    list(APPEND classes tsTest_AnimXEvaluator)
    list(APPEND pycpp wrapTsTest_AnimXEvaluator.cpp)
endif()

pxr_library(ts
    LIBRARIES
        ${libs}

    INCLUDE_DIRS
        ${include}

    PUBLIC_CLASSES
        ${classes}

    PUBLIC_HEADERS
        ${headers}

    PYMODULE_CPPFILES
        ${pycpp}

    PYMODULE_FILES
        ${pyfiles}

    DOXYGEN_FILES
        doxygen/overview.md
        doxygen/status.md
        doxygen/regression.md
        doxygen/tsTest.dox
        doxygen/images/boldS.png
        doxygen/images/centerVertical.png
        doxygen/images/endVert.png
        doxygen/images/fourThirdOneThird.png
        doxygen/images/keepRatio.png
        doxygen/images/keepStart.png
        doxygen/images/nearVertical.png
        doxygen/images/oneThirdFourThird.png
        doxygen/images/regressiveS.png
        doxygen/images/regressiveSStandard.png
        doxygen/images/startVert.png
        doxygen/images/tanLens.png
)

if (${PXR_BUILD_TESTS})
    target_compile_definitions(ts PRIVATE TS_BUILD_TEST_FRAMEWORK)
    if (TARGET _ts)
        target_compile_definitions(_ts PRIVATE TS_BUILD_TEST_FRAMEWORK)
    endif()
endif()

if (${PXR_BUILD_ANIMX_TESTS})
    target_compile_definitions(ts PRIVATE TS_BUILD_ANIMX_TEST_FRAMEWORK)
    if (TARGET _ts)
        target_compile_definitions(_ts PRIVATE TS_BUILD_ANIMX_TEST_FRAMEWORK)
    endif()
endif()

if (PXR_TS_DEFAULT_ANTI_REGRESSION_AUTHORING_MODE)
    set(val "${PXR_TS_DEFAULT_ANTI_REGRESSION_AUTHORING_MODE}")
    target_compile_definitions(
        ts PRIVATE "-DPXR_TS_DEFAULT_ANTI_REGRESSION_AUTHORING_MODE=${val}")
endif()

pxr_build_test(
    testTsThreadedCOW
    CPPFILES
        testenv/testTsThreadedCOW.cpp
    LIBRARIES
        ts
        tf
)

pxr_build_test(
    testTsSplineAPI
    CPPFILES
        testenv/testTsSplineAPI.cpp
    LIBRARIES
        ts
        vt
        tf
)

pxr_build_test(
    testTsTangentConversion
    CPPFILES
        testenv/testTsTangentConversion.cpp
    LIBRARIES
        ts
        vt
        tf
)

pxr_test_scripts(
    testenv/testTsDerivatives.py
    testenv/testTsRegressionPreventer.py
    testenv/tsTest_TsFramework.py
    testenv/tsTest_TsVsBezier.py
)

pxr_install_test_dir(
    SRC testenv/tsTest_TsFramework.testenv
    DEST tsTest_TsFramework
)

pxr_install_test_dir(
    SRC testenv/testTsRegressionPreventer.testenv
    DEST testTsRegressionPreventer
)

pxr_register_test(
    testTsDerivatives
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsDerivatives"
)

pxr_register_test(
    testTsRegressionPreventer
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsRegressionPreventer"
)

pxr_register_test(
    testTsThreadedCOW
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsThreadedCOW"
)

pxr_register_test(
    testTsSplineAPI
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsSplineAPI"
)

pxr_register_test(
    testTsTangentConversion
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsTangentConversion"
)

pxr_register_test(
    tsTest_TsFramework
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/tsTest_TsFramework"
)

pxr_register_test(
    tsTest_TsVsBezier
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/tsTest_TsVsBezier"
)

if (${PXR_BUILD_MAYAPY_TESTS})
    pxr_test_scripts(
        testenv/tsTest_MayapyFramework.py
        testenv/tsTest_TsVsMayapy.py
    )

    pxr_install_test_dir(
        SRC testenv/tsTest_MayapyFramework.testenv
        DEST tsTest_MayapyFramework
    )

    set(mayapyBin "${MAYAPY_LOCATION}/mayapy")

    set(cmd "${CMAKE_INSTALL_PREFIX}/tests/tsTest_MayapyFramework")
    pxr_register_test(
        tsTest_MayapyFramework
        PYTHON
        COMMAND "${cmd} ${mayapyBin}"
    )

    set(cmd "${CMAKE_INSTALL_PREFIX}/tests/tsTest_TsVsMayapy")
    pxr_register_test(
        tsTest_TsVsMayapy
        PYTHON
        COMMAND "${cmd} ${mayapyBin}"
    )
endif()

if (${PXR_BUILD_ANIMX_TESTS})
    pxr_test_scripts(testenv/tsTest_AnimXFramework.py)

    pxr_install_test_dir(
        SRC testenv/tsTest_AnimXFramework.testenv
        DEST tsTest_AnimXFramework
    )

    pxr_register_test(
        tsTest_AnimXFramework
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/tsTest_AnimXFramework"
    )
endif()

if (${PXR_BUILD_MAYAPY_TESTS} AND ${PXR_BUILD_ANIMX_TESTS})
    pxr_test_scripts(testenv/tsTest_MayapyVsAnimX.py)

    set(cmd "${CMAKE_INSTALL_PREFIX}/tests/tsTest_MayapyVsAnimX")
    set(mayapyBin "${MAYAPY_LOCATION}/mayapy")
    pxr_register_test(
        tsTest_MayapyVsAnimX
        PYTHON
        COMMAND "${cmd} ${mayapyBin}"
    )
endif()
