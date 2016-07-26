set(PXR_PACKAGE usdObj)

pxr_plugin(${PXR_PACKAGE}
    LIBRARIES
        tf
        gf
        sdf
        usd
        usdGeom

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
        ${PYTHON_INCLUDE_DIRS}

    CPPFILES
        fileFormat.cpp
        stream.cpp
        streamIO.cpp
        translator.cpp

    RESOURCE_FILES
        plugInfo.json
)

