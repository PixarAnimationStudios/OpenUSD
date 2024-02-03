set(PXR_PREFIX pxr/base)
set(PXR_PACKAGE ts)

set(libs
    arch
    gf
    plug
    tf
    trace
    vt
    ${Boost_PYTHON_LIBRARY}
)

set(include
    ${Boost_INCLUDE_DIRS}
)

set(classes
    data
    diff
    evalCache
    keyFrame
    keyFrameMap
    keyFrameUtils
    loopParams
    mathUtils
    simplify
    spline
    spline_KeyFrames
    tsTest_Evaluator
    tsTest_Museum
    tsTest_SampleBezier
    tsTest_SampleTimes
    tsTest_SplineData
    tsTest_TsEvaluator
    tsTest_Types
    types
    typeRegistry
)

set(pyfiles
    __init__.py
    TsTest_Comparator.py
    TsTest_CompareBaseline.py
    TsTest_Grapher.py
)

set(pycpp
    module.cpp
    wrapKeyFrame.cpp
    wrapLoopParams.cpp
    wrapSimplify.cpp
    wrapSpline.cpp
    wrapTsTest_Evaluator.cpp
    wrapTsTest_Museum.cpp
    wrapTsTest_SampleBezier.cpp
    wrapTsTest_SampleTimes.cpp
    wrapTsTest_SplineData.cpp
    wrapTsTest_TsEvaluator.cpp
    wrapTsTest_Types.cpp
    wrapTypes.cpp
)

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
        api.h
        evaluator.h

    PRIVATE_HEADERS
        wrapUtils.h

    PRIVATE_CLASSES
        evalUtils

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        ${pycpp}

    PYMODULE_FILES
        ${pyfiles}

    DOXYGEN_FILES
        tsTest.dox
)

pxr_build_test(
    testTs_HardToReach
    CPPFILES
        testenv/testTs_HardToReach.cpp
    LIBRARIES
        ts
        gf
        tf
)

pxr_build_test(
    testTsThreadedCOW
    CPPFILES
        testenv/testTsThreadedCOW.cpp
    LIBRARIES
        ts
        tf
        vt
)

pxr_test_scripts(
    testenv/testTsSpline.py
    testenv/testTsSplineAPI.py
    testenv/testTsKeyFrame.py
    testenv/testTsSimplify.py
    testenv/testTsDerivatives.py
    testenv/tsTest_TsFramework.py
)

pxr_install_test_dir(
    SRC testenv/tsTest_TsFramework.testenv
    DEST tsTest_TsFramework
)

pxr_register_test(
    testTsSpline
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsSpline"
)

pxr_register_test(
    testTsSplineAPI
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsSplineAPI"
)

pxr_register_test(
    testTsKeyFrame
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsKeyFrame"
)

pxr_register_test(
    testTsSimplify
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsSimplify"
)

pxr_register_test(
    testTsDerivatives
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsDerivatives"
)

pxr_register_test(
    testTs_HardToReach
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTs_HardToReach"
)

pxr_register_test(
    testTsThreadedCOW
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testTsThreadedCOW"
)

pxr_register_test(
    tsTest_TsFramework
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/tsTest_TsFramework"
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
