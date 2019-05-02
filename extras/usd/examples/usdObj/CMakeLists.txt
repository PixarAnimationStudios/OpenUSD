set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdObj)

IF(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
ENDIF()

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

