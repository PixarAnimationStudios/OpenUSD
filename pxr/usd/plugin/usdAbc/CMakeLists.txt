set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdAbc)

if(NOT ALEMBIC_FOUND)
    message(WARNING "Not building ${PXR_PACKAGE} because of missing dependency: Alembic")
    return()
endif()

set(MULTIVERSE_DEPS "")
if (PXR_ENABLE_MULTIVERSE_SUPPORT)
    list(APPEND MULTIVERSE_DEPS ${GIT2_LIB})
    list(APPEND MULTIVERSE_DEPS ${Boost_FILESYSTEM_LIBRARY})
    list(APPEND MULTIVERSE_DEPS ${Boost_SYSTEM_LIBRARY})
    add_definitions(-DUSDABC_MULTIVERSE_SUPPORT)
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
        ${MULTIVERSE_DEPS}

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

    CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapAlembicTest.cpp

    PYTHON_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
)
