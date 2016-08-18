set(PXR_PACKAGE pxrUsd)

pxr_plugin(${PXR_PACKAGE}
    LIBRARIES
        ar
        arch
        usdMaya
        pxrUsdMayaGL
        ${MAYA_LIBRARIES}

    INCLUDE_DIRS
		${TBB_INCLUDE_DIRS}
        ${MAYA_INCLUDE_DIRS}

    CPPFILES
        plugin.cpp
        usdTranslator.cpp

    PYTHON_FILES
        plugInfo.json
)

