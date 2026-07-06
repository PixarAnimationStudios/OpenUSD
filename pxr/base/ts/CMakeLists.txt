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

set(privateHeaders
    ""
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

# Headers and cpp files for the tsTest framework. 
#
# The cpp files are built with the unit tests and Python module instead
# instead of the shared library to avoid having the library's ABI depend
# on whether tests are enabled or disabled. However, the headers are
# specified as private headers on ts itself so that include paths can
# still be specified as full paths, which is more conventional in the
# source tree.
set(tstestheaders
    tsTest_Museum.h
    tsTest_SampleBezier.h
    tsTest_SampleTimes.h
    tsTest_TsEvaluator.h
    tsTest_Types.h
)    

set(tstestcpp
    tsTest_Museum.cpp
    tsTest_SampleBezier.cpp
    tsTest_SampleTimes.cpp
    tsTest_TsEvaluator.cpp
)

if (${PXR_BUILD_TESTS})
    list(APPEND privateHeaders
        ${tstestheaders}
    )

    list(APPEND pycpp
        ${tstestcpp}
        wrapTsTest_Museum.cpp
        wrapTsTest_SampleBezier.cpp
        wrapTsTest_SampleTimes.cpp
        wrapTsTest_TsEvaluator.cpp
        wrapTsTest_Types.cpp
    )
    list(APPEND pyfiles
        TsTest_Comparator.py
        TsTest_Baseliner.py
        TsTest_Grapher.py
    )
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

    PRIVATE_HEADERS
        ${privateHeaders}

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

    # XXX:
    # Precompiled headers are disabled because the source files for the tsTest
    # framework are compiled into the Python module for ts _and_ into the
    # various C++ unit tests below. When precompiled headers are enabled, the 
    # build system adds compile flags that are used whenever any source file
    # in PYMODULE_CPPFILES are built, regardless of their target. This is fine
    # when the tsTest files are built in the Python module, but it causes
    # conflicts and errors when they are built with the unit tests. The easy
    # workaround is to just disable precompiled headers in this library.
    #
    # Longer term, the build system should start using cmake's built-in
    # target_precompile_headers function instead of implementing its own
    # handling. We could then disable precompiled headers on a per-target
    # basis in the ts_cpp_test helper function below.
    DISABLE_PRECOMPILED_HEADERS
)

if (${PXR_BUILD_TESTS})
    if (TARGET _ts)
        target_compile_definitions(_ts PRIVATE TS_BUILD_TEST_FRAMEWORK)
    endif()
endif()

if (PXR_TS_DEFAULT_ANTI_REGRESSION_AUTHORING_MODE)
    set(val "${PXR_TS_DEFAULT_ANTI_REGRESSION_AUTHORING_MODE}")
    target_compile_definitions(
        ts PRIVATE "-DPXR_TS_DEFAULT_ANTI_REGRESSION_AUTHORING_MODE=${val}")
endif()

# Helper function that automatically includes the tsTest framework when
# building C++ tests.
function(ts_cpp_test TEST_NAME)
    set(multiValueArgs CPPFILES LIBRARIES)
    cmake_parse_arguments(test "" "" "${multiValueArgs}" ${ARGN})

    pxr_build_test(
        ${TEST_NAME}
        CPPFILES
            ${tstestcpp}
            ${test_CPPFILES}
        LIBRARIES
            ${test_LIBRARIES}
    )
endfunction() # ts_cpp_test

ts_cpp_test(
    testTsThreadedCOW
    CPPFILES
        testenv/testTsThreadedCOW.cpp
    LIBRARIES
        ts
        tf
)

ts_cpp_test(
    testTsRegressionPreventerCpp
    CPPFILES
        testenv/testTsRegressionPreventerCpp.cpp
    LIBRARIES
        gf
        tf
        ts
)

ts_cpp_test(
    testTsSegmentIterator
    CPPFILES
        testenv/testTsSegmentIterator.cpp
    LIBRARIES
        gf
        tf
        ts
)

ts_cpp_test(
    testTsSplineAPI
    CPPFILES
        testenv/testTsSplineAPI.cpp
    LIBRARIES
        ts
        vt
        tf
)

ts_cpp_test(
    testTsSplineBaking
    CPPFILES
        testenv/testTsSplineBaking.cpp
    LIBRARIES
        gf
        ts
        tf
)

ts_cpp_test(
    testTsSplineDiff
    CPPFILES
        testenv/testTsSplineDiff.cpp
    LIBRARIES
        gf
        tf
        ts
)

ts_cpp_test(
    testTsSplineEval
    CPPFILES
        testenv/testTsSplineEval.cpp
    LIBRARIES
        ts
        vt
        tf
)

ts_cpp_test(
    testTsSplineSampling
    CPPFILES
        testenv/testTsSplineSampling.cpp
    LIBRARIES
        ts
        vt
        tf
)

ts_cpp_test(
    testTsSplineTransforms
    CPPFILES
        testenv/testTsSplineTransforms.cpp
    LIBRARIES
        ts
        vt
        tf
)

ts_cpp_test(
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
    testTsSplineTransforms
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsSplineTransforms"
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
