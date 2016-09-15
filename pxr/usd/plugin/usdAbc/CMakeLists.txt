set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdAbc)

if(NOT ALEMBIC_FOUND)
    message(WARNING "Not building ${PXR_PACKAGE} because of missing dependency: Alembic")
    return()
endif()

if(WIN32)
    set(MODULE_ENTRYPOINT_CPP DllMain.cpp)
endif()

pxr_plugin(usdAbc
    LIBRARIES
        tf
        sdf
        usd
        usdGeom
        ${ALEMBIC_LIBRARIES}
        ${HDF5_LIBRARIES}
        ${HDF5_HL_LIBRARIES}
        ${OPENEXR_Iex_LIBRARY}
        ${OPENEXR_IexMath_LIBRARY}
        ${OPENEXR_Imath_LIBRARY}
        ${ZLIB_LIBRARY_RELEASE}

    INCLUDE_DIRS
        ${ALEMBIC_INCLUDE_DIR}
        ${HDF5_INCLUDE_DIRS}

    PUBLIC_CLASSES
        alembicFileFormat
        alembicTest

    PRIVATE_CLASSES
        alembicData
        alembicReader
        alembicUtil
        alembicWriter

    PUBLIC_HEADERS
        api.h

    CPPFILES
        moduleDeps.cpp
        ${MODULE_ENTRYPOINT_CPP}

    PYMODULE_CPPFILES
        module.cpp
        wrapAlembicTest.cpp

    PYTHON_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
)
