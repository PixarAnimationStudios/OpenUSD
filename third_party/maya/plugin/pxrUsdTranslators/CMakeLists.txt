set(PXR_PACKAGE pxrUsdTranslators)

pxr_plugin(${PXR_PACKAGE}
    MAYA_PLUGIN

    LIBRARIES
        arch
        gf
        kind
        sdf
        tf
        usd
        usdGeom
        usdMaya
        usdShade
        usdSkel
        usdUtils
        vt
        ${Boost_PYTHON_LIBRARY}
        ${MAYA_LIBRARIES}

    INCLUDE_DIRS
        ${MAYA_INCLUDE_DIRS}

    PRIVATE_CLASSES
        cameraWriter
        fileTextureWriter
        instancerWriter
        jointWriter
        locatorWriter
        meshWriter
        nurbsCurveWriter
        nurbsSurfaceWriter
        particleWriter

    CPPFILES
        cameraReader.cpp
        materialReader.cpp
        meshReader.cpp
        meshWriter_Primvars.cpp
        meshWriter_Skin.cpp
        meshWriter_Subdiv.cpp
        nurbsCurvesReader.cpp
        nurbsPatchReader.cpp
        plugin.cpp
        scopeReader.cpp
        skelReader.cpp
        xformReader.cpp

    PRIVATE_HEADERS
        api.h

    RESOURCE_FILES
        plugInfo.json

    DISABLE_PRECOMPILED_HEADERS
)

pxr_test_scripts(
    testenv/testPxrUsdTranslators.py
    testenv/testPxrUsdTranslatorsScope.py
)

pxr_register_test(testPxrUsdTranslators
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPxrUsdTranslators"
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/ScopeTest
    DEST testPxrUsdTranslatorsScope
)
pxr_register_test(testPxrUsdTranslatorsScope
    CUSTOM_PYTHON "${MAYA_BASE_DIR}/bin/mayapy"
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPxrUsdTranslatorsScope"
    TESTENV testPxrUsdTranslatorsScope
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)
