set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdAbc)

if(NOT ALEMBIC_FOUND)
    message(WARNING "Not building ${PXR_PACKAGE} because of missing dependency: Alembic")
    return()
endif()

set(optionalLibs "")
set(optionalIncludeDirs "")

if (PXR_ENABLE_MULTIVERSE_SUPPORT)
    add_definitions(-DUSDABC_MULTIVERSE_SUPPORT)
endif()

if (PXR_ENABLE_HDF5_SUPPORT)
   add_definitions(-DUSDABC_HDF5_SUPPORT)
   list(APPEND optionalLibs ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES})
   list(APPEND optionalIncludeDirs ${HDF5_INCLUDE_DIRS})
endif()

pxr_plugin(usdAbc
    LIBRARIES
        tf
        work
        sdf
        usd
        usdGeom
        ${ALEMBIC_LIBRARIES}
        ${OPENEXR_Iex_LIBRARY}
        ${OPENEXR_Half_LIBRARY}
        ${optionalLibs}

    INCLUDE_DIRS
        ${ALEMBIC_INCLUDE_DIR}
        ${OPENEXR_INCLUDE_DIRS}
        ${optionalIncludeDirs}

    PRIVATE_CLASSES
        alembicData
        alembicFileFormat
        alembicReader
        alembicTest
        alembicUtil
        alembicWriter

    PRIVATE_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapAlembicTest.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
)

pxr_test_scripts(
    testenv/testUsdAbcAlembicData.py       
    testenv/testUsdAbcBugs.py              
    testenv/testUsdAbcCamera.py  
    testenv/testUsdAbcConversionSubdiv.py 
    testenv/testUsdAbcInstancing.py 
)

pxr_install_test_dir(
    SRC testenv/testUsdAbcAlembicData
    DEST testUsdAbcAlembicData
)

pxr_install_test_dir(
    SRC testenv/testUsdAbcBugs
    DEST testUsdAbcBugs
)

pxr_install_test_dir(
    SRC testenv/testUsdAbcCamera
    DEST testUsdAbcCamera
)

pxr_install_test_dir(
    SRC testenv/testUsdAbcConversionSubdiv
    DEST testUsdAbcConversionSubdiv
)

pxr_install_test_dir(
    SRC testenv/testUsdAbcInstancing
    DEST testUsdAbcInstancing
)

pxr_install_test_dir(
    SRC testenv/testUsdAbcInstancingXinst
    DEST testUsdAbcInstancingXinst
)

pxr_install_test_dir(
    SRC testenv/testUsdAbcInstancingNinst
    DEST testUsdAbcInstancingNinst
)

pxr_install_test_dir(
    SRC testenv/testUsdAbcInstancingPinst
    DEST testUsdAbcInstancingPinst
)

pxr_register_test(testUsdAbcAlembicData
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdAbcAlembicData"
    EXPECTED_RETURN_CODE 0
    ENV 
        USD_ABC_NUM_OGAWA_STREAMS=2
)

pxr_register_test(testUsdAbcBugs
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdAbcBugs"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdAbcCamera
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdAbcCamera"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdAbcConversionSubdiv
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdAbcConversionSubdiv"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdAbcInstancing
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdAbcInstancing"
    DIFF_COMPARE nestedInstancing.def.usda
    EXPECTED_RETURN_CODE 0
    ENV
        USD_ABC_TESTSUFFIX=def
)

pxr_register_test(testUsdAbcInstancingXinst
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdAbcInstancing"
    DIFF_COMPARE nestedInstancing.xinst.usda
    EXPECTED_RETURN_CODE 0
    ENV
        USD_ABC_TESTSUFFIX=xinst
        USD_ABC_EXPAND_INSTANCES=1
)

pxr_register_test(testUsdAbcInstancingPinst
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdAbcInstancing"
    DIFF_COMPARE nestedInstancing.pinst.usda
    EXPECTED_RETURN_CODE 0
    ENV
        USD_ABC_TESTSUFFIX=pinst
        USD_ABC_PARENT_INSTANCES=0
)

pxr_register_test(testUsdAbcInstancingNinst
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdAbcInstancing"
    DIFF_COMPARE nestedInstancing.ninst.usda
    EXPECTED_RETURN_CODE 0
    ENV
        USD_ABC_TESTSUFFIX=ninst
        USD_ABC_DISABLE_INSTANCING=1
)
