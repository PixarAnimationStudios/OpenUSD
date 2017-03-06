set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdAbc)

if(NOT ALEMBIC_FOUND)
    message(WARNING "Not building ${PXR_PACKAGE} because of missing dependency: Alembic")
    return()
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

    INCLUDE_DIRS
        ${ALEMBIC_INCLUDE_DIR}
        ${HDF5_INCLUDE_DIRS}

    PRIVATE_CLASSES
        alembicData
        alembicFileFormat
        alembicReader
        alembicUtil
        alembicWriter

    PRIVATE_HEADERS
        alembicTest.h

    CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        alembicTest.cpp
        wrapAlembicTest.cpp

    PYTHON_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
)
