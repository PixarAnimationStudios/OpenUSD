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
)

