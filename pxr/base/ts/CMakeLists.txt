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

set(privateClasses
    diff
    iterator
    sample
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
    segment
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

set(tsTestHeaders
    tsTest_Museum.h
    tsTest_SampleBezier.h
    tsTest_SampleTimes.h
    tsTest_SplineData.h
    tsTest_TsEvaluator.h
    tsTest_Types.h
)

if (${PXR_BUILD_TESTS})
    list(APPEND headers
        ${tsTestHeaders}
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

if (${PXR_BUILD_ANIMX_TESTS})
    list(APPEND libs ${ANIMX_LIBRARY})
    list(APPEND include ${ANIMX_INCLUDES})
    list(APPEND headers tsTest_AnimXEvaluator.h)
    list(APPEND pycpp wrapTsTest_AnimXEvaluator.cpp)
endif()

pxr_library(ts
    LIBRARIES
        ${libs}

    INCLUDE_DIRS
        ${include}

    PRIVATE_CLASSES
        ${privateClasses}

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
    set(tsTestCpp
        tsTest_Museum.cpp
        tsTest_SampleBezier.cpp
        tsTest_SampleTimes.cpp
        tsTest_SplineData.cpp
        tsTest_TsEvaluator.cpp
    )
    if (${PXR_BUILD_ANIMX_TESTS})
        list(APPEND tsTestCpp tsTest_AnimXEvaluator.cpp)
    endif()

    pxr_library(tsTest
        LIBRARIES
            ts

        INCLUDE_DIRS
            ${include}

        CPPFILES
            ${tsTestCpp}
    )
    # TsTest headers currently use TS_API; export those symbols from tsTest.
    target_compile_definitions(tsTest PRIVATE TS_EXPORTS=1)

    if (TARGET _ts)
        target_compile_definitions(_ts PRIVATE TS_BUILD_TEST_FRAMEWORK)
        target_link_libraries(_ts PRIVATE tsTest)
    endif()
endif()

if (${PXR_BUILD_ANIMX_TESTS})
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
    testTsRegressionPreventerCpp
    CPPFILES
        testenv/testTsRegressionPreventerCpp.cpp
    LIBRARIES
        gf
        tf
        ts
)

pxr_build_test(
    testTsSegmentIterator
    CPPFILES
        testenv/testTsSegmentIterator.cpp
    LIBRARIES
        gf
        tf
        ts
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
    testTsSplineBaking
    CPPFILES
        testenv/testTsSplineBaking.cpp
    LIBRARIES
        gf
        tsTest
        ts
        tf
)

pxr_build_test(
    testTsSplineDiff
    CPPFILES
        testenv/testTsSplineDiff.cpp
    LIBRARIES
        gf
        tf
        ts
)

pxr_build_test(
    testTsSplineEval
    CPPFILES
        testenv/testTsSplineEval.cpp
    LIBRARIES
        ts
        vt
        tf
)

pxr_build_test(
    testTsSplineSampling
    CPPFILES
        testenv/testTsSplineSampling.cpp
    LIBRARIES
        tsTest
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
    testenv/testTsBugs.py
    testenv/testTsRegressionPreventer.py
    testenv/testTsSplineAPIPy.py
    testenv/testTsSplineBreakdown.py
    testenv/testTsSplineValueBlock.py
    testenv/testTsTangentAlgorithms.py
    testenv/tsTest_TsFramework.py
    testenv/tsTest_TsVsBezier.py
)

# This test is disabled for emscripten due to requiring resources from
# the virtual filesystem which is not currently implemented for emscripten
# tests
if (NOT EMSCRIPTEN)
    pxr_register_test(
        testTsSplineSampling
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsSplineSampling"
        TESTENV testenv/testTsSplineSampling.testenv
        DIFF_COMPARE testTsSplineSampling.txt
    )
endif()

pxr_register_test(
    testTsDerivatives
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsDerivatives"
)

pxr_register_test(
    testTsBugs
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsBugs"
)

pxr_register_test(
    testTsRegressionPreventer
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsRegressionPreventer"
    TESTENV testenv/testTsRegressionPreventer.testenv
)

pxr_register_test(
    testTsSplineAPIPy
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsSplineAPIPy"
)

pxr_register_test(
    testTsSplineBreakdown
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsSplineBreakdown"
)

pxr_register_test(
    testTsSplineValueBlock
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsSplineValueBlock"
)

pxr_register_test(
    testTsTangentAlgorithms
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsTangentAlgorithms"
)

pxr_register_test(
    testTsThreadedCOW
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsThreadedCOW"
)

pxr_register_test(
    testTsRegressionPreventerCpp
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsRegressionPreventerCpp"
)

pxr_register_test(
    testTsSegmentIterator
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsSegmentIterator"
)

pxr_register_test(
    testTsSplineAPI
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsSplineAPI"
)

pxr_register_test(
    testTsSplineBaking
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsSplineBaking"
)

pxr_register_test(
    testTsSplineEval
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsSplineEval"
)

pxr_register_test(
    testTsTangentConversion
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsTangentConversion"
)

pxr_register_test(
    tsTest_TsFramework
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/tsTest_TsFramework"
    TESTENV testenv/tsTest_TsFramework.testenv
)

pxr_register_test(
    tsTest_TsVsBezier
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/tsTest_TsVsBezier"
)
