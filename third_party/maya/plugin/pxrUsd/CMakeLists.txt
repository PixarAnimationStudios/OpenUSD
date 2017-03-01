set(PXR_PACKAGE pxrUsd)

pxr_plugin(${PXR_PACKAGE}
    LIBRARIES
        pxrUsdMayaGL
        sdf
        tf
        usd
        usdGeom
        usdMaya
        ${MAYA_LIBRARIES}

    INCLUDE_DIRS
        ${MAYA_INCLUDE_DIRS}

    CPPFILES
        alembicChaser.cpp
        plugin.cpp
        usdTranslator.cpp

    PYTHON_FILES
        plugInfo.json
        testenv/testPxrUsdAlembicChaser.py
)

pxr_test_scripts(
        testenv/testPxrUsdAlembicChaser.py
)

pxr_install_test_dir(
    SRC testenv/AlembicChaser
    DEST testPxrUsdAlembicChaser
)
pxr_register_test(testPxrUsdAlembicChaser
    COMMAND "${MAYA_LOCATION}/bin/mayapy ${CMAKE_INSTALL_PREFIX}/tests/testPxrUsdAlembicChaser"
    TESTENV testPxrUsdAlembicChaser
    ENV
        MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/plugin
        MAYA_SCRIPT_PATH=${CMAKE_INSTALL_PREFIX}/third_party/maya/share/usd/plugins/usdMaya/resources
)
