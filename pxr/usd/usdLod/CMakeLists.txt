set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdLod)

pxr_library(usdLod
    INCLUDE_SCHEMA_FILES

    LIBRARIES
        tf
        gf
        vt
        sdf
        usd
        usdGeom

    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        distanceHeuristicQuery
        heuristicQuery
        screenSizeHeuristicQuery

    PYMODULE_FILES
        __init__.py

    PYMODULE_CPPFILES
        wrapDistanceHeuristicQuery.cpp
        wrapHeuristicQuery.cpp
        wrapScreenSizeHeuristicQuery.cpp

    DOXYGEN_FILES
        overview.dox
)

pxr_build_test(testUsdLodGetLod
    LIBRARIES
        tf
        gf
        usd
        usdGeom
        usdLod
    CPPFILES
        testenv/testUsdLodGetLod.cpp
)

pxr_test_scripts(
    testenv/testUsdLodRootAPI.py
    testenv/testUsdLodDistanceHeuristic.py
    testenv/testUsdLodScreenSizeHeuristic.py
)

pxr_register_test(testUsdLodRootAPI
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdLodRootAPI"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdLodDistanceHeuristic
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdLodDistanceHeuristic"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdLodScreenSizeHeuristic
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdLodScreenSizeHeuristic"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdLodGetLod
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdLodGetLod"
    EXPECTED_RETURN_CODE 0
)
